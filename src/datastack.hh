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
/// @todo Add the redo stack
class DataStack {

  /// The DataSet objects
  QList<DataSet *> dataSets;

  /// The only DataStack present in the program.
  ///
  /// @todo My code relies heavily on static variables, which may be
  /// just as well after all. I doubt I will ever need to run two
  /// instances of QSoas at the same time, but if I do, I'll need to
  /// factor all static variables into a well-known class, possibly
  /// MainWin ? and have it as additional parameter of \b all
  /// Command...
  static DataStack * theDataStack;

  /// A chache DataSet name -> DataSet.
  QHash<QString, DataSet *> dataSetByName;
public:

  /// Constructs a DataStack object.
  DataStack();

  ~DataStack();

  /// Returns the application-wide data stack.
  static DataStack * dataStack() {
    return theDataStack;
  };

  /// Push the given DataSet object to the stack.
  ///
  /// @warning DataStack takes ownership of the DataSet.
  void pushDataSet(DataSet * dataset);

  /// Displays to terminal a small text description of the contents of
  /// the stack.
  void showStackContents(bool mostRecentFirst = true) const;
  
};

#endif
