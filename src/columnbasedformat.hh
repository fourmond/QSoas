/**
   \file columnbasedformat.hh
   Reading/writing of column based files, with validation
   Copyright 2022 by CNRS/AMU

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
#ifndef __COLUMNBASEDFORMAT_HH
#define __COLUMNBASEDFORMAT_HH

class Vector;

/// This class provides a framework for serializing column-based data,
/// with validation.
///
/// It is not optimized for performance but for power and expressivity
///
/// The various columns are added via the addColumn() function.
class ColumnBasedFormat {
public:

  /// The type for reading the value to from the file
  typedef std::function<void (const QString & )> ReadFunction;

  /// The function for getting the string to be written
  typedef std::function<QString ()> WriteFunction;

protected:

  
  
  class Value {
  public:

    /// The name of the column
    QString name;

    /// Whether it is optional or not
    bool optional;

    /// The function to read it
    ReadFunction read;

    /// The function to write it
    WriteFunction write;

    Value(const QString & n, bool o, const ReadFunction & r,
          const WriteFunction & w);
    
  };


  /// The columns
  QList<Value> columns;

  /// A hash column name -> index in columns
  QHash<QString, int> indices;

public:

  /// Default precision for writing doubles
  int precision;

  ColumnBasedFormat();

  /// Adds a column
  ColumnBasedFormat & addColumn(const QString & name,
                                const ReadFunction & read,
                                const WriteFunction & write,
                                bool optional = false);

  /// Adds a double-value column based on a simple pointer. 
  ColumnBasedFormat & addNumberColumn(const QString & name,
                                      double * value,
                                      bool optional = false);

  /// Adds a string column based on a simple pointer. 
  ColumnBasedFormat & addStringColumn(const QString & name,
                                      QString * value,
                                      bool optional = false);

  /// Adds a date time column, serialized as ms since epoch
  ColumnBasedFormat & addTimeColumn(const QString & name,
                                    QDateTime * value,
                                    bool optional = false);

  /// Adds an integer column, 
  ColumnBasedFormat & addIntColumn(const QString & name,
                                   int * value,
                                   bool optional = false);

  /// Adds an integer column, 
  ColumnBasedFormat & addIntColumn(const QString & name,
                                   qint64 * value,
                                   bool optional = false);

  /// An element targetting a specific item in a Vector
  ColumnBasedFormat & addVectorItemColumn(const QString & name,
                                          Vector * value,
                                          int index,
                                          bool optional = false);


  /// Returns the headers of the columns
  QStringList headers() const;

  /// Returns the values returned by the write functions
  QStringList writeValues() const;

  /// Reads the values from the source by calling the read functions.
  ///
  /// It will raise an exception if one of the non-optional functions
  /// are missing
  ///
  /// The missing optional elements are listed in the missingOptional
  /// hash.
  void readValues(const QHash<QString, QString> & source,
                  QStringList * missingOptional = NULL) const;


  /// Does the same thing as the other function, assuming that the
  /// elements are in the order returned by headers()
  void readValues(const QStringList &source,
                  QStringList * missingOptional = NULL) const;

  /// Helper function to read a given number into a Vector, creating
  /// the missing elements if necessary.
  static void readIntoVector(Vector * target, int index,
                             const QString & value, double missing = 0);


  /// Parses a double from the string, returns a NaN value if invalid.
  static double parseDouble(const QString & value,
                            double defaultValue = std::numeric_limits<double>::quiet_NaN());

};

#endif
