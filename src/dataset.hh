/**
   \file dataset.hh
   The DataSet class, representing a data set (most of the time one data file)
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

#ifndef __DATASET_HH
#define __DATASET_HH

#include <vector.hh>

/// A class representing a data file.
///
/// A DataSet represents a single data set, ie most of the time the
/// data contained in a single file. It contains a X column, a Y
/// column, and possibly additional columns whose use is left for
/// later.
///
/// @todo Add support for meta-data.
class DataSet {

  /// The columns
  QList<Vector> columns;
public:

  DataSet() {;};
  DataSet(const QList<Vector> & cols) : columns(cols) {;};

  /// Adds a new column to the data set.
  DataSet & operator<<(const Vector & column);

  Vector & x() {
    return columns[0];
  };

  const Vector & x() const{
    return columns[0];
  };

  Vector & y() {
    return columns[1];
  };

  const Vector & y() const{
    return columns[1];
  };

  /// Dump the data to standard output.
  ///
  /// @todo This may grow into something interesting later on, but not
  /// now.
  void dump() const;

};

#endif
