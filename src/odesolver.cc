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
#include <argument-templates.hh>

ODEStepperOptions::ODEStepperOptions(const gsl_odeiv2_step_type * t,
                                     double hs, double ea, 
                                     double er, bool f) :
  type(t), hStart(hs), epsAbs(ea), epsRel(er), fixed(f)
{
}

QHash<QString, const gsl_odeiv2_step_type *> stepperTypes()
{
  QHash<QString, const gsl_odeiv2_step_type *> types;
  types["rk2"] = gsl_odeiv2_step_rk2;
  types["rk4"] =  gsl_odeiv2_step_rk4;
  types["rkf45"] = gsl_odeiv2_step_rkf45;
  types["rkck"] = gsl_odeiv2_step_rkck;
  types["rk8pd"] = gsl_odeiv2_step_rk8pd;
  types["rk1imp"] = gsl_odeiv2_step_rk1imp;
  types["rk2imp"] = gsl_odeiv2_step_rk2imp;
  types["rk4imp"] = gsl_odeiv2_step_rk4imp;
  types["bsimp"] = gsl_odeiv2_step_bsimp;
  types["msadams"] = gsl_odeiv2_step_msadams;
  types["msbdf"] = gsl_odeiv2_step_msbdf;
  return types;
}

CommandOptions ODEStepperOptions::asOptions() const
{
  CommandOptions opts;

  opts["adaptative"] = new ArgumentMarshallerChild<bool>(! fixed);
  opts["step-size"] = new ArgumentMarshallerChild<double>(hStart);
  opts["prec-relative"] = new ArgumentMarshallerChild<double>(epsRel);
  opts["prec-absolute"] = new ArgumentMarshallerChild<double>(epsAbs);
  opts["stepper"] = new ArgumentMarshallerChild<const gsl_odeiv2_step_type *>(type);

  return opts;
}


QList<Argument*> ODEStepperOptions::commandOptions()
{
  QList<Argument*> args;
  args << new BoolArgument("adaptative", "Adaptative step",
                           "Whether or not to use adaptative stepper")
       << new NumberArgument("step-size", "Step size",
                             "Initial step size for the stepper")
       << new NumberArgument("prec-relative", "Relative precision",
                             "Relative precision required")
       << new NumberArgument("prec-absolute", "Absolute precision",
                             "Absolute precision required")
       << new TemplateChoiceArgument<const gsl_odeiv2_step_type *>
    (stepperTypes(), "stepper", "Stepper algorithm",
     "Algorithm used for integration");
  return args;
}

void ODEStepperOptions::parseOptions(const CommandOptions & opts)
{
  bool adapt = ! fixed;
  updateFromOptions(opts, "adaptative", adapt);
  fixed = ! adapt;
  updateFromOptions(opts, "step-size", hStart);
  updateFromOptions(opts, "stepper", type);
  updateFromOptions(opts, "prec-relative", epsRel);
  updateFromOptions(opts, "prec-absolute", epsAbs);
}

