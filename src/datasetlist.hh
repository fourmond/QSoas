/**
   \file datasetlist.hh
   A helper class for parsing (options) and handling several datasets
   Copyright 2019 by CNRS/AMU

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
#ifndef __DATASETLIST_HH
#define __DATASETLIST_HH

#include <argumentmarshaller.hh>

class DataSet;
class Argument;

/// This helper class handles collecting several datasets from the
/// stack through the /buffers and /for-which options, defaulting to
/// the current buffer if none is found
class DataSetList {

  QList<const DataSet *> datasets;

  /// This is when there was no current dataset
  bool noDataSet;

  
public:
  /// Creates and parses a DataSetList from the options.
  /// If nothing is specified, the list defaults to either:
  /// @li the current buffer when @a all is false
  /// @li all the stack when @a all is true
  DataSetList(const CommandOptions & opts, bool all = false);
  ~DataSetList();

  /// These so we can use a for loop.
  QList<const DataSet *>::const_iterator begin() const;
  QList<const DataSet *>::const_iterator end() const;

  /// Returns true when there were specified datasets, either
  /// implicitly because there is a current dataset, or through the
  /// selection mechanism. The list can be empty nevertheless.
  bool dataSetsSpecified() const;

  /// Returns the number
  int size() const;

  operator const QList<const DataSet *> &() const;

  /// Returns the static list of options.
  /// The @a txt argument is the help text.
  static QList<Argument *> listOptions(const QString & txt,
                                       bool defaultOptions = true);
};


#endif
