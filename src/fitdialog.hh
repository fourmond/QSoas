/**
   \file fitdialog.hh
   Dialog box handling fits in QSoas
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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
#ifndef __FITDIALOG_HH
#define __FITDIALOG_HH

#include <fitengine.hh>
#include <fitworkspace.hh>
#include <fittrajectory.hh>

class FitData;
class CurveView;
class FitParameterEditor;
class DataSet;
class ParametersViewer;

class NupWidget;

class ArgumentList;

/// This class handles all the user interaction during fits.
class FitDialog : public QDialog {

  Q_OBJECT;

  void setupFrame();

  /// The FitData object we'll populate and run
  FitData * data;

  /// The widget holding all the views
  NupWidget * nup;

  /// The dataset views
  QList<CurveView *> views;

  /// The combo box for toogling between the various buffers
  QComboBox * bufferSelection;

  /// Label displaying the current buffer number
  QLabel * bufferNumber;

  /// The line editor for entering the weight of the buffer, set to
  /// NULL if that has been opted out.
  QLineEdit * bufferWeightEditor;

  /// Soft options of the fit. Only non-NULL if there is at least one
  /// option.
  ArgumentList * softOptions;

  /// List of editors
  QList<FitParameterEditor *> editors;

  /// Temporary storage for subfunctions
  QList<Vector> subFunctions;

  /// The parameters
  FitWorkspace parameters;

  /// The current index
  int currentIndex;


  /// A flag to avoid updating twice...
  bool settingEditors;

  /// Wheter we're showing subfunctions or not
  bool displaySubFunctions;

  /// Whether or not we've already shown inconsistency warning
  bool errorInconsistencyShown;

  /// Whether or not we are currently updating the page
  bool alreadyChangingPage;

  /// A list of the curve data objects in current use.
  ///
  /// These should be cleared basically every time we setup the
  /// curves.
  QList<CurveData*> subFunctionCurves;


  /// The name of the perpendicular coordinate from meta-data.
  QString perpendicularMeta;

  /// Setup the clearing and display of the subfunction curves. Does
  /// nothing when the subfunction display is off.
  void setupSubFunctionCurves();

  /// A small text display to report current progress
  QLabel * progressReport;

  /// A small text right next to progressReport to display residuals.
  QLabel * residualsDisplay;



  /// the start button
  QPushButton * startButton;

  /// The cancel button, only visible during fit
  QPushButton * cancelButton;

  /// Fills the FitData with parameter information
  void setDataParameters();

  /// Whether or not we should cancel the current fit.
  bool shouldCancelFit;

  /// A list of all the fits started (and ended) since the spawning of
  /// the dialog
  QList<FitTrajectory> trajectories;

  /// The parameters as saved just before starting the fit
  Vector parametersBackup;

  /// The editor for the maximum number of iterations
  QLineEdit * iterationLimitEditor;

  /// Fit engine parameters
  ///
  /// @todo A way to reset the parameter to default
  QHash<FitEngineFactoryItem *, CommandOptions * > fitEngineParameterValues;

  /// Same thing, but
  QHash<FitEngineFactoryItem *, ArgumentList * > fitEngineParameters;

  /// Selection of the fit engines...
  QComboBox * fitEngineSelection;


  /// A (guarded) pointer to the ParametersViewer
  QPointer<ParametersViewer> parametersViewer;


protected:

  virtual void closeEvent(QCloseEvent * event);

  /// Same thing as compute, but raises an exception if something failed.
  void internalCompute();


  /// Writes a message, with immediate display
  void message(const QString & str);


  /// Appends the given text to the current message
  void appendToMessage(const QString & str, bool format = false);

  /// Exports to a file after prompting for a name
  void promptExport(bool withErrors);

  /// The maximum number of iterations
  int iterationLimit;


public:
  FitDialog(FitData * data, bool displayWeights = false, 
            const QString & perpMeta = QString());
  ~FitDialog();

  /// Sets the fit engine to the named one
  void setFitEngineFactory(const QString & name);

  /// Idem, directly with the factory item
  void setFitEngineFactory(FitEngineFactoryItem * factory);

  /// Sets the maximal number of iterations
  void setIterationLimit(int nb);

  /// returns the maximal number of iterations
  int getIterationLimit() const;

  /// Returns the list of trajectories.
  const QList<FitTrajectory> & fitTrajectories() const;

  /// Returns the underlying FitData object
  FitData * getData() const;

  /// Returns the underlying FitWorkspace
  FitWorkspace * getWorkspace();
  const FitWorkspace * getWorkspace() const;

  /// This string gets written to the window title after the rest.
  QString extraTitleInfo;
  


signals:
  void currentDataSetChanged(int ds);

  /// This signal is sent at the end of every iteration. Provides the
  /// current iteration number and the parameters
  void nextIteration(int iteration, double residuals,
                     const Vector & parameters);

  /// Sent when the fit is finished
  void finishedFitting();

  /// Sent at the beginning of the fit
  void startedFitting();

public slots:

  /// Loads parameters from the given file.
  void loadParametersFile(const QString & fileName, int targetDS = -1,
                          bool recompute = true, bool onlyVals = false);

  /// Add all simulated datasets to the data stack
  void pushSimulatedCurves(const QStringList & flags = QStringList());

  /// Adds all the residuals to the data stack
  void pushResiduals();

  /// Pushes all computed subfunctions to the stack
  void pushSubFunctions();

  /// Sets the values of the parameters
  void setParameterValues(const Vector & values);

  /// Sets the parameter value by name.
  ///
  /// This function does @b not recompute the value of the fit.
  void setParameterValue(const QString & name, double value, int ds = -1);

  /// Compute the new curves based on initial guesses (or
  /// hand-modified versions)
  void compute();

  /// Exports parameters to the current output file (with errors)
  void exportToOutFileWithErrors();

  /// Start the fit
  void startFit();

  /// Cancels the current fit
  void cancelFit();

protected slots:

  /// Updates the display of residuals.
  void updateResidualsDisplay();

  /// Hides/shows all the parameter editors
  void showEditors(bool show = true);

  /// Called whenever the nup is changed
  void nupChanged();

  /// Pops up a dialog box to set soft options.
  void setSoftOptions();

  void dataSetChanged(int newds);

  void weightEdited(const QString & str);

  /// Update all the editors
  void updateEditors(bool updateErrors = false);


  /// Adds current simulated datset to the data stack
  void pushCurrentCurve();

  /// Saves all simulated data to files
  void saveSimulatedCurves();

  /// Loads parameters from files
  void loadParameters();

  /// Loads parameters for the current dataset
  void loadParametersForCurrent();

  /// Loads parameters values making use of perpendicular coordinates
  void loadUsingZValues();

  /// Saves parameters to a file
  void saveParameters();

  /// Shows all the parameters in a dialog box
  void showParameters();

  /// Exports parameters to a file, losing information about their
  /// state (global and/or fixed)
  void exportParameters();

  /// Exports parameters to a file, losing information about their
  /// state (global and/or fixed)
  void exportParametersWithErrors();

  /// Exports parameters to the current output file
  void exportToOutFile();

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


  /// Toggle the display of sub components when applicable
  void toggleSubFunctions();

  /// Assign equal weights to all buffers, ie compensate for the
  /// inevitable inequalities arising from difference in intensity and
  /// point numbers.
  void equalWeightsPerBuffer();

  /// Assign weights to buffers so that all points have about the same
  /// importance in average.
  void equalWeightsPerPoint();

  /// Resets all the weights to 1.
  void resetWeights();

  /// Double the weight of the current buffer
  void doubleWeight();
  
  /// Divides by two the weight of the current buffer
  void halfWeight();

  /// Resets the parameters to the original values (before the fit).
  void resetParameters();


  /// Upon change in the FitEngine combo box...
  void engineSelected(int id);


  /// Shows the transposed data.
  void showTransposed();

  /// Display a spreadsheet to edit the parameters
  void parametersSpreadsheet();


};

#endif
