/**
   \file externalfunction.hh
   The class hierarchy to access computing facilities from other programs
   Copyright 2021 by CNRS/AMU

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
#ifndef __EXTERNALFUNCTION_HH
#define __EXTERNALFUNCTION_HH

#include <vector.hh>

/// An ExternalFunction represents external means to compute the value
/// of a y=f(x, parameters...) function.
/// 
/// The idea behind external is that other programs are launched to do
/// the computation for QSoas (like the Python interpreter
///
/// This class is abstract.
class ExternalFunction {
public:

  /// Computes the function for the given parameters at the given X
  /// values
  virtual Vector compute(const Vector & x, const double * parameters)  = 0;

  /// Returns the list of the parameters (by name)
  virtual QStringList parameters() const = 0;

  /// Returns default values for the parameters (the parameters can be
  /// missing)
  virtual QHash<QString, double> defaultValues() const;
};

#endif
