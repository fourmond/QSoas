/*
  odesolver.cc: implementation of the ODE solver class
  Copyright 2012 by Vincent Fourmond

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <headers.hh>
#include <odesolver.hh>

#include <vector.hh>
#include <exceptions.hh>

#include <argumentmarshaller.hh>
#include <general-arguments.hh>

ODEStepperOptions::ODEStepperOptions(const gsl_odeiv2_step_type * t,
                                     double hs, double ea, 
                                     double er, bool f) :
  type(t), hStart(hs), epsAbs(ea), epsRel(er), fixed(f)
{
}

QList<Argument*> ODEStepperOptions::commandOptions()
{
  QList<Argument*> args;
  args << new BoolArgument("adaptative", "Adaptative step",
                           "Whether or not to use adaptative stepper")
       << new NumberArgument("step-size", "Step size",
                             "Initial step size for the stepper");
  return args;
}

void ODEStepperOptions::parseOptions(const CommandOptions & opts)
{
  bool adapt = ! fixed;
  updateFromOptions(opts, "adaptative", adapt);
  fixed = ! adapt;
  updateFromOptions(opts, "step-size", hStart);
}

QString ODEStepperOptions::description() const
{
  return QString("%1, with %2step size: %3 %4-- absolute precision: %5, relative precision: %6").
    arg(fixed ? "fixed" : "adaptative").
    arg(fixed ? "" : "initial ").
    arg(hStart).
    arg(hStart == 0 && fixed ? "(step is data delta_t) " :"").
    arg(epsAbs).
    arg(epsRel);
}

//////////////////////////////////////////////////////////////////////


ODEStepper::ODEStepper() : 
  driver(NULL), system(NULL)
{
}

void ODEStepper::freeDriver()
{
  if(driver) {
    gsl_odeiv2_driver_free(driver);
    driver = NULL;
  }
}

/// @todo Make provisions for using several different control objects.
void ODEStepper::initialize(gsl_odeiv2_system * system)
{
  freeDriver();
  driver = gsl_odeiv2_driver_alloc_y_new(system, options.type, 
                                         options.hStart,
                                         options.epsAbs,
                                         options.epsRel);
}

void ODEStepper::reset()
{
  if(driver)
    gsl_odeiv2_driver_reset(driver);
}

int ODEStepper::apply(double * t, const double t1, double y[])
{
  if(! driver)
    throw InternalError("Using step() on an unitialized ODEStepper");
  if(options.fixed) {
    // We choose the closest integer number of points
    int nb = 1;
    if(options.hStart > 0)
       nb = (int) ceil((t1 - *t)/options.hStart);
    double step = (t1 - *t)/nb;
    return gsl_odeiv2_driver_apply_fixed_step(driver, t, step, nb, y);
  }
  else
    return gsl_odeiv2_driver_apply(driver, t, t1, y);
}

void ODEStepper::setOptions(const ODEStepperOptions & opts)
{
  freeDriver();
  options = opts;
}

ODEStepper::~ODEStepper()
{
  freeDriver();
}


//////////////////////////////////////////////////////////////////////

ODESolver::ODESolver() : yValues(NULL)
{
}

int ODESolver::function(double t, const double y[], double dydt[], 
                        void * params)
{
  ODESolver * solver = static_cast<ODESolver*>(params);
  return solver->computeDerivatives(t, y, dydt);
}


void ODESolver::initializeDriver()
{
  yValues = new double[dimension()];
  system.function = &ODESolver::function;
  system.jacobian = NULL;       // Hey !
  system.dimension = dimension();
  system.params = this;
  
  stepper.initialize(&system);
}

void ODESolver::freeDriver()
{
  delete[] yValues;
}

ODESolver::~ODESolver()
{
  freeDriver();
}

void ODESolver::setStepperOptions(const ODEStepperOptions & opts)
{
  stepper.setOptions(opts);
}

const ODEStepperOptions & ODESolver::getStepperOptions()
{
  return stepper.getOptions();
}

void ODESolver::initialize(const double * yStart, double tStart)
{
  if(! yValues)
    initializeDriver();
  stepper.reset();

  for(int i = 0; i < system.dimension; i++)
    yValues[i] = yStart[i];
  t = tStart;
}

void ODESolver::stepTo(double to)
{
  
  int status = stepper.apply(&t, to, yValues);
  if(status != GSL_SUCCESS) {
    throw RuntimeError("Integration failed to give the desired "
                       "precision stepping from %1 to %2").
      arg(t).arg(to);
  }
}

QList<Vector> ODESolver::steps(const Vector &tValues)
{
  QList<Vector> ret;
  for(int i = 0; i < dimension(); i++)
    ret += tValues;
  for(int i = 0; i < tValues.size(); i++) {
    stepTo(tValues[i]);
    for(int j = 0; j < ret.size(); j++)
      ret[j][i] = yValues[j];
  }
  return ret;
}

