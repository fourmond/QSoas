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

ODESolver::ODESolver(const gsl_odeiv2_step_type * t) :
  driver(NULL), type(t), yValues(NULL)
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

  driver = gsl_odeiv2_driver_alloc_y_new(&system, type, 
                                         0.01, 1e-6, 1e-6);
}

void ODESolver::freeDriver()
{
  if(! driver) 
    return;
  gsl_odeiv2_driver_free(driver);
  delete[] yValues;
}

ODESolver::~ODESolver()
{
  freeDriver();
}

void ODESolver::initialize(const double * yStart, double tStart)
{
  if(! driver)
    initializeDriver();

  gsl_odeiv2_driver_reset(driver);

  for(int i = 0; i < system.dimension; i++)
    yValues[i] = yStart[i];
  t = tStart;
}

void ODESolver::stepTo(double to)
{
  int status = gsl_odeiv2_driver_apply(driver, &t, to, yValues);
}
