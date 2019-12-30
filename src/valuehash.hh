/**
   \file valuehash.hh
   A string->value hash
   Copyright 2013, 2014, 2019 by CNRS/AMU

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

#include <argumentmarshaller.hh>

class DataSet;
class Argument;

/// This class embeds a series of related information, such as dataset
/// meta-data, that can be represented as key/value pairs.
///
/// Values may be numeric, but also dates, plain text, string lists...
///
/// A particular feature of ValueHash that make it useful to store
/// data is the "ordered key" feature, where displaying/exporting
/// makes keys come in a specific order. This doesn't have to be used,
/// though.
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
  ///
  /// If \a joinStringLists isn't null, then string lists are
  /// converted to string by using join().
  QHash<QString, QString> extractStrings(const QString & joinStringLists = 
                                         QString()) const;

  /// Extract values that can be converted to dates
  QHash<QString, QDateTime> extractDates() const;


  /// List of keys for ordered output. Keys not present here just come
  /// at the end (in a random order !).
  QStringList keyOrder;
  

  /// Clears the hash and the list of keys
  void clear();

  /// Keeps only the given keys, in that order.
  ///
  /// @todo Signal when a key is missing ?
  ValueHash select(const QStringList & keys) const;

  /// Appends a key and its value to the hash
  void append(const QString & key, const QVariant & value);

  /// Prepends a key and its value to the hash
  void prepend(const QString & key, const QVariant & value);

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
  ///
  /// If @a sort is true, then the keys that are not specifically
  /// ordered are sorted alphabetically.
  ///
  /// If @a overrideorder, then keys that are specifically ordered are
  /// not treated differently.
  QString prettyPrint(int nbCols = 3,
                      const QString & prefix = "",
                      const QString & joinStringLists = QString(),
                      bool dumpOther = false,
                      bool sort = true, bool overrideorder = false) const;

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

  /// @overload
  ValueHash& operator <<(const QList<QString> & str);

  /// @overload Adds the value corresponding to the given Ruby
  /// value. 
  ValueHash& operator <<(mrb_value val);


  /// Adds several keys and values to the hash, keeping the order in
  /// keys.
  void multiAdd(const QStringList & keys, const QList<QVariant> & values);

  /// Merges another ValueHash in this one, while keeping the current
  /// order of the keys and appending the new ones.
  ///
  /// Elements in \a other override those already present,
  /// unless \a override is false.
  ///
  /// The final key order is: first the keys of this hash, then of @a
  /// other. Duplicates are removed, unless @a allowMultiple is true.
  void merge(const ValueHash & other, bool override = true,
             bool allowMultiple = false);

  /// Appends the @a value to the list of strings stored in @a key.
  void appendToList(const QString & key, const QString & value);

  /// @name Conversion functions
  ///
  /// These functions do not really act on a ValueHash, but they
  /// provide handy type-aware conversion between QVariant and other
  /// types
  ///
  /// @{

  /// Converts a variant to a string.
  ///
  /// @todo Here, the precision is hard-wired to 12 digits, which
  /// should be more than enough.
  static QString toString(const QVariant & value, bool * canConvert = NULL);

  /// Converts a QVariant into a Ruby object. Not all types are
  /// supported for now. Qnil is returned on unsupported values
  static mrb_value variantToRuby(const QVariant & variant);

  /// Converts a Ruby object to a QVariant. Not all types are
  /// supported for now. Qnil is returned on unsupported values
  static QVariant rubyToVariant(mrb_value value);

  /// The options that tune the conversion from text to QVariant.
  static QList<Argument *> variantConversionOptions();

  
  /// Converts a given text to a QVariant, taking into account the
  /// information given by the options from
  /// variantConversionOptions().
  static QVariant variantFromText(const QString & text,
                                  const CommandOptions & opts);
  
  /// @}

  /// Converts to a Ruby Hash.
  mrb_value toRuby() const;


  /// Returns the value of the meta-data as a double.
  ///
  /// Performs some type checking and conversions otherwise not
  /// supported by QVariant.
  double doubleValue(const QString & param, bool * ok) const;

  /// Overridden to disable quality control
  double doubleValue(const QString & param) const;

  /// Sets data from a Ruby hash
  void setFromRuby(mrb_value hsh);
  
  /// Builds a new ValueHash from a Ruby hsh
  static ValueHash fromRuby(mrb_value hsh);

  /// Returns a copy of the hash selecting keys/values  from the spec:
  /// @li '*' means "set all"
  /// @li 'txt' means set the meta txt to the value txt
  /// @li 'txt->txt2' means set the meta txt2 to the value txt
  ///
  ValueHash copyFromSpec(const QStringList & spec, QStringList * missing = NULL) const;

  /// @name Accessor-like function
  /// 
  /// A series of functions to help store/retrieve 
  /// @{

  /// Sets the value of the given key
  void setValue(const QString & key, const QVariant& value);

  /// Sets the target value from the hash, if the key exists and it
  /// can be converted to the desired type.
  template<class T> void getValue(const QString & key, T & dest) const;
  ///@}

  /// @name Output file and such related functions
  ///
  /// @{

  /// The options for handling the output file.  The optional argument
  /// is the default value for output (need to be provided to
  /// handleOutput too.
  static QList<Argument *> outputOptions(bool deflt = false);

  /// Returns true if one of the output options is specified.
  static bool hasOutputOptions(const CommandOptions & opts);

  /// Process the output of the hash to the output file, or other
  /// things like that.
  void handleOutput(const DataSet * ds, const CommandOptions & opts,
                    bool deflt = false) const;

  /// @}


  /// @name Writing and reading of meta-data files
  ///
  /// @{

  /// Reads the given meta-data file into the given value hash
  void fromJSON(const QString & json);

  /// Saves as a meta-data file
  QString toJSON() const;

  /// @}


};

template<class T> void ValueHash::getValue(const QString & key, T & dest) const
{
  if(! contains(key))
    return;
  const QVariant & v = value(key);
  if(! v.canConvert<T>())
    return;
  dest = v.value<T>();
}

#endif
