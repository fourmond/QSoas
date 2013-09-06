/**
   \file integrator.hh
   The Integrator class, a one dimensional root finder
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
#ifndef __INTEGRATOR_HH
#define __INTEGRATOR_HH

#include <gsl/gsl_integration.h>


class Integrator {
protected:

  /// Precision: absolute
  double absolutePrec;

  /// relative precision
  double relativePrec;

  /// Maximum of intervals
  int max;

  /// Number of function calls for the integration
  int funcalls;

  /// The integration workspace
  gsl_integration_workspace * workspace;

  /// The underlying function, set upon 
  std::function<double (double)> fnc;

  /// The integration function
  static double f(double x, void * params);


  /// The function pointing towards f
  gsl_function intf;
  
public:

  Integrator(int intervals = 20, 
             double rel = 1e-4, double abs = 0);

  ~Integrator();

  /// Integrate over a segment
  double integrateSegment(const std::function<double (double)> & f, 
                          double a, double b, double * error = NULL,
                          int limit = -1,
                          int key = 3);
                          

  /// Returns the number of function calls during the last
  /// integration.
  int functionCalls() const {
    return funcalls;
  };
};

#endif
