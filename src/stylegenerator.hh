/**
   \file stylegenerator.hh
   Automatic generation of styles for display purposes
   Copyright 2013 by CNRS/AMU

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
#ifndef __STYLEGENERATOR_HH
#define __STYLEGENERATOR_HH

#include <vector.hh>

class StyleGenerator;

class StyleGeneratorFactoryItem {
public:
  /// A generator function, taking the overall number of styles to
  /// give and an optional string argument.

  // Hell, clang doesn't have std::function !
  // typedef std::function<StyleGenerator * (int, const QString &)>
  // Creator;

  typedef StyleGenerator * (*Creator) (int, const QString &);

protected:
  friend class StyleGenerator;
  /// The creation function
  Creator creator;

public:
  /// The creation name
  QString name;

  /// The public name
  QString publicName;

  /// Creates and register a factory item.
  StyleGeneratorFactoryItem(const QString & n, 
                            const QString & pn, Creator c);
};

/// Generates a style for the next curve.
class StyleGenerator {

  /// The application-wide StyleGenerator factory
  static QHash<QString, StyleGeneratorFactoryItem*> * factory;

  friend class StyleGeneratorFactoryItem;

  static void registerFactoryItem(StyleGeneratorFactoryItem * item);

  static StyleGeneratorFactoryItem * namedFactoryItem(const QString & name);

protected:

  /// The total number of styles we're going to ask from this
  /// StyleGenerator. May be a negative number in case the number is
  /// not known ?
  int totalNumber;

public:

  /// Creates a StyleGenerator with the given name
  static StyleGenerator * createNamedGenerator(const QString & name,
                                               int nb = -1, 
                                               const QString & arg = "");

  /// almost the same as createNamedGenerator, excepted that the
  /// argument is parsed from the string. 
  static StyleGenerator * fromText(const QString & name, int nb = -1);

  /// Returns the names of the available generators
  static QStringList availableGenerators();

  /// We need a virtual destructor.
  virtual ~StyleGenerator() {;};


  /// Sets the total number of elements to give a style to.
  void setTotal(int nb) { totalNumber = nb;};

  StyleGenerator(int nb) : totalNumber(nb) { ; }; 

  /// Generates the style for the next curve. @question Maybe just
  /// returning a QPen isn't enough ?
  virtual QPen nextStyle() = 0;
};


#endif
