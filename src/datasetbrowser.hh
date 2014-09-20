/**
   \file datasetbrowser.hh
   Dialog box handling fits in QSoas
   Copyright 2012, 2013 by CNRS/AMU

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
#ifndef __DATASETBROWSER_HH
#define __DATASETBROWSER_HH

#include <dataset.hh>
#include <curveview.hh>

class CheckableWidget;
class NupWidget;

/// This class handles all the user interaction during fits.
class DatasetBrowser : public QDialog {

  Q_OBJECT;

private:

  /// The widget holding the whole nup stuff
  NupWidget * nup;

  /// The dataset views
  QList<CheckableWidget *> views;

  /// The currently displayed datasets
  QList<const DataSet *> datasets;

  /// Display of the current buffers
  QLabel * bufferDisplay;

  /// THe horizontal bottom layout
  QHBoxLayout * bottomLayout;

  void setupFrame();
  void setupGrid();

  void cleanupViews();

  /// Whether or not to use extended selection capacities
  bool extendedSelection;

public:
  DatasetBrowser();
  ~DatasetBrowser();

signals:
  void currentDataSetChanged(int ds);

public:
  void displayDataSets(const QList<const DataSet *> &ds, 
                       bool extendedSelection = true);
  void displayDataSets(const QList<DataSet *> &ds, 
                       bool extendedSelection = true);

  /// Returns the list of currently selected datasets.
  QList<const DataSet*> selectedDatasets() const;

protected slots:

  void pageChanged(int newpage);

  /// Runs the numbered hook
  void runHook(int hook);
  
public:
  /// @name Actions
  ///
  /// Functions to setup various hooks to perform actions on the
  /// selected/displayed datasets.
  ///
  /// @{

  typedef void (*ActOnSelected)(const QList<const DataSet*> &);

  /// Adds a button with the given hook.
  void addButton(QString name, ActOnSelected hook);

protected:

  /// The Qt side of the mapping between a button and a hook.
  QSignalMapper * actionsMapper;

  /// The object side of the mapper, ie the list of hooks.
  ///
  /// @todo This won't scale well at all when using multiple types for
  /// hooks. A real virtual system would be much better, or using
  /// QObject children for each type and bypassing the QSignalMapper
  /// altogether
  QList<ActOnSelected> actionHooks;

  /// @}

};

#endif
