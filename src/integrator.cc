/*
  integrator.cc: implementation of the Integrator class
  Copyright 2013 by Vincent Fourmond

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
#include <integrator.hh>

#include <exceptions.hh>

Integrator::Integrator(int intervals, double rel, double abs) :
  absolutePrec(abs), relativePrec(rel)
{
  
  workspace = gsl_integration_workspace_alloc(intervals);
  max = intervals;
  intf.params = this;
  intf.function = &Integrator::f;
}

double Integrator::f(double x, void * params)
{
  Integrator * i = reinterpret_cast<Integrator*>(params);
  i->funcalls++;
  return i->fnc(x);
}

Integrator::~Integrator()
{
  gsl_integration_workspace_free(workspace);
}

double Integrator::integrateSegment(const std::function<double (double)> & f, 
                                    double a, double b, double * error,
                                    int limit, int key)
{
  double res = 0;
  double err = 0;
  if(limit < 0)
    limit = max;
  funcalls = 0;
  fnc = f;
  
  int status = gsl_integration_qag(&intf, a, b, absolutePrec, relativePrec,
                                   limit, key, workspace, &res, &err);
  if(error)
    *error = err;
  return res;
}



