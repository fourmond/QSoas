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
class ParameterDefinition;

/// Fit data. This data will be carried around using the void *
/// argument to the function calls.
///
/// @todo I'm unsure where the code should go, but the fit mechanism
/// should have a way to detect when all the parameters are
/// buffer-specific, which means that instead of running a full-scale
/// fit, one could just run separate fits for each and every buffer.
///
/// That wouldn't be that simple, though, as one needs to keep track
/// seperately of each buffer in the iteration process.
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

class ArgumentList;
class CommandEffector;

#endif
