/**
   \file fittrajectorydisplay.hh
   Displays of fit trajectories display.
   Copyright 2013, 2014, 2015, 2016, 2017, 2018, 2023 by CNRS/AMU

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
class FitTrajectory;

/// A class to display a series of param = f(perpendicular coordinate)
/// for several trajectories.
class TrajectoryParametersDisplay : public QWidget {
  Q_OBJECT;
  Vector perpendicularCoordinates;
  Vector zero;

  FitWorkspace * workspace;

  /// The view
  CurveView * view;

  /// The right side, the parameters checkboxes
  QGridLayout * parametersLayout;

  /// The list of displayed trajectories. The bool argument denotes
  /// whether we are displaying the initial (false) or final (true)
  /// result.
  QList<QPair<const FitTrajectory *, bool> > trajectories;

  /// Adds a trajectory to the display
  void setupTrajectory(int index, const FitTrajectory * traj, bool isFinal);

  /// The main check boxes for parameters
  QList<QCheckBox *> parameters;

  /// The corresponding list of displays
  QList<QList<TuneableDataDisplay *> > parameterDisplays;

  QList<QList<gsl_vector_view> > views;
  
public:

  explicit TrajectoryParametersDisplay(FitWorkspace * workspace);

public slots:

  void displayRows(const QSet<int> & columns);
};

class TrajectoriesModel;
class FlaggedTrajectoriesModel;
class DoubleDisplay;

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


  /// The parameters display in the first tab
  TrajectoryParametersDisplay * graphicalDisplay;


  /// The list of checkboxes for the parameters display as a function
  /// of
  QList<TuneableDataDisplay *> parametersDisplays;

  /// The view (in the second tab)
  CurveView * view;

  

  /// The underlying item model
  TrajectoriesModel * model;

  DoubleDisplay * doubleDisplay;

  /// The model for displaying the flags
  FlaggedTrajectoriesModel * flagsModel;

  /// The table view for displaying the flags/clusters
  QTableView * flagsView;


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

  QList<QAction *> contextActions;

  void addCMAction(const QString & name, QObject * receiver, 
                   const char * slot, 
                   const QKeySequence & shortCut = QKeySequence());

  void addCMSeparator();


public:

  explicit FitTrajectoryDisplay(FitWorkspace * workspace);
  ~FitTrajectoryDisplay();

  /// Brings a dialog box to browse the trajectories of the given
  /// workspace. Defaults to the default fit workspace.
  static void browseTrajectories(FitWorkspace * ws = NULL);


public slots:

  /// Update the display.
  void update();

  /// Removes all trajectories above a given threshold.
  void trim();

  /// Removes all trajectories whose final residuals are more than \a
  /// threshold times the lowest (or 1 + \a threshold if \a threshold
  /// is less than 1)
  void trim(double threshold);


  void nextBuffer();

  void previousBuffer();

  /// Hide columns that are fixed
  void hideFixed();

  /// Hide selected
  void hideSelected();

  // /// Hide selected
  // void hideSelectedEverywhere();

  /// Hide all (from dataset) but selected
  void hideButSelected();

  /// Hide all (from dataset) but selected
  void hideButSelectedEverywhere();

  
  /// Show all columns
  void showAll();

  /// Sets the current reference trajectory
  void setAsReference();

  /// Clears the current trajectory
  void clearReference();


  /// Sorts according to the current column
  void sortByCurrentColumn();

  /// (reverse) sortByCurrentColumn()
  void reverseSortByCurrentColumn();

protected:

  /// Returns the list of the indices of the currently selected
  /// trajectories. Sorted.
  QList<int> selectedTrajectories() const;

  void updateModels();


protected slots:


  void contextMenuOnTable(const QPoint & pos);

  
  void flagsViewContextMenu(const QPoint & pos);

  /// Send the currently selected parameters to the FitWorkspace
  void reuseCurrentParameters();

  /// Reuse the current flagged parameters
  void reuseFlaggedParameters(const QString & flag,
                              bool best);

  /// Send the currently selected parameters for the current dataset
  /// to the FitWorkspace.
  void reuseParametersForThisDataset();

  /// Deletes the selected parameters
  void deleteSelectedTrajectories();

  /// Called when the selection has changed
  void onSelectionChanged();

  /// Called whenever the user double clicks on an item
  void onDoubleClick(const QModelIndex & index);

  /// Asks for a new tag name and add it
  void promptAddFlag();

  /// Adds a given flag to the selected trajectories
  void addFlagToSelected(const QString & flag);

  /// Removes a given flag to the selected trajectories
  void removeFlagToSelected(const QString & flag);
};



#endif
