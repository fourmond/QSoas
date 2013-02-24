/**
   \file valuehash.hh
   A string->value hash
   Copyright 2013 by Vincent Fourmond

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
#ifndef __VALUEHASH_HH
#define __VALUEHASH_HH

/// This class embeds a series of related information, such as dataset
/// meta-data, that can be represented as key/value pairs.
///
/// Values may be numeric, but also dates, plain text.
///
/// @todo This should be convertible to a Ruby hash, and in particular
/// many functions should be able to store the results they display
/// into a Ruby hash. An interface still needs to be designed for
/// that.
class ValueHash : public QHash<QString, QVariant> {
protected:
  /// Extract keys that can be converted to the given type.
  ///
  /// Unfortunately, as QVariant doesn't support template convert (at
  /// least as of Qt 4.7), it is not possible to be rid of the
  /// QVariant::Type argument. As a result, this function isn't
  /// exposed.
  template<class T> QHash<QString, T> extract(QVariant::Type type) const;



  /// Used for the << "key" << value scheme
  QString lastKey;
public:

  /// Extract values that can be converted to double.
  QHash<QString, double> extractDoubles() const;

  /// Extract values that can be converted to strings (ie most ?)
  QHash<QString, QString> extractStrings() const;

  /// Extract values that can be converted to dates
  QHash<QString, QDateTime> extractDates() const;


  /// List of keys for ordered output. Keys not present here just come
  /// at the end (in a random order !).
  QStringList keyOrder;
  
  
  /// Returns the values of the elements in the order specified by
  /// keyOrder (or unspecified else !) separated by "\t" (or \a
  /// sep). Missing elements specified in keyOrder will be replaced by
  /// \a missing.
  ///
  /// @todo Maybe make a function to write out several of those in a
  /// row ?
  QString toString(const QString & sep = "\t", 
                   const QString & missing = "",
                   bool skipUnordered = false) const;

  /// Formats the elements as "key = value" in tab-separated columns
  ///
  /// @todo Customization.
  QString prettyPrint(int nbCols = 3) const;

  /// Depending on the number of times used so far:
  /// @li if even, sets the key for the next addition
  /// @li if odd, sets the value corresponding to the last key
  ///
  /// keyOrder is the same as the arguments
  ///
  /// This allows uses like:
  ///
  /// \code
  /// hsh << "key" << value;
  /// \endcode
  ValueHash& operator <<(const QVariant & var);

};


#endif
