/**
   \file argumentlist.hh
   Handling of Argument Lists for QSoas.
   Copyright 2011 by Vincent Fourmond

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

#ifndef __ARGUMENTLIST_HH
#define __ARGUMENTLIST_HH
#include <argument.hh>

class Command;

/// An argument list, a wrapper around a QList of Arguments. It is
/// used both for the argument list, for which the
/// Argument::argumentName only has an indicative value and for the
/// options, for which the Argument::argumentName is the option key.
///
/// @todo Add the possibility for options to slurp all unkown options
/// as strings.
class ArgumentList : public QList<Argument *> {
  
  mutable QHash<QString, Argument *> cache;

  /// The number of the greedy arg, or -1 if there isn't.
  mutable int greedyArg;

  /// Regenerate the cache, along with
  void regenerateCache() const;


  int assignArg(int i, int total) const; 
  
  
public:
  ArgumentList(const QList<Argument *> & lst);
  ArgumentList();

  /// Whether the list contains an argument of the given name.
  bool contains(const QString & name) const;

  /// Returns the named argument, or NULL if there isn't any
  Argument * namedArgument(const QString & name) const;

  /// Returns the names of all the arguments.
  QStringList argumentNames() const;

  /// Returns the number of the Argument object used for taking care
  /// of the numbered argument string, taking into account greedy
  /// arguments.
  int assignArgument(int arg, int total) const;

  /// Returns the Argument that should take care of the given numbered
  /// arg, considering that there are altogether \p total arguments.
  Argument * whichArgument(int arg, int total) const {
    return value(assignArgument(arg, total), NULL);
  }; 

  /// Parse the given arguments, prompting using the given widget as
  /// base when necessary.
  CommandArguments parseArguments(const QStringList & args,
                                  QWidget * base) const;
  
};

#endif
