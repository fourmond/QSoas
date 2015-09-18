/*
  multiintegrators.cc: various implementations of the 
  Copyright 2015 by CNRS/AMU

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
#include <multiintegrator.hh>

class NaiveMultiIntegrator : public  MultiIntegrator {
public:
  NaiveMultiIntegrator(Function fnc, int dim, double rel = 1e-4, double abs = 0, int maxc = 0) :
    MultiIntegrator(fnc, dim, rel, abs, maxc) {
  };

  virtual double integrate(gsl_vector * res, double a, double b) {
    
    return 0;
  };

};
