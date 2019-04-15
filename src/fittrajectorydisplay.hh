/**
   \file fittrajectorydisplay.hh
   Displays of fit trajectories display.
   Copyright 2013, 2014, 2015, 2016, 2017, 2018 by CNRS/AMU

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
#ifndef __FITTRAJECTORYDISPLAY_HH
#define __FITTRAJECTORYDISPLAY_HH

#include <fittrajectory.hh>
#include <vector.hh>

class FitDialog;
class FitData;
class CurveView;
class TuneableDataDisplay;

class TrajectoriesModel;

/// A dialog box displaying a set of FitTrajectory objects.
///
class FitTrajectoryDisplay : public QDialog {
  Q_OBJECT;

  /// The workspace whose trajectories we display
  FitWorkspace * workspace;

  /// The underlying fit data...
  const FitData * fitData;


  /// The main tabs representing the trajectories.
  QTabWidget * tabs;

  /// The main display of the parameters
  QTableView * parametersDisplay;


  /// The list of checkboxes for the parameters.
  QList<TuneableDataDisplay *> parametersDisplays;

  /// The view !
  CurveView * view;

  

  /// The underlying item model
  TrajectoriesModel * model;


  /// The heads of the display
  QStringList heads;



  /// The global vertical layout
  QVBoxLayout * overallLayout;

  /// Setup the insides of the dialog box.
  void setupFrame();

  /// Whether iterations did start
  bool iterationsStarted;

  /// Whether we should stop iterations
  bool shouldStop;

  /// Whether we have done at least one iteration
  bool doneOne;

public:

  FitTrajectoryDisplay(FitWorkspace * workspace);
  ~FitTrajectoryDisplay();


public slots:

  /// Update the display.
  void update();

  /// Export to TAB-separated data file
  void exportToFile();

  /// Removes all trajectories above a given threshold.
  void trim();

  /// Removes all trajectories whose final residuals are more than \a
  /// threshold times the lowest (or 1 + \a threshold if \a threshold
  /// is less than 1)
  void trim(double threshold);


  /// Import from TAB-separated data file
  void importFromFile(const QString & file);

  /// Imports from a file; prompts for the name !
  void importFromFile();

  /// Sort with the smallest residuals first
  void sortByResiduals();

  /// Attempts to cluster the trajectories
  void clusterTrajectories();

  void nextBuffer();

  void previousBuffer();

  /// Hide columns that are fixed
  void hideFixed();

  /// Show all columns
  void showAll();

  /// Sets the current reference trajectory
  void setAsReference();

  /// Clears the current trajectory
  void clearReference();

protected slots:

  void contextMenuOnTable(const QPoint & pos);

  /// Send the currently selected parameters to the FitWorkspace
  void reuseCurrentParameters();

  /// Send the currently selected parameters for the current dataset
  /// to the FitWorkspace.
  void reuseParametersForThisDataset();

  /// Deletes the currently selected parameters
  void deleteCurrentParameters();
};



#endif
