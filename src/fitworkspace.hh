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
class Command;

class ParameterSpaceExplorer;
class OneTimeWarnings;

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
  friend class FitEngine;

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

public:

  OneTimeWarnings * warnings;
  
  /// Loads parameters from a parsed parameters file.
  ///
  /// If \a targetDS isn't negative, we copy only from \a sourceDS to
  /// the given targetDS.
  void loadParameters(const FitParametersFile & params, int targetDS = -1, 
                      int sourceDS = 0);


  /// Same as loadParameters, but splice the source and target DS
  /// according to the @a splice parameter, which is a hash target ->
  /// source. (as a source can be used for several targets)
  void loadParameters(const FitParametersFile & params,
                      const QHash<int, int> & splice);

  /// Loads the parameter VALUES from a parsed parameters file, taking
  /// into account Z values and interpolating if needed.
  ///
  /// It won't load other things such the fixed and global statuses,
  /// and the formulas...
  void loadParametersValues(FitParametersFile & params);

  /// A function to format residuals in a consistent way
  static QString formatResiduals(double res);

private:
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

  /// The commands generated during the initialization of the
  /// workspace, that must be destroyed upon destruction.
  QList<Command *> generatedCommands;
public:

  static FitWorkspace * currentWorkspace();

  /// @name Residuals
  ///
  /// Residuals-related storage
  /// @{


  /// The point-average residuals for each dataset
  Vector pointResiduals;

  /// The overall average point residuals. It is defined as:
  /// 
  /// \f[
  ///   \sqrt{\frac{\sum_i w_i\left(f_i - y_i\right)^2}{\sum_i w_i}}
  /// \f]
  /// 
  /// In which \f$f_i\f$ is the computed function, \f$y_i\f$ the data and
  /// \f$w_i\f$ the weight of the point.
  double overallPointResiduals;


  /// The chi-squared parameter, defined as:
  /// 
  /// \f[
  ///   \sum_i w_i\left(f_i - y_i\right)^2
  /// \f]
  double overallChiSquared;

  /// The relative residuals for each dataset
  Vector relativeResiduals;

  /// The overall relative residuals;
  double overallRelativeResiduals;

  /// Recomputes all the residuals -- assumes that the
  /// fitData->storage contains the correctly evaluated function.
  ///
  /// Optionally calls the setRubyResiduals() function.
  void computeResiduals(bool setRuby = false);

  /// Sets the following global variables:
  ///
  ///  * $residuals -> the values of overallPointResiduals
  ///  * $detailed_res -> [unspecified as of now]
  void setRubyResiduals();
  
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


  explicit FitWorkspace(FitData * data);
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
  /// If @a unknowns is provided, adds the names of the parameters
  /// that could not be parsed there, else throws exceptions upon
  /// unknown parameters.
  QList<QPair<int, int> > parseParameterList(const QString & spec,
                                             QStringList * unknowns = NULL) const;

  /// Returns the index of the named parameter (not taking into
  /// account datasets). Returns -1 if no such parameter exists.
  int parameterIndex(const QString & name) const;
  
  /// Sets the value by name. 
  void setValue(const QString & name, double value, int ds = -1);

  /// Gets the value of the given parameter
  double getValue(int index, int dataset) const {
    if(dataset == 0 || isGlobal(index))
      return values[index % nbParameters];
    else
      return values[dataset * nbParameters + (index % nbParameters)];
  };

  /// Returns the parameter value as QString
  QString stringValue(int index, int dataset) const;

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
  double goodnessOfFit();


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

  /// The name of the numbered parameter. This does not consider the
  /// dataset, the index must be lower than parametersPerDataset().
  QString parameterName(int idx) const;

  /// The full name of the parameter including the dataset
  /// specification, when given an index in saveParameterValues().
  QString fullParameterName(int idx) const;

  /// The names of all parameters (for a single buffer)
  QStringList parameterNames() const;

  /// The number of datasets
  int datasetNumber() const;

  /// The number of parameter per dataset
  int parametersPerDataset() const;

  /// The total number of parameters, i.e. the number by dataset
  /// multiplied by the number of datasets.
  int totalParameterNumber() const;

  /// @}

  /// @name Computed functions handling
  ///
  /// A set of functions to retrieve the values of the computed
  /// functions (and recompute them)
  ///
  /// @{


  /// Recompute data stored in the storage vector of fitData. Also
  /// updates the residuals.
  ///
  /// Returns @true if the computation succeeded, or false if it
  /// failed (i.e. invalid parameters, exception...)
  ///
  /// If @a silent is true, then exceptions are silently caught. If
  /// not, they are forwarded.
  bool recompute(bool dontSend = false, bool silent = true);

  /// Force the recomputation of the jacobian, useful to ensure that
  /// the errors are up-to-date. Use with caution
  void recomputeJacobian();

  /// Returns the computed subfunctions
  QList<Vector> computeSubFunctions(bool dontSend = false,
                                    QStringList * annotations = NULL);

  /// The data computed from the fit function.
  ///
  /// This function does not trigger a computation, you should use
  /// recompute() for that.
  DataSet * computedData(int i, bool residuals = false);

  /// Push computed datasets onto the stack
  ///
  /// @li @a residuals, if true, then the residuals are pushed
  /// @li @a subfunctions, if true, then the columns Y2 and so
  /// forth are subfunctions, if applicable
  /// @li pushing to the datastack is effected via the @a helper
  /// DataStackHelper when present
  ///
  /// This function does @b not recompute the data, call recompute()
  /// before if you have changed the parameters.
  void pushComputedData(bool residuals = false, bool subfunctions = false,
                        DataStackHelper * helper = NULL);

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

  /// Exports the parameters as a dataset, with a row for each buffer,
  /// and a column for each parameter.
  ///
  /// The X value is either the perpendicular coordinates or simply an
  /// index when there is no perpendicular coordinate.
  ///
  /// The pointer parameters are used to plug different values of the
  /// parameters/errors.
  ///
  /// @todo In fact these parameters are not used -- remove ?
  DataSet * exportAsDataSet(bool errors = false, bool meta = false,
                            const double * paramValues = NULL,
                            const double * errorValues = NULL);

  /// Prepares the additional info for export, which is:
  /// @li the row names (in @p rowNames)
  /// @li the X coordinates (in @p xCoords)
  /// @li their names (in @p xName)
  /// @li the names of meta (in @p metaNames)
  /// @li the values of the meta (in @p metaValues)
  ///
  /// If any of the last two is NULL, then the meta are not prepared.
  void prepareInfo(QStringList * rowNames, Vector * xCoords,
                   QString * xName, QStringList * metaNames = NULL,
                   QList<Vector> * metaValues = NULL) const;


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

