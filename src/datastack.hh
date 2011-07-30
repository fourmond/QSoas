/**
   \file datastack.hh
   The DataStack class, that contains all data Soas knows about.
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

#ifndef __DATASTACK_HH
#define __DATASTACK_HH

#include <dataset.hh>

/// The data stack, ie all DataSet objects known to Soas.
///
/// Commands missing here:
/// @li undo/redo + redo stack
/// @li save/load stack (in a binary format containing
/// exactly everyting, see QDataStream)
class DataStack {

  /// The DataSet objects
  QList<DataSet *> dataSets;

  /// A chache DataSet name -> DataSet.
  QHash<QString, DataSet *> dataSetByName;
public:

  /// Constructs a DataStack object.
  DataStack();

  ~DataStack();


  /// Push the given DataSet object to the stack.
  ///
  /// @warning DataStack takes ownership of the DataSet.
  void pushDataSet(DataSet * dataset);

  /// Displays to terminal a small text description of the contents of
  /// the stack.
  void showStackContents(bool mostRecentFirst = true) const;
  
};

#endif
