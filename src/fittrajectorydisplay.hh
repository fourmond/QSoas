/**
   \file fittrajectorydisplay.hh
   Dialog box handling fits in QSoas
   Copyright 2012 by Vincent Fourmond

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

  FitTrajectory() {
  };

  FitTrajectory(const Vector & init, const Vector & final,
                const Vector & errors, 
                double res, double rr) :
    initialParameters(init), finalParameters(final), 
    parameterErrors(errors),
    ending(Converged), residuals(res), relativeResiduals(rr) {
  };

};

/// A dialog box displaying a set of FitTrajectory objects
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


  /// Setup the insides of the dialog box.
  void setupFrame();
public:

  FitTrajectoryDisplay(FitDialog * dlg, FitData * data, 
                       QList<FitTrajectory> * trajectories);
  ~FitTrajectoryDisplay();

public slots:

  /// Update the display.
  void update();

  /// Export to TAB-separated data file
  void exportToFile();

protected slots:

  void contextMenuOnTable(const QPoint & pos);
};



#endif
