/**
   \file datasetoptions.hh
   A class holding display options for datasets
   Copyright 2013, 2014 by CNRS/AMU

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

#include <argumentlist.hh>

class DataSet;
class ArgumentList;

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
  /// error. Errors are assumed to be symmetric. This value is always
  /// positive.
  double yError(const DataSet * ds, int idx) const;

  /// Sets the columns for Y errors
  void setYErrors(int col);

  /// Sets the error value for the given dataset. Will throw if not
  /// possible.
  void setYError(DataSet * ds, int idx, double value) const;


  /// If on, the dataset should be shown as histograms.
  bool histogram;

  /// If strictly positive, it is used to determine if one should draw
  /// lines or just markers: if the averaged abs delta is larger than
  /// that many times the predicted one, then one disables the drawing
  /// of lines, unless specifically requested.
  int jagThreshold;


  DatasetOptions();

  /// Returns true if the dataset should be considered "jaggy"
  bool isJaggy(const DataSet * ds) const;

  /// Whether one should draw markers or not.
  bool shouldDrawMarkers(const DataSet * ds) const;

  /// Whether one should draw lines or not
  bool shouldDrawLines(const DataSet * ds) const;


  /// Returns an option list whose parsing can be fed to
  /// setDatasetOptions()
  static ArgumentList optionList();


  /// Sets the target dataset options from the (user-supplied)
  /// CommandOptions given.
  static void setDatasetOptions(DataSet * ds, 
                                const CommandOptions & opts);
};


QDataStream & operator<<(QDataStream & out, const DatasetOptions & ds);
QDataStream & operator>>(QDataStream & in, DatasetOptions & ds);

#endif
