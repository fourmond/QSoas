/**
   \file fitworkspace.hh
   The FitWorkspace class, handling fit parameters as the user sees them
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
#ifndef __FITWORKSPACE_HH
#define __FITWORKSPACE_HH


#include <fitengine.hh>
#include <fittrajectories.hh>
#include <possessive-containers.hh>
#include <vector.hh>

class FitData;
class FitParameter;
class OutFile;

class FitParametersFile;

class CurveData;
class DataSet;
class DataStackHelper;

/// This class holds all the structures necessary to run a fit, from
/// QSoas's perspective -- setting of parameters
///
/// This class is also responsible for saving/loading the parameters,
/// for exporting (and importing ?) them and so on.
///
/// @todo Handle import (which is quite different from load).
class FitWorkspace : public QObject {

  Q_OBJECT;

  /// The underlying FitData object.
  FitData * fitData;

  /// A cache parameter name -> index
  QHash<QString, int> parameterIndices;

  /// The number of datasets
  int datasets;

  /// The current dataset
  int currentDS;

  /// Parameter values (same size as fixed)
  double * values;

  /// A list of gsl_vector_view pointing to the values of a given
  /// parameter as a function of the dataset index.
  QList<gsl_vector_view> parameterView;

  /// Parameter errors. Set to zero whenever the parameters are
  /// changed.
  ///
  /// Recomputed by calling explicitly 
  double * errors;

  /// A list of gsl_vector_view pointing to the values of a given
  /// parameter as a function of the dataset index.
  QList<gsl_vector_view> errorView;

  /// This list has the same size as values.
  PossessiveList<FitParameter> parameters;

  /// The number of parameters for each dataset in the "unpacked" data
  int nbParameters;

  friend class FitParameterEditor;

  /// Returns the given parameter
  const FitParameter * parameter(int idx, int ds) const {
    return parameters[idx + nbParameters * ds];
  };

  /// Returns the given parameter
  FitParameter * parameter(int idx, int ds) {
    return parameters[idx + nbParameters * ds];
  };

  /// Returns a reference to the given parameter
  FitParameter *& parameterRef(int idx, int ds) {
    return parameters[idx + nbParameters * ds];
  };

  double valueFor(int index, int dataset) const {
    return values[dataset * nbParameters + (index % nbParameters)];
  };

  double & valueFor(int index, int dataset) {
    return values[dataset * nbParameters + (index % nbParameters)];
  };

  void dump() const;

  /// Clears the parameter list
  void clear();

  void prepareExport(QStringList & headers, QString & lines, 
                     bool exportErrors = false, bool exportMeta = true);

  /// Loads parameters from a parsed parameters file.
  ///
  /// If \a targetDS isn't negative, we copy only from \a sourceDS to
  /// the given targetDS.
  void loadParameters(FitParametersFile & params, int targetDS = -1, 
                      int sourceDS = 0);
  
  /// Loads the parameter VALUES from a parsed parameters file, taking
  /// into account Z values and interpolating if needed.
  ///
  /// It won't load other things such the fixed and global statuses,
  /// and the formulas...
  void loadParametersValues(FitParametersFile & params);


  /// This updates the parameters values, by packing from values and
  /// unpacking back to values.
  ///
  /// By default, this function updates the parameters list in the
  /// FitData. If you don't want that, set @a dontSend to true.
  void updateParameterValues(bool dontSend = false);

  /// Two temporary storage for the covariance matrix
  gsl_matrix * rawCVMatrix;
  gsl_matrix * cookedCVMatrix;

  /// Frees the matrices
  void freeMatrices();

  /// Allocates and computes the covariance matrices.
  void computeMatrices();

  /// Writes the text to the stream-like target
  template <typename T>  void writeText(T & target, 
                                        bool writeMatrix = false,
                                        const QString & prefix = "") const;

  /// Makes sure the index and datasets are within boundaries. Dataset
  /// can be negative, it will mean "all datasets".
  void checkIndex(int index, int ds) const;

  static FitWorkspace * currentWS;
public:

  static FitWorkspace * currentWorkspace();

  /// @name Residuals
  ///
  /// Residuals-related storage
  /// @{


  /// The point-average residuals for each dataset
  Vector pointResiduals;

  /// The overall point residuals
  double overallPointResiduals;

  /// The relative residuals for each dataset
  Vector relativeResiduals;

  /// The overall relative residuals;
  double overallRelativeResiduals;

  /// Recomputes all the residuals -- assumes that the
  /// fitData->storage contains the right parameters.
  void computeResiduals();
  
  /// @}

  /// @name Parameters "backup"
  ///
  /// Storage for several "backups" of parameter values
  ///
  /// @{

  /// The initial guess of parameters
  Vector initialGuess;

  /// Thee backup, i.e. the parameters before starting the fit
  Vector parametersBackup;

public slots:
  /// Resets all the parameters to the backup values
  void resetToBackup();

  /// Resets only those present in @a resetOnly
  void resetToBackup(const QList<QPair<int, int> > & resetOnly);

  /// Resets all parameters to the initial guess
  void resetAllToInitialGuess();

  /// Resets the parameters for the current dataset to the initial
  /// guess.
  void resetToInitialGuess(int ds);

  /// Resets only those present in @a resetOnly
  void resetToInitialGuess(const QList<QPair<int, int> > & resetOnly);

public:

  /// @}

  /// The perpendicular coordinates. FitDialog should make them up
  /// when appropriate. There are as many elements as the number of
  /// datasets, or 0 elements if there are no relevant perpendicular
  /// coordinates.
  Vector perpendicularCoordinates;

  void computePerpendicularCoordinates(const QString & perpmeta = "");

  bool hasPerpendicularCoordinates() const;


  FitWorkspace(FitData * data);
  ~FitWorkspace();

  /// @name Parameter edition
  ///
  /// Set of functions to set/retrieve parameter values. 
  /// 
  /// @{
  
  /// Whether or not the given parameter is global
  bool isGlobal(int index) const;

  /// Whether or not the given parameter is fixed
  bool isFixed(int index, int ds) const;

  /// Sets the fixed status of the given parameter. If ds is negative,
  /// changes for all datasets.
  void setFixed(int index, int ds, bool fixed = true);
  
  /// Setting a global parameter effectively sets all the parameters !
  void setValue(int index, int dataset, double val);

  /// Parses a parameter list. Returns a list of index/dataset pairs.
  ///
  /// Throws exceptions.
  QList<QPair<int, int> > parseParameterList(const QString & spec) const;
  
  /// Sets the value by name. 
  void setValue(const QString & name, double value, int ds = -1);

  /// Gets the value of the given parameter
  double getValue(int index, int dataset) const {
    if(dataset == 0 || isGlobal(index))
      return values[index % nbParameters];
    else
      return values[dataset * nbParameters + (index % nbParameters)];
  };

  /// returns a vector pointing to the values for all the datasets of
  /// the numbered parameter.
  gsl_vector * parameterVector(int index) {
    return &parameterView[index].vector;
  };

  /// returns a vector pointing to the errors for all the datasets of
  /// the numbered parameter.
  gsl_vector * errorVector(int index) {
    return &errorView[index].vector;
  };

  /// Recompute all the errors.
  void recomputeErrors(double confidenceThreshold = 0.975);

  /// Returns the value of the \b relative error for the given parameter:
  double getParameterError(int index, int dataset, 
                           double confidenceThreshold = 0.975) const;

  /// Returns the parameters for the numbered dataset
  QHash<QString, double> parametersForDataset(int ds) const;


  /// Sets the given parameter to global or not.
  void setGlobal(int index, bool global = true);

  /// Sets the value, using a QString, which means that the parameter
  /// conversion between standard Fixed parameters and formula-based
  /// ones can happen.
  void setValue(int index, int dataset, const QString & val);

  /// Returns the text value associated with the given parameter. In
  /// principle, passing that value to setValue() should be a no-op --
  /// barring floating-point conversion problems, of course.
  QString getTextValue(int index, int dataset) const;


  /// Returns the weight of the given buffer
  double getBufferWeight(int dataset) const;


  /// Sets the buffer weight. Negative means set all buffer weights
  void setBufferWeight(int dataset, double value);

  /// @}

  /// Returns the underlying data
  const FitData * data() const {
    return fitData;
  };

  /// This function returns the chi-squared goodness of fit.
  ///
  /// This function depends on error vectors, it will return 0 if they
  /// are not available.
  double goodnessOfFit() const;


  /// @name Commodity accessors
  ///
  /// A few functions that wrap around Fit's function, providing them
  /// the correct FitData.
  ///
  /// @{

  bool hasSubFunctions() const;

  bool displaySubFunctions() const;

  QString annotateDataSet(int idx) const;

  CommandOptions currentSoftOptions() const;


  void processSoftOptions(const CommandOptions & opts) const;

  QString fitName(bool includeOptions = true) const;

  /// The name of the numbered parameter
  QString parameterName(int idx) const;

  /// The names of all parameters
  QStringList parameterNames() const;

  /// The number of datasets
  int datasetNumber() const;

  /// @}

  /// @name Computed functions handling
  ///
  /// A set of functions to retrieve the values of the computed
  /// functions (and recompute them)
  ///
  /// @{


  /// Recompute data stored in the storage vector of fitData. Also
  /// updates the residuals.
  void recompute(bool dontSend = false);

  /// Force the recomputation of the jacobian, useful to ensure that
  /// the errors are up-to-date. Use with caution
  void recomputeJacobian();

  /// Returns the computed subfunctions
  QList<Vector> computeSubFunctions(bool dontSend = false);

  /// The data computed from the fit function.
  ///
  /// This function does not trigger a computation, you should use
  /// recompute() for that.
  DataSet * computedData(int i, bool residuals = false);

  /// Push computed datasets onto the stack
  ///
  /// @todo This should be interfaced with DataStackHelper
  void pushComputedData(bool residuals = false, DataStackHelper * help = NULL);

  /// Computes and push the jacobian matrix to the stack
  void computeAndPushJacobian();

  /// Push computed the original datasets, annotated with parameters
  ///
  /// @todo This should be interfaced with DataStackHelper
  void pushAnnotatedData(DataStackHelper * help = NULL);

  /// @}

  /// Translates the current state into FitData parameters.
  void sendDataParameters();

  /// Prepare fit
  void prepareFit(CommandOptions * opts = NULL);

  /// Retrieve parameters from the fit
  void retrieveParameters();


  /// @name IO functions
  ///
  /// Functions to read/write parameter files
  ///
  /// @{
  
  /// Export to the target stream. This essentially exports raw
  /// values, thereby losing information about what was fixed and what
  /// wasn't.
  ///
  /// On the other hand, it may be easier to work with than the output
  /// of saveParameters (which is closer to the original Soas
  /// version).
  void exportParameters(QIODevice * out, bool writeError = false);

  /// Write the result of the fits to the terminal, as those can be
  /// quite interesting... It is different from exportParameters in
  /// the sense that it is optimized for human reading.
  ///
  /// @todo Write the correlation matrix ? Write the confidence matrix ?
  void writeToTerminal(bool writeMatrix = false);

  /// Export parameters to the given output file (or the default one)
  void exportToOutFile(bool writeError = false, OutFile * out = NULL);

  /// Save to the given stream
  void saveParameters(QIODevice * out) const;

  /// Save to the named file
  void saveParameters(const QString & fileName) const;

  /// Load from the given stream
  void loadParameters(QIODevice * in, int targetDS = -1, int sourceDS = 0);

  /// Load from the file
  void loadParameters(const QString & fn, int targetDS = -1, int sourceDS = 0);

  /// Loads the parameter values, taking into account the
  /// perpendicular coordinates when applicable.
  void loadParametersValues(QIODevice * in);

  /// Overridden version 
  void loadParametersValues(const QString & fn);


  /// @}
  
  /// Fill up a QTableWidget with the contents of the covariance
  /// matrix. If not in raw mode, display the correlation coefficients
  /// and the square root of the diagonal parameters.
  ///
  /// A side effect of this function (is that a hack ?)
  void setupWithCovarianceMatrix(QTableWidget * widget, bool raw = false);


  /// Writes the covariance matrix to a file
  void writeCovarianceMatrix(QTextStream & out,  bool raw = false);

  /// Writes the covariance matrix in latex-friendly form to a file.
  void writeCovarianceMatrixLatex(QTextStream & out,  bool raw = false);
  


  /// @name Functions to backup parameters...
  ///
  /// @{

  /// Saves the current parameters as a Vector. If data has a engine,
  /// retrieve it from the engine.
  Vector saveParameterValues();

  /// Returns the \b relative errors on the parameters as a Vector
  Vector saveParameterErrors(double confidenceThreshold = 0.975);

  /// Restores the previously saved values.
  void restoreParameterValues(const Vector & values);

  /// Same as the other one, only restores the parameters
  void restoreParameterValues(const Vector & values,
                              const QList<QPair<int, int> > & parameters);


  /// Wraps the given parameters (obtained from saveParameterValues(),
  /// for instance) as a Ruby [dataset][name] construct
  mrb_value parametersToRuby(const Vector & values) const;


  /// @}

  /// The status of the fitting process
  typedef enum {
    Converged,
    Cancelled,
    TimeOut,
    NonFinite,
    Error,
    Running,
    NotStarted,
    Invalid
  } Ending;

  /// @name Functions to run the fit
  /// @{

  /// Whether or not we should cancel the current fit.
  bool shouldCancelFit;

  /// The internal residuals
  double residuals;

  /// The last internal residuals
  double lastResiduals;

  QHash<FitEngineFactoryItem *, CommandOptions * > fitEngineParameterValues;
  
  /// The time at which the fit started
  QDateTime fitStartTime;

  /// The time in seconds that has elapsed since the beginning of the fit
  double elapsedTime() const;

  /// All the initial guess -> final pairs since the beginning of the
  /// spawn of the Fit Workspace
  FitTrajectories trajectories;

  /// The current name for trajectories
  QString trajectoryName;

  /// Named trajectories
  QHash<QString, FitTrajectories *> namedTrjs;

  /// Sets the name for currnt trajectory namespace
  void setTrajectoryName(const QString & name);

  /// Returns the named trajectory. * or empty designates the global
  /// trajectory.
  const FitTrajectories & namedTrajectories(const QString & name);

  /// Starts the fit.
  void startFit();

  /// Runs the next iteration, returns the status code
  int nextIteration();

  void runFit(int iterationLimit);

  void endFit(Ending ending);

  /// The reason for the end of fit
  Ending fitEnding;

  /// @}

signals:
  /// This signal is sent at the end of every iteration. Provides the
  /// current iteration number and the parameters
  void iterated(int iteration, double residuals,
                const Vector & parameters);

  /// Sent when the fit is finished
  void finishedFitting(int);

  /// Sent at the beginning of the fit. Passes the number of free parameters
  void startedFitting(int freeParameters);

  /// Emitted when the workspace is finishing
  void quitWorkspace();

  /// Emitted when the value of the given parameter has changed
  void parameterChanged(int index, int ds);

  /// Emitted when all the parameters were changed
  void parametersChanged();

  /// Emitted whenever the current dataset has changed
  void datasetChanged(int newDS);

public slots:
  /// Triggers the emission of the quitWorkSpace() signal.
  void quit();

  /// Selects the current dataset.
  ///
  /// Will fail with an exception if @a silent is false and @a dataset
  /// is out of bounds.
  void selectDataSet(int dataset, bool silent = true);

public:
  int currentDataset() const;
};


/// This class wraps up a QTableWidget to display covariance matrices
/// (and possibly save them up ?)
class CovarianceMatrixDisplay : public QDialog {
  Q_OBJECT;

  FitWorkspace * parameters;
  
  QTableWidget * widget;

  void setupFrame();
public:
  CovarianceMatrixDisplay(FitWorkspace * params, QWidget * parent = 0);

public slots:

  /// Export to file
  void exportToFile();

  /// Export as a LaTeX table ?
  void exportAsLatex();
};

#endif
