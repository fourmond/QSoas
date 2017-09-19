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

class MRuby;

/// Base class for all the functions
///
/// @todo Rename as "special functions"
class GSLFunction {
  static QList<GSLFunction *> * functions;

  void registerSelf();
public:

  /// The name of the function (with the arguments)
  QString name;

  /// The name of the function, deduced from the name
  QString rubyName;

  /// A short description
  QString desc;

  /// An optional URL
  QString url;

  /// Returns a description (generally desc, but may be more complex)
  virtual QString description() const;
  

  /// Registers the function to the MRuby interpreter
  virtual void registerFunction(MRuby * mr, struct RClass * cls);

    /// "Special" module
  static void registerAllFunctions(MRuby * mr);

  /// Creates and registers the given object.
  GSLFunction(const QString & n, const QString & d, const QString &u,
              bool autoreg = true);

  /// Returns a markup-friendly list of all available functions
  static QString functionDocumentation();

  /// Return the list of available functions
  static QStringList availableFunctions();
};


// We also add a GSLConstant class here, although more to avoid the
// hassle to create and additional file ;-)...

/// Represents one of the constants available in the Ruby code
class GSLConstant {
  static QList<GSLConstant *> * constants;

  void registerSelf();

  /// Make this constant available to Ruby code
  void registerConstant();

  /// Make this constant available to MRuby code
  void registerConstant(MRuby * mr);

public:

  /// All the available names (the first one is used for ordering)
  QStringList names;

  /// A short description
  QString description;

  /// The value
  double value;


  /// Creates and registers all constants at startup time
  static void registerAllConstants();

  /// Creates and registers all constants at startup time
  static void registerAllConstants(MRuby * mr);

  /// Returns a markup-friendly list of available constants
  static QString constantsDocumentation();

  /// Returns the list of available constants
  static QStringList availableConstants();

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
