/**
   \file parametersspreadsheet.hh
   Display of fit parameters along the perpendicular coordinate
   Copyright 2014 by CNRS/AMU

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
#ifndef __PARAMETERSSPREADSHEET_HH
#define __PARAMETERSSPREADSHEET_HH

#include <dataset.hh>
#include <curveview.hh>


class FitWorkspace;
class ParametersItemModel;

/// This class displays
class ParametersSpreadsheet : public QDialog {

  Q_OBJECT;

private:

  /// The workspace
  FitWorkspace * workspace;

  /// The model
  ParametersItemModel * model;

  /// The view of the parameters.
  QTableView * view;

  void setupFrame();

  QList<QAction *> contextActions;

  void addCMAction(const QString & name, QObject * receiver, 
                 const char * slot, 
                 const QKeySequence & shortCut = QKeySequence());



public:
  ParametersSpreadsheet(FitWorkspace * params);
  ~ParametersSpreadsheet();


  /// This is true if during the course of the edition, the data
  /// changed. Then one updates the editors...
  bool dataChanged() const;

protected slots:

  /// Shows a context menu to modify a few things...
  void spawnContextMenu(const QPoint & pos);

  /// Spawns a local editor for the closest? item and use the
  /// resulting to set 
  void editSelected();

  /// Propagates the values of the parameters at the top down
  void propagateDown();

  /// Interpolate the parameters between beginning and end (using
  /// column number values).
  void interpolateParameters();

  /// Fix selected parameters
  void fixParameters(bool fix = true);

  /// Unfix parameters
  void unfixParameters();

  /// Reset parameter values to initial guess
  void resetParameters();

};

#endif
