/**
   \file implicitexpression.hh
   The ImplicitExpression class, an equivalent of Expression for
   implicit equations
   Copyright 2020, 2024 by CNRS/AMU

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
#ifndef __IMPLICITEXPRESSION_HH
#define __IMPLICITEXPRESSION_HH

#include <solver.hh>

class Expression;

/// This class is the equivalent to Expression, but for equations of
/// one variable that are not explicit, that must be solved.
/// It provides:
/// @li setup of the main equation, including the choice of variable
/// @li reporter expression
/// @li initial seed (@todo: add left/right boundaries)
///
/// 
class ImplicitExpression : protected Solver {
protected:

  /// The formula
  QString formula;

  /// The expression for the main formula
  Expression * expression;

  /// Reporter expression, if the "result" of the expression isn't the
  /// variable we're solving for
  Expression * reporterExpression;

  /// Seed expression, or NULL if we don't use any seeds
  Expression * seedExpression;

  void clear();

  /// The list of all the variables in the expression
  QStringList naturalVariables;

  /// The solver variable
  QString variable;

  /// The function
  /// @b Important: both the expressions and the storage @b MUST be initialized
  double f(double val) override;

  /// A temporary storage for the arrays for the expression.
  /// @b Important: the ImplicitExpression doesn't free it, but it
  /// @b needs to write in the 0 position
  double * storage;

public:
  
  ImplicitExpression(const QString & expression);

  virtual ~ImplicitExpression();

  /// Prepare the expression.
  /// Must be called before calls to any other 
  void prepare();

  /// Sets the variables of the expressions.
  /// The variables @b must @b not contain the variable
  void setVariables(const QStringList & variables);

  /// Returns the variables, excluding the solver variable
  QStringList variables() const;


  /// Returns true if the solver requires a seed
  bool requiresSeed() const;

  /// Solves the equation with the given seed.
  /// It takes an array that contains the parameters, either the ones from
  /// variables() or the ones given to setVariables() with @b an @b additional
  /// space at the beginning.
  ///
  /// This function either returns the solver variable or the results of the
  /// reporter expression. In any case, the first slot of parameters
  /// is filled with the solver variable at the solution.
  double solve(double seed, double * parameters);


  /// @override Same as solve(), using a dichotomy approach
  double solve(double left, double right, double * parameters);

  /// @override Same as solve(), but using the seed provided internally
  /// @todo add the dichotomy here too
  double solve(double * parameters);


};


#endif
