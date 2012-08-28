/**
   \file odesolver.hh
   Access from Ruby to GSL special mathematical functions
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

#ifndef __ODESOLVER_HH
#define __ODESOLVER_HH


/// This is the base class for all the ODE solver problems. To use it,
/// you must use a derived class and reimplement the
class ODESolver {

  /// Initialize the driver function.
  void initializeDriver();
public:

  /// Returns the dimension of the problem
  virtual int dimension() const = 0;

  /// Computes the derivatives at the given point, and store them in
  /// \a dydt
  virtual int computeDerivatives(double t, const double * y, double * dydt);
