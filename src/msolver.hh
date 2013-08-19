/**
   \file msolver.hh
   The MSolver class, a multidimensional root finder
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
#ifndef __MSOLVER_HH
#define __MSOLVER_HH

/// Multidimensional root finder
class MSolver {
protected:
  /// The solver
  gsl_multiroot_fdfsolver * solver;

  /// The solver type
  const gsl_multiroot_fdfsolver_type * type;

  /// The function provided to the msolver
  gsl_multiroot_function_fdf function;

  static int f(const gsl_vector * x, void * params, gsl_vector * f);
  static int df(const gsl_vector * x, void * params, gsl_matrix * J);
  static int fdf(const gsl_vector * x, void * params, gsl_vector * f, 
                  gsl_matrix * J);

  /// Precision: absolute
  double absolutePrec;

  /// relative precision
  double relativePrec;

  /// Maximum number of iterations
  int maxIterations;


public:
  MSolver(const gsl_multiroot_fdfsolver_type * type = 
          gsl_multiroot_fdfsolver_hybridsj);

  virtual ~MSolver();

  /// Returns the dimension of the system.
  virtual int dimension() const  = 0;

  /// Initializes the msolver with the given value
  void initialize(const gsl_vector * init);

  /// Places the values of the function at coordinates \a x into \a
  /// tg. Returns a GSL error code.
  virtual int f(const gsl_vector * x, gsl_vector * tg) = 0;

  /// Returns the current value of the root
  const gsl_vector * currentValue() const;

  /// Performs one iteration
  int iterate();

  /// Solves until the desired relative and absolute accuracies
  void solve(); 
};


#endif
