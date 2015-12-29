/**
   \file functions.hh
   Mathematical functions
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
#ifndef __FUNCTIONS_HH
#define __FUNCTIONS_HH



/// This namespace contains mathematical functions
namespace Functions {

  /// "Cardinal" arctangent (accurate also for small x
  double atanc(double x);

  /// "Cardinal" hyperbolic arctangent (accurate also for small x)
  double atanhc(double x);

  /// Pseudo-voigt function
  double pseudoVoigt(double x, double w, double mu);

};

#endif
