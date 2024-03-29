/**
   \file argumentlist.hh
   Handling of Argument Lists for QSoas.
   Copyright 2011 by Vincent Fourmond
             2012, 2013 by CNRS/AMU

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
#ifndef __ARGUMENTLIST_HH
#define __ARGUMENTLIST_HH
#include <argument.hh>

class Command;

/// An argument list, a wrapper around a QList of Arguments. It is
/// used both for the argument list, for which the
/// Argument::argumentName only has an indicative value and for the
/// options, for which the Argument::argumentName is the option key.
///
/// @todo Add the possibility for options to slurp all unknown options
/// as strings.
class ArgumentList {
  
  mutable QHash<QString, const Argument *> cache;

  /// The number of the greedy arg, or -1 if there isn't.
  mutable int greedyArg;

  /// The index of the default option, or -1 if there isn't.
  mutable int defaultOptionIndex;

  /// Regenerate the cache, along with
  void regenerateCache() const;


  int assignArg(int i, int total) const;

  QList<QExplicitlySharedDataPointer<Argument> > arguments;

public:
  ArgumentList(const QList<Argument *> & lst);
  ArgumentList();

  /// @name Array-like operation
  ///
  /// @{

  /// Addition
  ArgumentList & operator<<(Argument * arg);

  /// Addition
  ArgumentList & operator<<(const QList<Argument *> & arg);

  /// Addition
  ArgumentList & operator<<(const ArgumentList & arg);

  /// 
  const Argument * operator[](int idx) const;

  /// 
  const Argument * value(int idx, const Argument * def = NULL) const;

  void insert(int idx, Argument * arg);

  int size() const;

  // QList<Argument *>::const_iterator begin() const;

  // QList<Argument *>::const_iterator end() const;

  /// @}

  /// Whether the list contains an argument of the given name.
  bool contains(const QString & name) const;

  /// Returns the named argument, or NULL if there isn't any
  ///
  /// If there is a special argument (option) named *, then it is
  /// returned for all calls that would otherwise not return an
  /// option.
  const Argument * namedArgument(const QString & name) const;

  /// Returns the names of all the arguments.
  QStringList argumentNames() const;

  /// Parses a ruby hash into command options
  CommandOptions parseRubyOptions(mrb_value hash) const;

  /// Parses a series of Ruby values into command arguments
  CommandArguments parseRubyArguments(int nb, mrb_value * values) const;

  /// Returns the number of the Argument object used for taking care
  /// of the numbered argument string, taking into account greedy
  /// arguments.
  int assignArgument(int arg, int total) const;

  /// Returns the Argument that should take care of the given numbered
  /// arg, considering that there are altogether \p total arguments.
  const Argument * whichArgument(int arg, int total) const;
  
  /// Parse the given arguments, prompting using the given widget as
  /// base when necessary.
  ///
  /// If @a defaultOption isn't NULL, then all the extra arguments end
  /// up there.
  ///
  /// If prompting occurred, then the value pointed to by
  /// @a prompted is set to true
  CommandArguments parseArguments(const QStringList & args,
                                  QStringList * defaultOption,
                                  QWidget * base, bool * prompted = NULL) const;

  /// Whether the ArgumentList has an argument marked as default
  /// option.
  bool hasDefaultOption() const;

  /// Returns the default option, or NULL if there isn't
  const Argument * defaultOption() const;

  /// Merges another argument list while trying to avoid name clashes.
  ///
  /// For now, duplicate names will get ignored. @todo Make this
  /// configurable
  void mergeOptions(const ArgumentList & other);


  /// Converts the given options to a QVariant hash
  QHash<QString, QVariant> optionsHash(const CommandOptions & opts) const;
};

#endif
