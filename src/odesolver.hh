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

#include <headers.hh>
#ifndef __ODESOLVER_HH
#define __ODESOLVER_HH


/// This is the base class for all the ODE solver problems. To use it,
/// you must use a derived class and reimplement the
///
///
/// @todo Add automatic jacobian computation ? (don't know if that's
/// that smart...) Possibly using gsl numerical differentiation
/// functions ?
class ODESolver {

  /// Initialize the driver function.
  void initializeDriver();

  /// Frees the driver
  void freeDriver();

  /// The underlying driver 
  gsl_odeiv2_driver * driver;

  /// The type of the solver
  const gsl_odeiv2_step_type * type;

  /// The current values
  double *yValues;

  /// The current time
  double t;

  /// The function computing the next derivative
  static int function(double t, const double y[], double dydt[], 
                      void * params);


  /// Underlying system
  gsl_odeiv2_system system;
public:

  /// Builds a solver object.
  ///
  /// @todo Add "control" object selection (and parametrization)
  ODESolver(const gsl_odeiv2_step_type * T = gsl_odeiv2_step_rkf45);

  virtual ~ODESolver();

  /// Returns the dimension of the problem
  virtual int dimension() const = 0;

  /// Computes the derivatives at the given point, and store them in
  /// \a dydt
  virtual int computeDerivatives(double t, const double * y, 
                                 double * dydt) = 0;

  /// Resets the solver to the given starting values
  void initialize(const double * yStart, double tstart);

  /// Returns the current Y values
  const double * currentValues() const {
    return yValues;
  };

  /// Returns the current time
  double currentTime() const {
    return t;
  };

  /// Steps to the given time
  ///
  /// This function probably should raise an exception upon GSL error
  void stepTo(double t);
  
};

#endif
