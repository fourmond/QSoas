/**
   \file dataseteditor.hh
   Copyright 2013 by CNRS/AMU

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
#ifndef __DATASETEDITOR_HH
#define __DATASETEDITOR_HH

class DataSet;
class ValueHashEditor;

/// A dialog box displaying/editing the contents of the dataset in
/// table form.
class DatasetEditor : public QDialog {

  Q_OBJECT;

private:

  /// Whether the buffer has changed at all
  bool hasChanged;

  /// The original dataset, not modified
  const DataSet * source;

  /// Setup the frame
  void setupFrame();

  /// Setup the target table to display the contents of the given
  /// dataset
  static void setupTable(QTableWidget * t, const DataSet * ds);

  /// The table widget
  QTableWidget * table;

  /// An editor for meta-data
  ValueHashEditor * metaEditor;


public:
  explicit DatasetEditor(const DataSet * ds);
  ~DatasetEditor();

public slots:
  /// Push a new dataset to the stack
  void pushToStack();


protected slots:
  /// Called whenever an item changes.
  void itemEdited(QTableWidgetItem * it);

  
};

#endif
