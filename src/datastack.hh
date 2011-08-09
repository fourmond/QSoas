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
/// @li save/load stack (in a binary format containing
/// exactly everyting, see QDataStream)
class DataStack : public QObject {

  Q_OBJECT;

  /// The DataSet objects
  QList<DataSet *> dataSets;

  /// The redo stack.
  QList<DataSet *> redoStack;

  /// A chache DataSet name -> DataSet.
  QHash<QString, DataSet *> dataSetByName;
public:

  /// Constructs a DataStack object.
  DataStack();

  ~DataStack();


  /// Push the given DataSet object to the stack. It becoms the
  /// currentDataSet().
  ///
  /// @warning DataStack takes ownership of the DataSet.
  ///
  /// if \a silent is true, the stack won't emit the
  /// currentBufferChanged() signal (useful for the functions that
  /// already take care of the display)
  void pushDataSet(DataSet * dataset, bool silent = false);

  /// Displays to terminal a small text description of the contents of
  /// the stack.
  void showStackContents(bool mostRecentFirst = true) const;


  /// Returns the numbered data set.
  /// \li 0 is the most recent
  /// \li -1 is the most recently pushed onto the redo stack.
  ///
  /// Returns NULL if no dataset could be found.
  DataSet * numberedDataSet(int nb) const;

  /// Returns the current dataset, ie the one that should be displayed
  /// currently.
  ///
  /// If silent is false, an exception will be raised if the buffer is
  /// NULL.
  DataSet * currentDataSet(bool silent = true) const;

  /// Returns the DataSet specified by the given string, ie:
  /// \li a string representing a number
  /// \li a buffer name (not implemented yet)
  DataSet * fromText(const QString & str) const;

  /// Undo (ie, buffer 0 becomes buffer -1)
  void undo(int nbtimes = 1);

  /// Clears the whole stack, freeing the memory held
  void clear();

  /// Does the reverse of undo();
  void redo(int nbtimes = 1);

signals:
  /// Emitted whenever the current dataset changed.
  void currentDataSetChanged();

};

#endif
