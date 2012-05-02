/**
   \file fitdialog.hh
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

#ifndef __FITDIALOG_HH
#define __FITDIALOG_HH

#include <fitparameters.hh>

class FitData;
class CurveView;
class FitParameterEditor;
class DataSet;


/// This class handles all the user interaction during fits.
class FitDialog : public QDialog {

  Q_OBJECT;

  void setupFrame();

  /// The FitData object we'll populate and run
  FitData * data;

  /// The stacked widgets holding the buffer view;
  QStackedWidget * stackedViews;

  /// The dataset views
  QList<CurveView *> views;

  /// The combo box for toogling between the various buffers
  QComboBox * bufferSelection;

  /// Label displaying the current buffer number
  QLabel * bufferNumber;

  /// The line editor for entering the weight of the buffer, set to
  /// NULL if that has been opted out.
  QLineEdit * bufferWeightEditor;

  /// List of editors
  QList<FitParameterEditor *> editors;

  /// The parameters
  FitParameters parameters;

  /// The current index
  int currentIndex;

  /// A flag to avoid updating twice...
  bool settingEditors;

  /// A small text display to report current progress
  QLabel * progressReport;

  /// the start button
  QPushButton * startButton;

  /// The cancel button, only visible during fit
  QPushButton * cancelButton;

  /// Fills the FitData with parameter information
  void setDataParameters();

  DataSet * simulatedData(int i);

  /// Whether or not we should cancel the current fit.
  bool shouldCancelFit;

protected:

  virtual void closeEvent(QCloseEvent * event);


public:
  FitDialog(FitData * data, bool displayWeights = false);
  ~FitDialog();

signals:
  void currentDataSetChanged(int ds);

public slots:

  /// Loads parameters from the given file.
  void loadParametersFile(const QString & fileName);

  /// Add all simulated datasets to the data stack
  void pushSimulatedCurves();

protected slots:

  void dataSetChanged(int newds);

  void weightEdited(const QString & str);

  /// Compute the new curves based on initial guesses (or
  /// hand-modified versions)
  void compute();

  /// Update all the editors
  void updateEditors();

  /// Starts the fit !
  void startFit();

  /// Cancels the current fit !
  void cancelFit();

  /// Jumps to previous dataset
  void previousDataset();

  /// Jumps to next dataset
  void nextDataset();

  /// Adds current simulated datset to the data stack
  void pushCurrentCurve();

  /// Saves all simulated data to files
  void saveSimulatedCurves();

  /// Loads parameters from files
  void loadParameters();

  /// Saves parameters to a file
  void saveParameters();

  /// Exports parameters to a file, losing information about their
  /// state (global and/or fixed)
  void exportParameters();

  /// Exports parameters to the current output file
  void exportToOutFile();

  /// Exports parameters to the current output file (with errors)
  void exportToOutFileWithErrors();

  /// Resets all the parameters data to the initial guess
  void resetAllToInitialGuess();

  /// Resets the currently displayed dataset parameters to initial
  /// guess.
  void resetThisToInitialGuess();

  /// Saves all the views as PDF
  void saveAllPDF();

  /// Shows the covariance matrix
  void showCovarianceMatrix();

  /// Edit all parameters in a dialog box
  void editParameters();

};

#endif
