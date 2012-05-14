/**
   \file expression.hh
   Mathematical expression handling (by Ruby) 
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

#ifndef __EXPRESSION_HH
#define __EXPRESSION_HH

/// This class represents a mathematical expression, internally
/// handled by Ruby.
///
/// @todo Derivatives !
class Expression {

  /// The expression
  QString expression;

  /// The current Ruby code for this expression.
  VALUE code;

  /// The list of variables naturally present in the expression, in
  /// the order in which they are found by the Ruby code.
  QStringList minimalVariables;

  /// The current list of variables. Empty means the same as
  /// minimalVariables.
  QStringList variables;


  /// The ID of the function call !
  static ID callIDCache;

  /// And the way to access it
  static ID callID();

  /// Returns the hash for safe-keeping of the Ruby procs, ie to avoid
  /// Ruby GC to treat them as unreferenced
  static VALUE safeKeepingHash();

  /// "frees" the code associated with the expression.
  void freeCode();

  /// Returns a unique key for the current hash.
  VALUE hashKey();

  /// Builds the code, using the current variable list.
  void buildCode();

public:

  /// Creates an expression object (and compile it)
  Expression(const QString & expression);

  /// Creates an expression object responding to the given variables.
  ///
  /// Equivalent to creating and setting the variables
  Expression(const QString & expression, const QStringList & variables);

  /// A neat copy constructor.
  Expression(const Expression & expression);


  /// Frees up all associated storage
  ~Expression();

  /// Evaluate the expression with the given values for the variables
  double evaluate(const double * variables) const;

  /// Change the list of variables we're evaluating against. It has to
  /// contain all the natural variables, but can be larger than that,
  /// of course.
  void setVariables(const QStringList & vars);

  /// returns the current variables
  const QStringList & currentVariables() const;

  /// Return the natural variables
  const QStringList & naturalVariables() const;

  /// Returns the list of variables needed to process the given
  /// expression
  static QStringList variablesNeeded(const QString & expression);
  
};

#endif
