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
class DataSetTableModel;

/// A dialog box displaying/editing the contents of the dataset in
/// table form.
class DatasetEditor : public QDialog {

  Q_OBJECT;

private:

  DataSetTableModel * model;

  /// The original dataset, not modified
  const DataSet * source;

  /// Setup the frame
  void setupFrame();

  /// The table widget
  QTableView * table;

  /// An editor for meta-data
  ValueHashEditor * metaEditor;


public:
  explicit DatasetEditor(const DataSet * ds);
  ~DatasetEditor();

public slots:
  /// Push a new dataset to the stack
  void pushToStack();

protected slots:
  void contextMenuOnTable(const QPoint& point);
  
};

#endif
