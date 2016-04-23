/**
   \file solver.hh
   The Solver class, a one dimensional root finder
   Copyright 2013, 2016 by CNRS/AMU

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
#ifndef __SOLVER_HH
#define __SOLVER_HH

#include <argumentmarshaller.hh>

class Argument;

/// Solves a 1-dimensional equation. This is a virtual base class.
///
/// This solver can use two different strategies depending on how they
/// are initialized: dichotomy if used via the range-specifiying
/// initialization or newton stuff in the other case.
class Solver {
protected:
  /// The solver used for the one-variable initial conditions
  gsl_root_fdfsolver * fdfsolver;

  /// The solver used for the dichotomy approach
  gsl_root_fsolver * fsolver;

  /// The function provided to the solver
  gsl_function_fdf function;

  /// The other function provided to the solver
  gsl_function fnc;

  static double  f(double x, void * params);
  static double df(double x, void * params);
  static void  fdf(double x, void * params, double * f, double * df);

  /// Precision: absolute
  double absolutePrec;

  /// relative precision
  double relativePrec;

  /// Maximum number of iterations
  int maxIterations;

  /// frees all solvers
  void freeSolvers();

  /// The type for fdfsolvers.
  const gsl_root_fdfsolver_type * type;

public:
  Solver(const gsl_root_fdfsolver_type * type = 
         gsl_root_fdfsolver_steffenson);

  virtual ~Solver();

  /// Initializes the solver with the given value for using a
  /// newton-like approach
  void initialize(double x0);

  /// Initializes the solver for a bisection-type approach
  void initialize(double min, double max);


  /// @name Options-related functions
  ///
  /// @{

  /// Sets the precision and suchlike
  void parseOptions(const CommandOptions & opts);
  
  /// options suitable for addition 
  static QList<Argument*> commandOptions();

  /// Converts the ODEStepperOptions into plain CommandOptions.
  CommandOptions currentOptions() const;

  /// @}

  /// Returns the value of the function whose root we should find !
  virtual double f(double x) = 0;

  /// Returns the current value of the root
  double currentValue() const;

  /// Performs one iteration
  int iterate();

  /// Solves until the desired relative and absolute accuracies
  double solve(); 

  /// Combines initialize() and the other solve() in one go
  double solve(double x0);

  /// Combines initialize() and the other solve() in one go
  double solve(double min, double max);
};


/// A class for solving an equation that can be provided as a lambda
/// expression
class LambdaSolver : public Solver {
protected:
  std::function<double (double)> fnc;
public:
  LambdaSolver(const std::function<double (double)> & f, 
               const gsl_root_fdfsolver_type * type = 
               gsl_root_fdfsolver_steffenson);

  virtual double f(double x);
};

#endif
