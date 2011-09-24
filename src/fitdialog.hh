/**
   \file fitdialog.hh
   Main window for QSoas
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
class DataSet;
class ParameterDefinition;

/// A widget to edit the settings for a given parameter.
///
/// @todo Disable the global flag when there is only one dataset or
/// that the parameter can't be buffer-local.
class FitParameterEditor : public QWidget {
  Q_OBJECT;
  int index;
  const ParameterDefinition * def;

  /// The editor
  QLineEdit * editor;
  
  /// The global checkbox
  QCheckBox * global;

  /// The fixed checkbox
  QCheckBox * fixed;

public:
  FitParameterEditor(const ParameterDefinition * d, int idx, 
                     int totalDatasets, int totalParams);

  /// Set the editor values.
  void setValues(double value, bool fixed, bool global);

  /// Whether the parameter is global
  bool isGlobal() const {
    return global->checkState() == Qt::Checked;
  };

  /// Whether the parameter is fixed
  bool isFixed() const {
    return fixed->checkState() == Qt::Checked;
  };

signals:
  void fixedChanged(int index, bool fixed);
  void globalChanged(int index, bool global);
  void valueChanged(int index, double value);

protected slots:
  void onFixedClicked();
  void onGlobalClicked();
  void onValueChanged(const QString & str);
};


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
  FitDialog(FitData * data);
  ~FitDialog();

protected slots:

  void dataSetChanged(int newds);

  /// Compute the new curves based on initial guesses (or
  /// hand-modified versions)
  void compute();

  /// Update all the editors
  void updateEditors();

  /// Called when one of the global flags are clicked.
  void onSetGlobal(int index);

  /// Called whenever a fixed checkbox is clicked...
  void onSetFixed(int index);

  /// Called whenever a value gets updated.
  void onSetValue(int index, double value);

  /// Starts the fit !
  void startFit();

  /// Cancels the current fit !
  void cancelFit();

  /// Jumps to previous dataset
  void previousDataset();

  /// Jumps to next dataset
  void nextDataset();

  /// Add all simulated datasets to the data stack
  void pushSimulatedCurves();

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

  /// Resets all the parameters data to the initial guess
  void resetAllToInitialGuess();

  /// Resets the currently displayed dataset parameters to initial
  /// guess.
  void resetThisToInitialGuess();

  /// Saves all the views as PDF
  void saveAllPDF();

};

#endif
