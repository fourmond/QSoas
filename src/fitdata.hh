/**
   \file fitdata.hh
   Implementation of fits helper functions
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


#ifndef __FITDATA_HH
#define __FITDATA_HH

#include <vector.hh>
#include <possessive-containers.hh>

// For the expression handling with the fit parameters...
#include <expression.hh>

class FitData;

/// Base class for effective parameters
class FitParameter {
public:
  /// The index of the parameters from within the Fit::parameters()
  /// return value.
  int paramIndex;

  /// The index of the dataset (-1) for global parameters.
  int dsIndex;

  /// The index of the parameter in the fit vector (-1 if not part of
  /// the fit vector)
  int fitIndex;

  FitParameter(int p, int ds) : paramIndex(p), dsIndex(ds), 
                                fitIndex(-1) {;};

  /// Sets the target parameters (natural parameters) from the fit
  /// parameters (the GSL vector).
  virtual void copyToUnpacked(double * target, const gsl_vector * fit, 
                             int nbdatasets, int nb_per_dataset) const = 0;

  /// Sets values of the fit parameters (the GSL vector) from the
  /// natural parameters
  virtual void copyToPacked(gsl_vector * fit, const double * unpacked,
                            int nbdatasets, int nb_per_dataset) const;

  /// Whether or not the parameter is fixed
  virtual bool fixed() const { return false;};

  /// Whether the parameter is global or dataset-local
  virtual bool global() const { return dsIndex < 0; };

  /// Performs one-time setup at fit initialization
  virtual void initialize(FitData * data);

  /// Whether or not the parameter needs a second pass.
  virtual bool needSecondPass() const { return false; };

  /// Returns a duplicate of the object.
  virtual FitParameter * dup() const = 0;

  /// Returns the text value for the given parameter.
  virtual QString textValue(double value) const {
    return QString::number(value);
  };

  /// Sets the value corresponding to the given string.
  ///
  /// Most children will update \a target, but some may not (think
  /// about formulas...)
  virtual void setValue(double * target, const QString & value) {
    bool ok;
    double v = value.toDouble(&ok);
    if(ok)
      *target = v;
  };

  /// Saves the parameter as a string. It is the second part of the
  /// output of FitParameters::saveParameters().
  ///
  /// Its goal isn't to save context information (indices) but raw
  /// FitParameter information. FitParameters::saveParameters takes
  /// care of the context.
  virtual QString saveAsString(double value) const;

  /// It is the reverse of saveAsString().
  static FitParameter * loadFromString(const QString & str, 
                                       double * target, int paramIndex, 
                                       int dsIndex);

  virtual ~FitParameter();

protected:

  /// This returns extra parameters information that must be
  /// saved. For instance, that is the way Bijection is stored for
  /// FreeParameter.
  virtual QString saveExtraInfo() const;

  /// Load the information as saved by saveExtraInfo().
  virtual void loadExtraInfo(const QString & str);

};

class Bijection;

/// A parameter, once it's in use. A list of that can be used to
/// convert GSL parameters to dataset-specific parameter values.
class FreeParameter : public FitParameter {
public:

  /// The factor used for derivation (when that applies)
  double derivationFactor;
    
  /// The minimum step used for derivation
  double minDerivationStep;

  virtual void copyToUnpacked(double * target, const gsl_vector * fit, 
                             int nbdatasets, int nb_per_dataset) const;


  virtual void copyToPacked(gsl_vector * fit, const double * unpacked,
                            int nbdatasets, int nb_per_dataset) const;

  /// The transformation applied to the parameter to go from the GSL
  /// parameter space to the user- and fit- visible parameters.
  ///
  /// Will be NULL most of the times.
  Bijection * bijection;
  
  FreeParameter(int p, int ds, double dev = 1e-4) :
    FitParameter(p, ds), derivationFactor(dev), 
    minDerivationStep(1e-8), bijection(NULL) {;};

  virtual FitParameter * dup() const;

  virtual ~FreeParameter();

protected:
  virtual QString saveExtraInfo() const;
  virtual void loadExtraInfo(const QString & str);
  
};

/// A fixed parameter, ie a parameter whose value is fixed, and
/// hence isn't part of the gsl_vector of parameters.
class FixedParameter : public FitParameter {
public:
  /// The actual value
  mutable double value;

  virtual void copyToUnpacked(double * target, const gsl_vector * fit, 
                             int nbdatasets, int nb_per_dataset) const;


  virtual void copyToPacked(gsl_vector * fit, const double * unpacked,
                            int nbdatasets, int nb_per_dataset) const;


  virtual bool fixed() const { return true;};

  FixedParameter(int p, int ds, double v)  :
    FitParameter(p, ds), value(v) {;};

  virtual FitParameter * dup() const {
    return new FixedParameter(*this);
  };
};

/// A parameter whose value is defined as a function of other
/// parameters.
class FormulaParameter : public FitParameter {
  Expression expression;

public:

  /// The parameter dependencies (to ensure they are computed in the
  /// correct order). It is also the argument list to the block
  QStringList dependencies;

  /// The same thing as dependencies, but with their index (in the
  /// parametersDefinition)
  QVector<int> depsIndex;



  QString formula;

  // The last value this stuff took... (could be useful ?)
  mutable double lastValue;

  virtual void copyToUnpacked(double * target, const gsl_vector * fit, 
                             int nbdatasets, int nb_per_dataset) const;


  virtual void copyToPacked(gsl_vector * fit, const double * unpacked,
                            int nbdatasets, int nb_per_dataset) const;


  virtual bool fixed() const { return true;};

  FormulaParameter(int p, int ds, const QString & f)  :
    FitParameter(p, ds), expression(f), formula(f) {;};

  virtual void initialize(FitData * data);

  virtual bool needSecondPass() const { return true; };

  virtual FitParameter * dup() const {
    return new FormulaParameter(*this);
  };

  virtual QString saveAsString(double value) const;

  virtual QString textValue(double ) const {
    return expression.formula();
  };

  virtual void setValue(double * target, const QString & value);

};




class Fit;  
class DataSet;
class ParameterDefinition;

/// Fit data. This data will be carried around using the void *
/// argument to the function calls.
///
/// This class is in charge of converting the fit parameters from the
/// fit algorithm point of view (ie, only ActualParameter) to the Fit
/// point of view (ie ParameterDefinition).
class FitData {

  static int staticF(const gsl_vector * x, void * params, gsl_vector * f);
  static int staticDf(const gsl_vector * x, void * params, gsl_matrix * df);
  static int staticFdf(const gsl_vector * x, void * params, gsl_vector * f,
                       gsl_matrix * df);

  int f(const gsl_vector * x, gsl_vector * f);
  int df(const gsl_vector * x, gsl_matrix * df);
  int fdf(const gsl_vector * x, gsl_vector * f, gsl_matrix * df);

  int totalSize;

  void freeSolver();

  /// This list is full with several FitData, one for each dataset to
  /// perform independant fitting when indendentDataSets returns true.
  QList<FitData*> subordinates;

  /// The number of GSL parameters. To be computed at initialization
  /// time.
  int gslParameters;

  /// A storage space for the covariance matrix
  gsl_matrix * covarStorage;

  /// Dumps the given string if debug is on
  void dumpString(const QString & str) const;

  /// Dumps the parameters as seen by the GSL if debug is on
  void dumpGSLParameters(const gsl_vector * params) const;

  /// Dump the fit parameters if debug is on
  void dumpFitParameters(const double * params) const;

public:
  /// The fit in use
  Fit * fit;

  /// A debug flag. If that flag is on, all calls to the functions are
  /// instrumented. For debugging the fit engine and/or the fit
  /// functions.
  bool debug;

  /// The datasets holding the data.
  QList<const DataSet *> datasets;

  /// Push parameters -- why not ?
  FitData & operator<<(const FitParameter & param);
  FitData & operator<<(FitParameter * param);

  /// All parameters
  PossessiveList<FitParameter> parameters;

  /// A cache for the parameters description. It \b must be the same
  /// as what Fit::parameters() return.
  QList<ParameterDefinition> parameterDefinitions;
    
  /// The solver in use
  gsl_multifit_fdfsolver * solver;

  /// The function in use
  gsl_multifit_function_fdf function;

  int nbIterations;

  /// A storage vector of the same size as the data points vector
  gsl_vector * storage;

  /// Another storage space, this time large enough to hold all parameters.
  gsl_vector * parametersStorage;

  /// Returns the covariance matrix, including the 0 elements of fixed
  /// parameters.
  ///
  /// @todo As of now, it is the raw covariance matrix (ie plainly
  /// wrong when using bijected parameters)
  const gsl_matrix * covarianceMatrix();


  FitData(Fit * f, const QList<const DataSet *> & ds, bool d = false);

  /// Creates the solver, and initializes it with the correct
  /// parameters, based one the contents of parameterDefinitions and
  /// parameters.
  void initializeSolver(const double * initialGuess);

  /// Iterates the solver, and returns the return code
  int iterate();

  /// Packs the full parameters specification into the actual
  /// parameter vector.
  void packParameters(const double * unpacked, 
                      gsl_vector * packed) const;

  /// Unpacks a parameter vector into a pre-allocated array of
  /// doubles.
  void unpackParameters(const gsl_vector  * packed, 
                        double * unpacked) const;

  gsl_vector_view viewForDataset(int ds, gsl_vector * vect);

  /// Returns the number of double parameters necessary to store an
  /// unpacked version of the parameters
  int fullParameterNumber() const {
    return datasets.size() * parameterDefinitions.size();
  };

  /// Subtracts the (Y) data from the target vector;
  void subtractData(gsl_vector * target);

  /// Gets the current parameters (in unpacked form)
  void unpackCurrentParameters(double * target);

  /// The residuals (ie sum of the square of differences)
  ///
  /// This are the raw residuals, ie what is seen by the fit
  /// algorithm.
  ///
  /// @todo write code for accessing to residuals relative to a single
  /// dataset ? (but that doesn't make much sense, does it ?)
  double residuals();

  /// The relative residuals: norm of the residuals over the norm of
  /// the data.
  ///
  /// @warning This takes into account the weight of the data.
  double relativeResiduals();

  ~FitData();


  /// Detect whether the datasets in the fit are coupled or are
  /// completely independent.
  bool independentDataSets() const;

  /// Returns the index of the ParameterDefinition with the given
  /// name.
  int namedParameterIndex(const QString & name) const;

  /// Returns the number of degrees of freedom
  int doF() const;

  /// Returns the required factor to apply to the variance to get the
  /// confidence limit with the given percentage
  double confidenceLimitFactor(double conf) const;

  /// @name Weighting-related functions and attributes
  ///
  /// @{

  /// The weight of each buffer, initialized to 1 by default.
  Vector weightsPerBuffer;



protected:
  /// Weights the target vector
  void weightVector(gsl_vector * tg);


  /// @}


};

class ArgumentList;
class CommandEffector;

#endif
