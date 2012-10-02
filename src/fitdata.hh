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


#include <headers.hh>
#ifndef __FITDATA_HH
#define __FITDATA_HH

#include <vector.hh>
#include <possessive-containers.hh>

class Fit;  
class FitEngine;
class FitEngineFactoryItem;
class FitParameter;
class DataSet;
class ParameterDefinition;

/// Fit data. This data will be carried around using the void *
/// argument to the function calls.
///
/// This class is in charge of converting the fit parameters from the
/// fit algorithm point of view (ie, only ActualParameter) to the Fit
/// point of view (ie ParameterDefinition).
class FitData {

public:
  int f(const gsl_vector * x, gsl_vector * f, bool doSubtract = true);
  int df(const gsl_vector * x, gsl_matrix * df);
  int fdf(const gsl_vector * x, gsl_vector * f, gsl_matrix * df);

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

  /// Dumps the given string if debug is on
  void dumpString(const QString & str) const;

  /// Dumps the parameters as seen by the GSL if debug is on
  void dumpGSLParameters(const gsl_vector * params) const;

  /// Dump the fit parameters if debug is on
  void dumpFitParameters(const double * params) const;

  /// The fit engine in use
  FitEngine * engine;

  /// Extra parameters
  QStringList extra;

public:
  /// The fit in use
  Fit * fit;

  /// A debug flag. If that flag is on, all calls to the functions are
  /// instrumented. For debugging the fit engine and/or the fit
  /// functions.
  bool debug;

  /// The datasets holding the data.
  QList<const DataSet *> datasets;


  /// The number of iterations
  int nbIterations;


  /// Push parameters -- why not ?
  FitData & operator<<(const FitParameter & param) __attribute__ ((deprecated));
  FitData & operator<<(FitParameter * param) __attribute__ ((deprecated));

  /// All parameters
  PossessiveList<FitParameter> parameters;

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


  
  FitData(Fit * f, const QList<const DataSet *> & ds, 
          bool d = false, const QStringList & extra = QStringList());

  /// Creates the solver, and initializes it with the correct
  /// parameters, based one the contents of parameterDefinitions and
  /// parameters.
  void initializeSolver(const double * initialGuess, 
                        FitEngineFactoryItem * engine = NULL);


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


  /// @}


};

class ArgumentList;
class CommandEffector;

#endif
