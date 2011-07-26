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

/// An argument list, ie a very thin wrapper around a QList of
/// Arguments.
class ArgumentList : public QList<Argument *> {
  
  mutable QHash<QString, Argument *> cache;

  /// Regenerate the cache
  void regenerateCache() const;
  
public:
  ArgumentList(const QList<Argument *> & lst);

  /// Whether the list contains an argument of the given name.
  bool contains(const QString & name) const;

  /// Returns the named argument, or NULL if there isn't any
  Argument * namedArgument(const QString & name) const;
};

#endif
