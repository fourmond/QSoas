/**
   \file fittrajectorydisplay.hh
   Displays of fit trajectories display.
   Copyright 2013 by Vincent Fourmond

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

#include <vector.hh>

class FitDialog;
class FitData;

/// This class permits edition of the range in which a parameter is
/// expected to be found.
class ParameterRangeEditor : public QObject {
  
  Q_OBJECT;

  /// Shows/hides all the editors
  void showEditors(bool show = true);
  
public:
  /// Whether the parameter varies at all (and its name)
  QPointer<QCheckBox> variable;

  /// Label for the lower range
  QPointer<QLabel> lowLabel;

  /// Lower range
  QPointer<QLineEdit> lowerRange;

  /// Label for the upper range
  QPointer<QLabel> upLabel;

  /// Upper range
  QPointer<QLineEdit> upperRange;

  /// Is it log ?
  QPointer<QCheckBox> isLog;

  /// Parameter index
  int index;

  /// Dataset index
  int dataset;

  /// Inserts all the widgets present within the editor inside the grid.
  void insertIntoGrid(QGridLayout * grid, int row, int baseColumn = 0);

  bool isVariable() const {
    return variable->checkState() == Qt::Checked;
  };

  /// The parent parameter is important to avoid spurious widgets
  /// showing up unwanted.
  ParameterRangeEditor(const QString & name, int idx, int ds, 
                       bool fixed, double val, QWidget * parent);

  virtual ~ParameterRangeEditor();

  /// Returns the value corresponding to alpha (a value between 0 and 1)
  double value(double alpha);
  

public slots:

  /// Called when the variable status changed
  void variableChanged();

};

/// This class represents a "fit operation", ie what happens every
/// time the user clicks on the fit button.
///
/// @todo Add storage of integers for indexing ?
class FitTrajectory {
public:


  /// The initial parameters
  Vector initialParameters;

  /// The final parameters
  Vector finalParameters;

  /// The errors on the final parameters
  Vector parameterErrors;

  typedef enum {
    Converged,
    Cancelled,
    TimeOut,
    Error
  } Ending;

  /// How the fit ended.
  Ending ending;

  /// The residuals of the final parameters
  double residuals;

  /// The (relative) residuals
  double relativeResiduals;

  /// The fit engine
  QString engine;

  FitTrajectory() {
  };

  FitTrajectory(const Vector & init, const Vector & final,
                const Vector & errors, 
                double res, double rr,
                const QString & eng) :
    initialParameters(init), finalParameters(final), 
    parameterErrors(errors),
    ending(Converged), residuals(res), relativeResiduals(rr),
    engine(eng) {
  };

  /// Comparison by residuals.
  bool operator<(const FitTrajectory & o) const {
    return relativeResiduals < o.relativeResiduals;
  };

};

/// A dialog box displaying a set of FitTrajectory objects.
///
/// It also provides means to run automatic screening of the parameter
/// space, based on different stategies.
class FitTrajectoryDisplay : public QDialog {
  Q_OBJECT;

  /// The dialog on which that one is based...
  FitDialog * fitDialog;

  /// The underlying fit data...
  FitData * fitData;

  /// The main display of the parameters
  QTableWidget * parametersDisplay;


  /// The list of trajectories
  ///
  /// (I'm unsure why this ends up being a pointer, but it doesn't
  /// look too bad this way).
  QList<FitTrajectory> * trajectories;


  /// The heads of the display
  QStringList heads;

  /// Number of repetitions
  QLineEdit * repetitions;

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

  /// The list of editors of parameter ranges
  QList<ParameterRangeEditor *> parameterRangeEditors;

  /// Setup the insides of the dialog box.
  void setupFrame();

  /// Setup the part that actually deals with the range editors and the rest
  void setupExploration();

  int currentIteration;

  int lastIteration;
public:

  FitTrajectoryDisplay(FitDialog * dlg, FitData * data, 
                       QList<FitTrajectory> * trajectories);
  ~FitTrajectoryDisplay();

public slots:

  /// Update the display.
  void update();

  /// Export to TAB-separated data file
  void exportToFile();

  /// Sort with the smallest residuals first
  void sortByResiduals();

protected slots:

  void contextMenuOnTable(const QPoint & pos);

  void startStopExploration();

  void stopExploration();

  void startExploration();

  void sendNextParameters();
};



#endif
