/**
   \file synchronizedtables.hh
   A widget that displays several table views on top of each other
   Copyright 2023 by CNRS/AMU

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
#ifndef __SYNCHRONIZEDTABLES_HH
#define __SYNCHRONIZEDTABLES_HH

/// This widget holds several QTableView in several regions (separated
/// each by a splitter), and maintains them synchronized, i.e.:
/// @li same column sizes
/// @li same horizontal position (in fact, a global horizontal slider is used)
/// @li same hidden/visible state
class SynchronizedTables : public QSplitter {
  Q_OBJECT;

  /// The widgets being handled by the splitter
  QList<QWidget*> splitterWidgets;

  /// All the widgets
  QList<QWidget*> allWidgets;

  /// The VBoxes use
  QList<QVBoxLayout*> layouts;

  /// The displayed tables
  QList<QTableView*> tables;

public:
  explicit SynchronizedTables(QWidget * parent = NULL);

  /// Adds a widget to the given region, or a new region if @a region
  /// is -1. Returns the new region.
  int addWidget(QWidget * widget, int region = -1, int stretch = 0);

  /// Adds a table view to the given region
  int addTable(QTableView * table, int region = -1, int stretch = 0);

public slots:

  void hideColumn(int col);

  void showColumn(int col);

// protected:
//   void resizeEvent(QResizeEvent * event) override;

protected slots:
  void columnResized(int column, int oldSize, int newSize);

  void tableSliderChanged(int value);

};

#endif
