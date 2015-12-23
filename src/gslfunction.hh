/**
   \file gslfunction.hh
   Access from Ruby to GSL special mathematical functions (and others)
   Copyright 2012 by CNRS/AMU

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
#ifndef __GSLFUNCTION_HH
#define __GSLFUNCTION_HH

#include <ruby-wrappers.h>

/// Base class for all the functions
///
/// @todo Rename as "special functions"
class GSLFunction {
  static QList<GSLFunction *> * functions;

  void registerSelf();
public:

  /// The (ruby) name of the function
  QString name;

  /// A short description
  QString desc;

  /// An optional URL
  QString url;

  /// Returns a description (generally desc, but may be more complex)
  virtual QString description() const;
  

  /// Registers the function to the Ruby interpreter, under the given
  /// module
  virtual void registerFunction(RUBY_VALUE module) = 0;

  /// Register all functions to the Ruby interpreter. Returns the
  /// "Special" module
  static RUBY_VALUE registerAllFunctions();

  /// Creates and registers the given object.
  GSLFunction(const QString & n, const QString & d, const QString &u,
              bool autoreg = true);

  /// Returns a markup-friendly list of all available functions
  static QString availableFunctions();
};


// We also add a GSLConstant class here, although more to avoid the
// hassle to create and additional file ;-)...

/// Represents one of the constants available in the Ruby code
class GSLConstant {
  static QList<GSLConstant *> * constants;

  void registerSelf();

  /// Make this constant available to Ruby code
  void registerConstant();;

public:

  /// All the available names (the first one is used for ordering)
  QStringList names;

  /// A short description
  QString description;

  /// The value
  double value;


  /// Creates and registers all constants at startup time
  static void registerAllConstants();

  /// Returns a markup-friendly list of available constants
  static QString availableConstants();

  GSLConstant(const QString & n, const QString & d, 
              double value, bool autoreg = true);
  GSLConstant(const QStringList & ns, const QString & d, 
              double value, bool autoreg = true);

  
};

/// @name Specially-defined functions
///
/// @{

/// Returns atan(x)/x, with a formula that is accurate even for small
/// x.
double qsoas_atanc(double value);

/// Returns atanh(x)/x, with a formula that is accurate even for small
/// x.
double qsoas_atanhc(double value);

/// @} 

#endif
