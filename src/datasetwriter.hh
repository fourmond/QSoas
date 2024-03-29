/**
   \file datasetwriter.hh
   A helper class for writing DataSet to files
   Copyright 2020 by CNRS/AMU

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
#ifndef __DATASETWRITER_HH
#define __DATASETWRITER_HH

class DataSet;
class Argument;
class File;
#include <argumentmarshaller.hh>

/// A helper class to write a dataset to a file.
///
/// This class will should handle various aspects of writing to a
/// file, such as:
/// @li the presence and format of meta-data
/// @li writing of column and/or row names
/// @li output separator
/// @li precision
/// @li surely something else ?
class DataSetWriter {

  /// Writes the dataset to the given device
  void writeData(QIODevice * device, const DataSet * dataset) const;
public:

  /// Whether to write row names or not
  bool writeRowNames;

  /// The separator
  QString separator;

  /// The prefix for comments
  QString commentPrefix;

  /// The prefix for column names
  QString columnNamesPrefix;

  /// A sprintf format for outputting the numbers
  QString format;

  /// Writes the dataset to the given File.
  void writeDataSet(File * file, const DataSet * dataset) const;

  /// Writes the metadata of the given dataset to the given file
  /// (adding the appropriate .qsm extension)
  void writeDataSetMeta(const QString & file, const DataSet * dataset) const;

  

  DataSetWriter();


  /// Options for writing.
  static QList<Argument *> writeOptions();

  /// Sets the writing parameters from the given options.
  void setFromOptions(const CommandOptions & opts);
};

#endif
