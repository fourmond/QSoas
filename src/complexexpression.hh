/**
   \file complexexpression.hh
   Subclass of Expression handling the case of a unique complex variable
   with additional real parameters
   Copyright 2024 by CNRS/AMU

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
#ifndef __COMPLEXEXPRESSION_HH
#define __COMPLEXEXPRESSION_HH

#include <gcguard.hh>

/// This class is a small copy of Expression that handles functions of
/// a complex variable with (optional) parameters.
///
/// All the arguments but the complex one are real
class ComplexExpression {

  /// The expression being used
  QString expression;

  /// The complex variable
  QString variable;

  /// The minimal variables, including the complex one even if it isn't used
  QStringList effectiveMinVars;

  /// The real list of variables
  QStringList variables;

  /// The underlying Ruby Proc object
  mrb_value code;

  /// The list of doubles used as cache for argument passing.
  mrb_value * args;

  /// The current size of args
  int argsSize;

  /// A guard against GC
  GCGuard guard;

  /// The index of the variables used to build the code
  int * indexInVariables;

  /// Builds the code, using the current variable list.
  void buildCode();

  /// Frees the code
  void freeCode();


  
  /// Builds the args array
  void buildArgs();

  /// Evaluate as a Ruby VALUE
  mrb_value rubyEvaluation(const double * values) const;

public:

  /// Creates a complex expression object (and compile it)
  ComplexExpression(const QString & variable,
                    const QString & expression);

  /// Let's disable this for now.
  ComplexExpression(const ComplexExpression & o);

  ~ComplexExpression();
  
  /// @name Evalution functions
  ///
  /// All the functions in here use the Ruby global lock excepted
  /// those whose name ends in NoLock
  ///
  /// @{

  /// Evaluate the complexexpression with the given values for the variables
  gsl_complex evaluate(const double * variables) const;

  /// Evaluate the complexexpression and returns the given ruby variable
  mrb_value evaluateAsRuby(const double * variables) const;

  /// @}

  /// Change the list of variables we're evaluating against. It has to
  /// contain all the natural variables, but can be larger than that,
  /// of course.
  ///
  /// Whatever happens, the complex variable is always the first
  void setVariables(const QStringList & vars);

  /// The current list of variables
  QStringList currentVariables() const;

  /// Returns the formula of the expression.
  QString formula() const;


  /// Performs the reverse Laplace transform of the expression, using
  /// the extra parameters (after the main variable) in @a parameters
  /// the X (t) values pointed to by @a xvalues, the @a target being
  /// set with the Y values (the number of elements is taken from
  /// that), with a number of computation @a steps.
  /// 35 seems to be a good choice for non-oscillatory transforms
  void reverseLaplace(const double * parameters,
                      const double * xvalues,
                      gsl_vector * target,
                      int steps = 35);

};

#endif
