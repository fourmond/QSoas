/**
   \file datasetoptions.hh
   A class holding display options for datasets
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
#ifndef __DATASETOPTIONS_HH
#define __DATASETOPTIONS_HH

class DataSet;

/// This class holds a whole lot of indications on a dataset about
/// what are the roles of the different columsn, and other various
/// options about how it should be displayed.
///
/// (planned/implemented) roles for this class
/// @li error bars (including their use as fit error bars)
/// @li 3D display
///
/// It doesn't sound too satisfactory that this class is mixing both
/// meaningful information (such as how errors are stored) with
/// display-only information (such as how they should be displayed ?)
/// (although this isn't the case for now yet)
///
/// @todo Implement error types (ie, relative, absolute, log
/// (whathever that means) ?)
class DatasetOptions {

  friend QDataStream & operator<<(QDataStream & out, const DatasetOptions & ds);
  friend QDataStream & operator>>(QDataStream & in, DatasetOptions & ds);

  /// The column for the Y error bars -- or a negative number should
  /// there be no errors.
  int yErrors;

public:

  /// Whether or not the options have errors for Y points.
  bool hasYErrors() const;

  /// Whether or not the given dataset would have error bar for Y
  /// points (takes into account whether the target has the necessary
  /// columns)
  bool hasYErrors(const DataSet * ds) const;

  /// Returns the error on the given Y point, or 0 if there isn't any
  /// error. Errors are assumed to be symmetric.
  double yError(const DataSet * ds, int idx) const;

  /// Sets the columns for Y errors
  void setYErrors(int col);


  DatasetOptions();
};


QDataStream & operator<<(QDataStream & out, const DatasetOptions & ds);
QDataStream & operator>>(QDataStream & in, DatasetOptions & ds);

#endif
