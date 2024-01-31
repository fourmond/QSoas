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

#include <expression.hh>

/// This subclass of Expression handles the case of a complex function
/// of a single variable with optional parameters.
///
/// The main differences with Expression:
/// @li the complex variable is always the first one
/// @li it corresponds to two doubles
class ComplexExpression : public Expression {

  /// The complex variable
  QString variable;

  /// Effective minimal variables, which always includes the complex
  /// variable as first even if it's not actually used
  QStringList effectiveMinVars;

  /// Builds the args array
  void buildArgs();

  /// Builds the code, using the current variable list.
  void buildCode();

  /// Evaluate as a Ruby VALUE
  mrb_value rubyEvaluation(const double * values) const;

public:

  /// Creates a complex expression object (and compile it)
  ComplexExpression(const QString & variable,
                    const QString & expression);

  /// Let's disable this for now.
  ComplexExpression(const ComplexExpression & o) = delete;

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

private:
  /// @name Disabled functions
  ///
  /// These functions are disabled from Expression
  /// @{
  double evaluateNoLock(const double * variables) const;
  bool evaluateAsBoolean(const double * variables) const;
  Vector evaluateAsArray(const double * variables) const;
  Vector evaluateAsArrayNoLock(const double * variables) const;
  int evaluateIntoArray(const double * variables, double * target, 
                        int size) const;
  int evaluateIntoArrayNoLock(const double * variables, double * target, 
                        int size) const;
  /// @}

};

#endif
