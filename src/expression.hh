/**
   \file expression.hh
   Mathematical expression handling (by Ruby) 
   Copyright 2012, 2013, 2014 by CNRS/AMU

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
#ifndef __EXPRESSION_HH
#define __EXPRESSION_HH

#include <vector.hh>

/// This class represents a mathematical expression, internally
/// handled by Ruby.
///
/// @todo Derivatives !
class Expression {

  /// The expression
  QString expression;

  /// The current Ruby code for this expression.
  VALUE code;

  /// The list of doubles used as cache for argument passing.
  VALUE * args;

  /// The list of variables naturally present in the expression, in
  /// the order in which they are found by the Ruby code.
  QStringList minimalVariables;

  /// The current list of variables. Empty means the same as
  /// minimalVariables.
  QStringList variables;

  /// If that index isn't negative, then the expression actually
  /// reduces to a single variable whose index is given
  int singleVariableIndex;


  /// The ID of the function call !
  static ID callIDCache;

  /// And the way to access it
  static ID callID();

  /// Returns the hash for safe-keeping of the Ruby procs, ie to avoid
  /// Ruby GC to treat them as unreferenced
  static VALUE codeSafeKeepingHash();

  /// Returns the hash for safe-keeping of the Ruby arrays of doubles,
  /// ie to avoid Ruby GC to treat them as unreferenced
  static VALUE argsSafeKeepingHash();

  /// "frees" the code associated with the expression.
  void freeCode();

  /// Returns a unique key for the current hash.
  VALUE hashKey();

  /// Builds the code, using the current variable list.
  void buildCode();

  /// Builds the args array
  void buildArgs();

  /// Evaluate as a Ruby VALUE
  VALUE rubyEvaluation(const double * values) const;

public:

  /// Creates an expression object (and compile it)
  Expression(const QString & expression);

  /// Creates an expression object responding to the given variables.
  ///
  /// Equivalent to creating and setting the variables, excepted in
  /// the case when \a skipAutodetect is true, in which case the
  /// detection of natural variables isn't performed.
  Expression(const QString & expression, 
             const QStringList & variables, bool skipAutodect = false);

  /// A neat copy constructor.
  Expression(const Expression & expression);


  /// Frees up all associated storage
  ~Expression();

  /// Returns true if the expression reduces to only
  bool isAVariable() const;

  /// Evaluate the expression with the given values for the variables
  double evaluate(const double * variables) const;

  
  /// Evaluate the expression as a boolean
  bool evaluateAsBoolean(const double * variables) const;

  /// Evaluates the expression as an array of doubles. Most probably
  /// not very efficient, but does not require to know in advance how
  /// many numbers are expected.
  Vector evaluateAsArray(const double * variables) const;


  /// This takes:
  /// @li a list of named parameters
  /// @li a Ruby expression given values to them
  /// @li a target array of doubles meant to get the values of the
  ///    parameters in the same order as the first argument
  ///
  /// Uses evaluateIntoArray internally.
  ///
  /// @todo This function should be used about everywhere.
  static void setParametersFromExpression(const QStringList & params,
                                          const QString &expression,
                                          double * target,
                                          bool forceDefault = true,
                                          double defaultValue = 0);
  

  /// Evaluate into an array. This can be used when an expression
  /// isn't expected to return a single value, but rather an array of
  /// values (think applyFormula), and we know more or less how many
  /// there will be in advance. If that's not the case, it is better
  /// to use evaluateAsArray().
  ///
  /// \a target is the storage space meant to receive the variables,
  /// \a size its size
  ///
  /// The return value is the number of values that resulted from the
  /// expression. It can be greater than \a size (but in that case,
  /// some values are simply lost).
  int evaluateIntoArray(const double * variables, double * target, 
                        int size) const;

  /// Change the list of variables we're evaluating against. It has to
  /// contain all the natural variables, but can be larger than that,
  /// of course.
  void setVariables(const QStringList & vars);

  /// returns the current variables
  const QStringList & currentVariables() const;

  /// Return the natural variables
  const QStringList & naturalVariables() const;

  /// Returns the list of variables needed to process the given
  /// expression, in addition to the ones given as second argument.
  static QStringList variablesNeeded(const QString & expression,
                                     const QStringList & variables = QStringList());

  /// Returns the formula of the expression.
  QString formula() const {
    return expression;
  };

  
  /// Takes a name that people would expect they can use (or maybe
  /// simply they have to use), and transform it into somethin Ruby
  /// can chew.
  static QString rubyIzeName(const QString &name);

  /// RubyIze a full expression, ie transform all the variables given
  /// in the expression
  static QString rubyIzeExpression(const QString &expr, 
                                   QStringList & variables);

  
};

#endif
