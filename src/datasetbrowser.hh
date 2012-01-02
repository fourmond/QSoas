/**
   \file datasetbrowser.hh
   Dialog box handling fits in QSoas
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

#ifndef __DATASETBROWSER_HH
#define __DATASETBROWSER_HH

#include <dataset.hh>
#include <curveview.hh>


/// This class handles all the user interaction during fits.
class DatasetBrowser : public QDialog {

  Q_OBJECT;

private:

  /// The dataset views
  QList<CurveView *> views;

  /// The current index
  int currentIndex;

  /// The currently displayed datasets
  QList<const DataSet *> datasets;

  /// Current number of columns
  int width;

  /// Current number of rows
  int height;

  /// Base index
  int index;

  /// The grid layout in use to display view
  QGridLayout * grid;

  /// Display of the current buffers
  QLabel * bufferDisplay;

  void setupFrame();
  void setupGrid();

  void cleanupViews();

public:
  DatasetBrowser();
  ~DatasetBrowser();

signals:
  void currentDataSetChanged(int ds);

public:
  void displayDataSets(const QList<const DataSet *> &ds);
  void displayDataSets(const QList<DataSet *> &ds);

protected slots:

  void dataSetChanged(int newds);
  void nextPage();
  void previousPage();

};

#endif
