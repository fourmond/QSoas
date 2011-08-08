/**
   \file fit.hh
   Implementation of fits.
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


#ifndef __FIT_HH
#define __FIT_HH

/// The definition of a parameter.
class ParameterDefinition {
public:
  /// Parameter name
  QString name;

  /// Whether or not the parameter is fixed by default. Parameters
  /// that are fixed by default could be external things (say, an
  /// offset), rather than real parameters, although in some cases it
  /// may make sense to have them variable.
  bool defaultsToFixed;

  /// If true, then this parameter can be specific to one dataset
  /// (one buffer) instead of being global to all datasets fitted at
  /// the same time.
  bool canBeBufferSpecific;

  ParameterDefinition(const QString & n, bool fixed = false,
                      bool cbs = true) :
    name(n), defaultsToFixed(fixed), canBeBufferSpecific(cbs)
  {
  };
};

/// A parameter, once it's in use. A list of that can be used to
/// convert GSL parameters to dataset-specific parameter values.
class ActualParameter {
public:
  /// The index of the parameters from within the Fit::parameters()
  /// return value.
  int paramIndex;

  /// The index of the dataset (-1) for global parameters.
  int dsIndex;

  /// The factor used for derivation (when that applies)
  double derivationFactor;
    
  /// The minimum step used for derivation
  double minDerivationStep;

  ActualParameter(int p, int ds, double dev = 1e-3) :
    paramIndex(p), dsIndex(ds), derivationFactor(dev) {;};
};

/// A fixed parameter, ie a parameter whose value is fixed, and
/// hence isn't part of the gsl_vector of parameters.
class FixedParameter {
public:
  /// The index of the parameters from within the Fit::parameters()
  /// return value.
  int paramIndex;

  /// The index of the dataset (-1) for global parameters.
  int dsIndex;

  /// The actual value
  double value;

  FixedParameter(int id, int ds, double v) : 
    paramIndex(id), dsIndex(ds), value(v) {;};
};

class Fit;  
class DataSet;

/// Fit data. This data will be carried around using the void *
/// argument to the function calls.
class FitData {

  static int staticF(const gsl_vector * x, void * params, gsl_vector * f);
  static int staticDf(const gsl_vector * x, void * params, gsl_matrix * df);
  static int staticFdf(const gsl_vector * x, void * params, gsl_vector * f,
                       gsl_matrix * df);

  int totalSize;

  void freeSolver();

public:
  /// The fit in use
  Fit * fit;

  /// The datasets holding the data.
  QList<const DataSet *> datasets;

  /// Adjustable parameters
  QList<ActualParameter> parameters;

  /// Fixed parameters
  QList<FixedParameter> fixedParameters;

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

  FitData(Fit * f, const QList<const DataSet *> & ds);

  /// Creates the solver, and initializes it with the correct
  /// parameters, based one the contents of parameterDefinitions and
  /// the like.
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

  /// Returns the index of the packed parameter idx in the unpacked
  /// version.
  int packedToUnpackedIndex(int idx) const;

  /// Gets the current parameters (in unpacked form)
  void unpackCurrentParameters(double * target);

  /// The residuals (ie sum of the square of differences)
  double residuals();

  /// @todo add functions for saving/loading parameters

  ~FitData();
};


/// This abstract class defines the interface for handling fits.
///
/// @todo handle validity range checking
class Fit {
protected:

  /// Fit name, used to make up command names
  QString name;

  /// Fit description
  const char * shortDesc;

  /// Long description
  const char * longDesc;

  /// Prepares and registers the commands corresponding to the fit.
  void makeCommands();

  /// The minimum number of datasets the fit should take.
  int minDataSets;

  /// The maximum number of datasets the fit can take (-1 for
  /// boundless)
  int maxDataSets;
public:
  

  /// The parameters
  virtual QList<ParameterDefinition> parameters() const = 0;

  /// @name Inner computations
  ///
  /// These functions compute the residuals of the fit, or the
  /// jacobian matrix.
  ///
  /// The function computing the residuals.
  ///
  /// @{
  virtual int f(const gsl_vector * parameters, 
                FitData * data, gsl_vector * target_f);

  virtual int df(const gsl_vector * parameters, 
                 FitData * data, gsl_matrix * target_J);

  virtual int fdf(const gsl_vector * parameters, 
                  FitData * data, gsl_vector * f, 
                  gsl_matrix * target_J);

  /// Computes the function (ie in the absence of the residuals)
  ///
  /// \a parameters are given in the fully expanded form, ie
  /// parameters().size() times number of datasets
  ///
  /// Keep in mind that in multidataset mode, \a target contains \b
  /// all datasets !
  virtual void function(const double * parameters,
                        FitData * data, gsl_vector * target) = 0;
  /// @}

  /// Prepares an initial guess for all parameters. The \a guess array
  /// is a fully expanded version of the parameters.
  ///
  /// \see packParameters()
  /// \see unpackParameters()
  virtual void initialGuess(FitData * data, double * guess) = 0;

  /// Short description.
  QString shortDescription() const {
    return QObject::tr(shortDesc);
  };

  /// Long description
  QString description() const {
    return QObject::tr(longDesc);
  };

  /// Constructs a Fit object with the given parameters and registers
  /// the corresponding commands.
  Fit(const char * n, const char * sd, const char * desc,
      int min = 1, int max = -1, bool mkCmds = true) :
    name(n), shortDesc(sd), longDesc(desc),
    minDataSets(min), maxDataSets(max) { 
    if(mkCmds)
      makeCommands();
  };

  virtual ~Fit();

  /// Runs the fit on the current dataset
  void runFitCurrentDataSet(const QString & name);

  /// Runs the fit on the given datasets
  void runFit(const QString & name, QList<const DataSet *> datasets);
};


#endif
