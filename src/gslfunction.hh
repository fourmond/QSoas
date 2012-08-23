/**
   \file gslfunction.hh
   Access from Ruby to GSL special mathematical functions
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

#ifndef __GSLFUNCTION_HH
#define __GSLFUNCTION_HH

/// Base class for all the functions
///
/// @todo Add a description
class GSLFunction {
  static QList<GSLFunction *> * functions;

  void registerSelf();
public:

  /// The (ruby) name of the function
  QString name;

  /// Registers the function to the Ruby interpreter, under the given
  /// module
  virtual void registerFunction(VALUE module) = 0;

  /// Register all functions to the Ruby interpreter. Returns the
  /// "Special" module
  static VALUE registerAllFunctions();

  /// Creates and registers the given object.
  GSLFunction(const QString & n, bool autoreg = true);
};

#endif
