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

  /// The dialog on which that one is based...
  FitDialog * fitDialog;

  /// The underlying fit data...
  FitData * fitData;


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


  /// The list of trajectories
  ///
  /// (I'm unsure why this ends up being a pointer, but it doesn't
  /// look too bad this way).
  QList<FitTrajectory> * trajectories;


  /// The heads of the display
  QStringList heads;

  /// Number of repetitions
  QLineEdit * repetitions;


  /// Max number of iterations, forwarded to FitDialog
  QLineEdit * maxIterations;


  /// Max number of iterations for the subfit
  QLineEdit * maxSubfitIterations;

  /// Whether we did the prefit
  bool prefitDone;


  /// Show start and stop
  QPushButton * startStopButton;

  /// Iteration counter
  QLabel * iterationDisplay;
 
  


  /// A scroll area holding all the parameters for the automatic
  /// screening procedure.
  ///
  /// to be used later ?
  QScrollArea * explorationControls;

  /// The grid layout used for the exploration controls
  QGridLayout * explorationLayout;


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

  FitTrajectoryDisplay(FitDialog * dlg, FitData * data, 
                       QList<FitTrajectory> * trajectories);
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

protected slots:

  void contextMenuOnTable(const QPoint & pos);
};



#endif
