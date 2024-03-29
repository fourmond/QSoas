/**
   \file datastackhelper.hh
   A helper class to add many datasets onto the stack (with style and such)
   Copyright 2016 by CNRS/AMU

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
#ifndef __DATASTACKHELPER_HH
#define __DATASTACKHELPER_HH

#include <argumentmarshaller.hh>

class DataSet;
class StyleGenerator;
class Argument;

/// This helper class handles pushing several datasets from a single
/// command onto the stack, and manages:
/// \li parsing dataset-pushing-related options
/// \li addition of several datasets, so that they are all displayed at once
/// \li flagging added datasets
/// \li style
///
///
/// @todo Would it make sense to use it for V/v (for v, yes, for V,
/// no, because the flagging doesn't make sense)?
///
/// The actual display of the datasets and the flagging happen at a
/// call to flush(). This is done automatically by the destructor,
/// and it should be OK, because the datastack pushing does not raise
/// exceptions. But you can do it manually, though.
class DataStackHelper {

  /// Flags for newly-created buffers
  QStringList flags;

  /// Whether pushing is immediate or deferred to the destruction of
  /// the object (probably the best choice).
  bool deferred;

  /// The style for the style-generator
  QString style;

  /// The list of datasets to be added.
  ///
  /// Always empty when deferred is false.
  QList<DataSet *> datasets;

  /// Whether we have clear or only update. Necessarily true after the
  /// FIRST dataset is pushed.
  bool update;

  /// Push one dataset with the given style (can be NULL).
  void pushOne(DataSet * ds, StyleGenerator * style);

  /// Meta-data values to be added to the newly-pushed data
  QHash<QString, QVariant> meta;

  /// Whether we push in reverse
  bool reversed;

  /// If not valid, this means we destroy datasets on exit, not push
  bool valid;

  
public:

  enum HelperFeature {
    SetMeta = 0x01,
    Flags = 0x02,
    Style = 0x04,
    Reversed = 0x08,
    /// Combined
    None = 0x00,
    All =  SetMeta|Flags|Style|Reversed
  };

  Q_DECLARE_FLAGS(HelperFeatures,HelperFeature);


  
  explicit DataStackHelper(const CommandOptions & opts, bool update = false,
                           bool deferred = true, HelperFeatures features = All);
  ~DataStackHelper();

  /// Invalidate the helper. It still accepts datasets, but will
  /// destroy them rather than pushing them on destruction, unless
  /// validate() is used. 
  void invalidate();

  /// Cancels the effect of invalidate()
  void validate();

  /// Returns the list of datasets currently held.
  const QList<DataSet *> & currentDataSets() const;

  /// Pushes one dataset to the stack, whether directly, or on the
  /// to-do list for when DataStackHelper os destroyed.
  void pushDataSet(DataSet * ds);

  /// Pushes several datasets.
  void pushDataSets(const QList<DataSet *> & dss);

  /// Runs pushDataSet() on the given dataset. Can be chained.
  DataStackHelper & operator <<(DataSet * ds);

  /// You
  void flush();

  /// Returns the static list of options
  static QList<Argument *> helperOptions(HelperFeatures features = All);
};


#endif