protected:
  /// Save to the given stream
  void saveParameters(QIODevice * out,
                      const QStringList & comments) const;

public:

  /// Save to the named file.
  ///
  /// The @a opts argument is given directly to File handling.
  void saveParameters(const QString & fileName,
                      bool overwrite = false,
                      const QStringList & comments = QStringList(),
                      const CommandOptions & opts = CommandOptions()) const;

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

  /// @name Parameter space exploration
  ///
  /// @{

  // protected:
  ParameterSpaceExplorer * currentExplorer;
  // public:

  void setExplorer(ParameterSpaceExplorer * expl);

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
  ///
  /// If @a dataset is not negative, then only restore the parameters
  /// for that dataset.
  void restoreParameterValues(const Vector & values, int dataset = -1);

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
    Exception,
    Running,
    NotStarted,
    Invalid
  } Ending;


  /// Returns a short string describing the ending
  static QString endingDescription(Ending end);

  /// a color associated with the ending ?
  static QColor endingColor(Ending end);

  /// @name Functions to run the fit
  /// @{

  /// Whether or not we should cancel the current fit.
  bool shouldCancelFit;

  /// The internal residuals
  double residuals;

  /// The last internal residuals
  double lastResiduals;

  /// @infra This is awkward, since the engine creation is handled by
  /// FitData, but FitWorkspace handles the options
  ///
  /// @todo This should be handled via a dedicated function ?
  QHash<FitEngineFactoryItem *, CommandOptions * > fitEngineParameterValues;
  
  /// Returns the parameters for the given fit engine, creating it if
  /// necessary.
  CommandOptions * fitEngineParameters(FitEngineFactoryItem * engine);
  
  /// The time at which the fit started
  QDateTime fitStartTime;

  /// The time in seconds that has elapsed since the beginning of the fit
  double elapsedTime() const;


  /// @name Fit trajectories
  ///
  /// These attributes/functions are related to the fit trajectories.
  /// 
  /// @{
  
  /// All the initial guess -> final pairs since the beginning of the
  /// spawn of the Fit Workspace
  FitTrajectories trajectories;

  /// The current flags applied to trajectories
  QSet<QString> currentFlags;

  /// Sets the flags for the trajectories for all subsequent fits
  void setTrajectoryFlags(const QSet<QString> & flags);

  /// Returns the last trajectory, or fails with an internal error if
  /// there isn't.
  const FitTrajectory & lastTrajectory() const;

  /// @}

  /// Starts the fit.
  void startFit();

  /// Runs the next iteration, returns the status code
  int nextIteration();

  Ending runFit(int iterationLimit);

  void endFit(Ending ending);

  /// The reason for the end of fit
  Ending fitEnding;

  /// Type describing the current status with respect to the parameters.
  typedef enum {
    ParametersUnknown,
    ParametersOK,
    ParametersFailed
  } ParametersStatus;

  /// The current status of the parameters
  ParametersStatus parametersStatus;

  /// Wether the covariance matrix is up-to-date or not
  bool covarianceMatrixOK;

  /// Attempts to find which parameters are linear, and optionally
  /// solve the linear least squares problem (if deltas is NULL).  It
  /// doesn't handle the linear global parameters properly for now.
  ///
  /// Returns a list of parameter, dataset
  QList<QPair<int, int> > findLinearParameters(Vector * deltas = NULL,
                                               double threshold = 1e-5);

  /// @name Tracing
  ///
  /// Tracing is a (simple) way to follow what the fit algorithm is
  /// doing.
  /// 
  /// @{
private:
  /// If this isn't NULL, then tracing will occur
  QTextStream * tracingStream;

  /// If tracing is on, write the tracing information to the tracing file.
  void traceFit();

public:

  /// Sets up the given tracing stream. NULL cancels tracing. The
  /// workspace does not take ownership of the stream.
  void setTracing(QTextStream * target);


public slots:
  /// Cancels the fit
  void cancelFit();

  /// Sets the fit engine factory of the fit, and sets its options.
  void setFitEngineFactory(FitEngineFactoryItem * item,
                           const CommandOptions & opts = CommandOptions());

  /// @}

signals:
  /// This signal is sent at the end of every iteration. Provides the
  /// current iteration number, the current residuals and the parameters
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

  /// Emitted whenever the fit factory has changed. 
  void fitEngineFactoryChanged(FitEngineFactoryItem * item);

protected slots:
  /// Called whenever a parametersChanged() or parameterChanged()
  /// signal is emitted, so as to make sure the flags are kept
  /// up-to-date.
  void invalidateStatus();

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
  explicit CovarianceMatrixDisplay(FitWorkspace * params, QWidget * parent = 0);

public slots:

  /// Export to file
  void exportToFile();

  /// Export as a LaTeX table ?
  void exportAsLatex();
};

#endif
