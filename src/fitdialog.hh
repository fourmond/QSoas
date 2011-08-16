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

class FitData;
class CurveView;
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

  /// The button group for the "global or not global" checkboxes
  QButtonGroup * globalGroup;

  /// And the corresponding checkboxes
  QList<QCheckBox *> globalCheckBoxes;

  /// The button group for the global "fixed or free" checkboxes
  QButtonGroup * globalFixedGroup;

  /// And the corresponding checkboxes
  QList<QCheckBox *> globalFixedCheckBoxes;

  /// The button group for the buffer-local "fixed or free" checkboxes
  QButtonGroup * localFixedGroup;

  /// And the corresponding checkboxes
  QList<QCheckBox *> localFixedCheckBoxes;

  /// The editors for the global parameters. Note that we use
  /// QLineEdit and not QDoubleSpinBox as the latter will round and
  /// will become very painful some times...
  QList<QLineEdit *> globalEditors;

  /// Editors for the local values of parameters.
  QList<QLineEdit *> localEditors;


  /// The current parameters of the Fit, in unpacked form
  double * unpackedParameters;

  /// The current value of the global flag
  bool * isGlobal;

  /// The current value of the fixed flag
  bool * isFixed;

  /// The current index
  int currentIndex;

  /// A flag to avoid updating twice...
  bool settingEditors;


  /// Fills the FitData with parameter information
  void setDataParameters();

  DataSet * simulatedData(int i);


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

  /// Set the global flag for the parameter number i
  void setGlobal(bool global, int index);

  /// Called when one of the global flags are clicked.
  void onSetGlobal(int index);

  /// Called whenever a fixed checkbox is clicked...
  void onSetFixed(int index);

  /// Update the values from the editors.
  void updateFromEditors();

  /// Starts the fit !
  void startFit();

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

};

#endif
