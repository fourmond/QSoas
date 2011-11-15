/**
   \file bijection.hh
   Bijective mathematical transformations
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


#ifndef __BIJECTION_HH
#define __BIJECTION_HH

class Bijection;

class BijectionFactoryItem {
public:
  typedef Bijection * (*Creator)();

protected:
  friend class Bijection;
  /// The creation function
  Creator creator;

public:
  /// The creation name
  QString name;

  /// The public name
  QString publicName;

  /// Parameter list
  QStringList parameters;

  /// Creates and register a factory item.
  BijectionFactoryItem(Creator c);
};

/// A Bijection is a two way transformation from R to a possibly
/// smaller subset of R.
class Bijection {

  /// The application-wide Bijection factory
  static QHash<QString, BijectionFactoryItem*> * factory;

  friend class BijectionFactoryItem;

  static void registerFactoryItem(BijectionFactoryItem * item);


  static BijectionFactoryItem * namedFactoryItem(const QString & name);
  
public:

  /// The name of the bijection.
  virtual QString name() const = 0;

  /// A public name
  virtual QString publicName() const = 0;

  /// A list of parameter names that the Bijection takes.
  virtual QStringList parameters() const;

  /// The forward transformation (from R to the target subset)
  virtual double forward(double x) const = 0;
  
  /// The backward transformation (from the subset to R).
  ///
  /// @warning This function should handle gracefully out-of-range
  /// errors.
  virtual double backward(double y) const = 0;

  /// Creates a Bijection with the given name
  static Bijection * createNamedBijection(const QString & name);

  /// Loads a Bijection from text, including parsing of parameters.
  static Bijection * loadFromText(const QString & spec);

  /// Returns the available items
  static QList<const BijectionFactoryItem *> factoryItems();
};


#endif
