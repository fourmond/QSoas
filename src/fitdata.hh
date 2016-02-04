/**
   \file fitdata.hh
   Implementation of fits helper functions
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
#ifndef __FITDATA_HH
#define __FITDATA_HH

#include <vector.hh>
#include <possessive-containers.hh>

#include <fitengine.hh>

class Fit;  
class FitParameter;
class FreeParameter;
class DataSet;
class ParameterDefinition;
class FitInternalStorage;

                     

/// Fit data. This data will be carried around using the void *
/// argument to the function calls.
///
/// This class is in charge of converting the fit parameters from the
/// fit algorithm point of view (ie, only ActualParameter) to the Fit
/// point of view (ie ParameterDefinition).
class FitData {

public:

  /// These functions compute the fit values using internal parameters
  int f(const gsl_vector * x, gsl_vector * f, bool doSubtract = true);
  int df(const gsl_vector * x, gsl_matrix * df);
  int fdf(const gsl_vector * x, gsl_vector * f, gsl_matrix * df);


  /// Computes the function
  int computeFunction(const double * params, gsl_vector * f,
                      bool doSubtract = false);
private:

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

  /// Wether the covarStorage matrix is up-to-date
  bool covarIsOK;

  /// Dumps the given string if debug is on
  void dumpString(const QString & str) const;

  /// Dumps the parameters as seen by the GSL if debug is on
  void dumpGSLParameters(const gsl_vector * params) const;

  /// Dump the fit parameters if debug is on
  void dumpFitParameters(const double * params) const;

  /// Dumps the structure of the parameters, ie what parameters are
  /// fixed, how they unpack and so on...
  void dumpFitParameterStructure() const;


  friend class FitEngine;
  /// The fit engine in use
  FitEngine * engine;
  /// Extra parameters
  QStringList extra;



public:
  /// The number of function evaluations since the last change of fit
  /// engine.
  int evaluationNumber;

  bool hasEngine() const {
    return engine;
  };

  /// The fit in use
  const Fit * fit;

  /// The debug level. 0 means no debug output to stdout, the details
  /// go increasing with the value.
  int debug;

  /// A storage space allocated by the fit for storing fit options, 
  FitInternalStorage * fitStorage;

  /// The datasets holding the data.
  QList<const DataSet *> datasets;

  /// If not NULL, standard gaussian errors on Y for each point
  gsl_vector * standardYErrors;

  /// If not NULL, weight of each point (1/error^2) for each point
  gsl_vector * pointWeights;

  /// Whether or not this FitData makes use of point weights
  bool usesPointWeights() const;

  /// Ensure the standardYErrors and pointWeights are set to some
  /// meaningful value (or unset)
  void computeWeights();


  /// Checks if some datasets have error bars and some others don't,
  /// in which case the error bars are NOT taken into account.
  ///
  /// Returns false if the weights are not used consistently.
  bool checkWeightsConsistency() const;


  /// The number of iterations
  int nbIterations;

  /// The residuals, as reported by the FitEngine.
  double residuals();

  /// Whether the given parameter (id, dataset) is fixed or not.
  ///
  /// @a ds has to be positive.
  bool isFixed(int id, int ds) const;


  /// All parameters
  PossessiveList<FitParameter> parameters;

  /// The free parameters, indexed by datasets.
  QHash<int, QList<FreeParameter *> > parametersByDataset;

  /// The free parameters, indexed by parameter number
  QList< QList<FreeParameter *> > parametersByDefinition;

  /// A cache for the parameters description. It \b must be the same
  /// as what Fit::parameters() return.
  QList<ParameterDefinition> parameterDefinitions;
    
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

  /// Forces the recomputation of the jacobian. Only supported by very
  /// few fit engines.
  void recomputeJacobian();

  
  FitData(const Fit * f, const QList<const DataSet *> & ds, 
          int d = 0, const QStringList & extra = QStringList());

  /// Finishes the last steps of initialization.
  ///
  /// This should be called when the parameter list is known.
  void finishInitialization();

  /// Creates the solver, and initializes it with the correct
  /// parameters, based one the contents of parameterDefinitions and
  /// parameters.
  void initializeSolver(const double * initialGuess, 
                        FitEngineFactoryItem * engine = NULL,
                        CommandOptions * opts = NULL);


  /// Initializes the parameters (if needed). This may have to be
  /// called more often than initializeSolver, basically every single
  /// time one wants to use f() or df().
  void initializeParameters();

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
  gsl_vector_const_view viewForDataset(int ds, const gsl_vector * vect);

  /// Returns the number of (unpacked) double parameters necessary for
  /// one dataset
  int parametersPerDataset() const {
    return parameterDefinitions.size();
  };

  /// Returns the number of double parameters necessary to store an
  /// unpacked version of the parameters
  int fullParameterNumber() const {
    return datasets.size() * parametersPerDataset();
  };


  /// Subtracts the (Y) data from the target vector;
  void subtractData(gsl_vector * target);

  /// Gets the current parameters (in unpacked form)
  void unpackCurrentParameters(double * target);


  ~FitData();


  /// Detect whether the datasets in the fit are coupled or are
  /// completely independent.
  bool independentDataSets() const;

  /// Returns the index of the ParameterDefinition with the given
  /// name.
  int namedParameterIndex(const QString & name) const;

  /// Returns the number of degrees of freedom
  int doF() const;

  /// Returns the number of free parameters
  int freeParameters() const { return gslParameters; } ;

  /// Returns the overall number of data points
  int dataPoints() const { return totalSize; };

  /// Returns the required factor to apply to the variance to get the
  /// confidence limit with the given percentage
  double confidenceLimitFactor(double conf) const;

  /// @name Weighting-related functions and attributes
  ///
  /// @{

  /// The weight of each buffer, initialized to 1 by default.
  Vector weightsPerBuffer;


  /// Weights the target vector
  void weightVector(gsl_vector * tg);

  /// Computes a weighted sum of squares of the given vector, or
  /// simpled the sum of the square of the weights if the target
  /// vector is NULL. If subtractData is not NULL, the data gets
  /// subtracted in any case (wether or not \a src is provided)
  double weightedSquareSum(const gsl_vector * src = NULL, 
                           bool subtractData = false);

  /// Does the same as weightedSquareSum, but only considering the
  /// given dataset. If provided, \a src is assumed to 
  double weightedSquareSumForDataset(int ds, const gsl_vector * src = NULL,
                                     bool subtractData = false);

  /// @}


};

class ArgumentList;
class CommandEffector;

#endif