QString ODEStepperOptions::description() const
{
  QString s = "unknown";
  QHash<QString, const gsl_odeiv2_step_type *> types = stepperTypes();
  for(auto i = types.begin(); i != types.end(); i++) {
    if(i.value() == type) {
      s = i.key();
      break;
    }
  }
  return QString("%7 %1, with %2step size: %3 %4-- absolute precision: %5, relative precision: %6").
    arg(fixed ? "fixed" : "adaptative").
    arg(fixed ? "" : "initial ").
    arg(hStart).
    arg(hStart == 0 && fixed ? "(step is data delta_t) " :"").
    arg(epsAbs).
    arg(epsRel).
    arg(s);
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
  double hs = options.hStart;
  if(hs == 0)
    hs = 0.01;                  // As good as anything ?
  driver = gsl_odeiv2_driver_alloc_y_new(system, options.type, 
                                         hs,
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
  int status = GSL_FAILURE;
  double orig = *t;
  if(! driver)
    throw InternalError("Using step() on an unitialized ODEStepper");
  if(options.fixed) {
    // We choose the closest integer number of points
    int nb = 1;
    if(options.hStart > 0)
       nb = (int) ceil((t1 - *t)/options.hStart);
    double step = (t1 - *t)/nb;
    status = gsl_odeiv2_driver_apply_fixed_step(driver, t, step, nb, y);
  }
  else {
    status = gsl_odeiv2_driver_apply(driver, t, t1, y);
    if(status == GSL_FAILURE) {
      // We try to reset the driver with the initial step where we are
      // now and proceeed
      double hs = options.hStart;
      if(hs == 0.0)
        hs = 0.01;
      //      gsl_odeiv2_driver_reset_hstart(driver, hs);
      gsl_odeiv2_driver_reset_hstart(driver);
      driver->h = hs;
      return apply(t, t1, y);
    }
  }

  // QTextStream o(stdout);
  // o << "Step from " << orig << " to " << t1  << "(dt= " << t1 - *t << ")"
  //   << ":\t h=" << driver->h << " -> " << status <<  "\t" 
  //   << "last_step: " << driver->e->last_step <<  "\tfailed:" 
  //   << driver->e->failed_steps << endl;
  return status;
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
  solver->evaluations++;
  return solver->computeDerivatives(t, y, dydt);
}

void ODESolver::resetStepper()
{
  stepper.reset();
}

int ODESolver::jacobian(double t, const double y[], 
                        double * dfdy,
                        double dydt[], 
                        void * params)
{
  ODESolver * solver = static_cast<ODESolver*>(params);
  
  solver->computeDerivatives(t, y, dydt);
  solver->evaluations++;
  int sz = solver->dimension();
  double parameters[sz];
  double buffer[sz];
  memcpy(parameters, y, sizeof(parameters));
  
  for(int i = 0; i < sz; i++) {
    double orig = parameters[i];
    double step = orig * 1e-7;
    if(fabs(step) < 1e-13)
      step = 1e-13;
    parameters[i] = orig + step;
    double fact = 1/step;
    solver->computeDerivatives(t, parameters, buffer);
    solver->evaluations++;
    for(int j = 0; j < sz; j++)
      dfdy[j * sz + i] = (buffer[j] - dydt[j]) * fact;

    parameters[i] = orig;
  }
  return GSL_SUCCESS;
}


void ODESolver::initializeDriver()
{
  yValues = new double[dimension()];
  system.function = &ODESolver::function;
  system.jacobian = &ODESolver::jacobian;       // Hey !
  system.dimension = dimension();
  system.params = this;
  
  stepper.initialize(&system);
}

void ODESolver::freeDriver()
{
  delete[] yValues;
  yValues = NULL;
}

ODESolver::~ODESolver()
{
  freeDriver();
}

void ODESolver::setStepperOptions(const ODEStepperOptions & opts)
{
  stepper.setOptions(opts);
  freeDriver();                 // Force reinitialization of driver
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
  evaluations = 0;
}

void ODESolver::stepTo(double to)
{
  double dt = to - t;
  double org = t;
  int status = stepper.apply(&t, to, yValues);
  if(status != GSL_SUCCESS) {
    throw RuntimeError("Integration failed to give the desired "
                       "precision stepping from %1 to %2 (last delta= %4): %3 (%5)").
      arg(org).arg(to).arg(gsl_strerror(status)).arg(to-t).arg(status);
  }
}

QList<Vector> ODESolver::steps(const Vector &tValues, bool annotate)
{
  QList<Vector> ret;
  int dim = dimension();
  for(int i = 0; i < dimension(); i++)
    ret += tValues;
  if(annotate)
    ret += tValues;
  for(int i = 0; i < tValues.size(); i++) {
    int last = evaluations;
    stepTo(tValues[i]);
    for(int j = 0; j < dim; j++)
      ret[j][i] = yValues[j];
    if(annotate)
      ret[dim][i] = evaluations - last;
  }
  return ret;
}

