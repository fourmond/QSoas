/**
   \file fitengine.hh
   Base class for engine driving the fits
   Copyright 2012, 2013, 2014, 2015 by CNRS/AMU

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
#ifndef __FITENGINE_HH
#define __FITENGINE_HH

#include <vector.hh>
#include <factory.hh>
#include <argumentmarshaller.hh>

class Fit;  
class FitData;
class ArgumentList;
class Command;
class CommandEffector;
class FitWorkspace;

/// A simple wrapper class around parameters found after an iteration.
class StoredParameters {

  /// A view around the current parameters.
  mutable gsl_vector_view view;
public:

  /// The parameters
  Vector parameters;

  /// The corresponding residuals
  double residuals;

  StoredParameters() : residuals(-1) {;};
  StoredParameters(const Vector & v, double r) : 
    parameters(v), residuals(r) {;};
  StoredParameters(const gsl_vector *, double r);

  /// Compares on residuals
  bool operator<(const StoredParameters & o) const {
    return residuals < o.residuals;
  };

  /// Returns a gsl vector for the 
  const gsl_vector * toGSLVector() const;

};


class FitEngine;
typedef Factory<FitEngine, FitData *> FitEngineFactoryItem;


/// This class wraps around call to the GSL for handling fits.
///
/// This class only sees the "GSL" side of the fits.
///
/// This is an abstract class.
///
/// @todo This class may handle automatic storage of the series of
/// parameters ?
class FitEngine {
protected:


  /// The underlying fit data
  FitData * fitData;

  /// Stored parameters
  QList<StoredParameters> storedParameters;

  /// Pushes the current parameters onto the stack. It uses
  /// currentParameters() and residuals();
  void pushCurrentParameters();

  /// Returns the command effector for switching to the given engine.
  static CommandEffector * engineEffector(const QString & name);

public:
  FitEngine(FitData * data);
  virtual ~FitEngine();

  /// Returns the short name of all available fit engines
  static QStringList availableEngines();

  /// Creates a named fir engine (or return NULL)
  static FitEngine * createEngine(const QString & name, FitData * data);

  /// Returns the named factory item - or NULL if there isn't.
  static FitEngineFactoryItem * namedFactoryItem(const QString & name);


  /// Creates the commands for setting the enging for the given
  /// workspace.
  static QList<Command *> createCommands(FitWorkspace * workspace);

  /// Returns the default factory item for the given number of
  /// buffers.
  static FitEngineFactoryItem * defaultFactoryItem(int nb = 1);

  /// Initialize the fit engine and set the initial parameters
  virtual void initialize(const double * initialGuess) = 0;

  /// Returns the current parameters
  virtual const gsl_vector * currentParameters() const = 0;

  /// Copies the current parameters to \a target
  virtual void copyCurrentParameters(gsl_vector * target) const;

  /// Computes the covariance matrix into target, which should be an n
  /// x n matrix (n being the number of free parameters)
  virtual void computeCovarianceMatrix(gsl_matrix * target) const = 0;

  /// Performs the next iteration.
  virtual int iterate() = 0;

  /// Whether or not the fit engine is weighting individual points on
  /// its own. If off, then FitData will perform the weighting on its
  /// own.
  ///
  /// Off for all but ODRpack
  virtual bool handlesWeights() const;

  /// Returns the residuals as computed by the last step
  ///
  /// Residuals are the square root of the sum of squares.
  virtual double residuals() const = 0;

  /// The number of iterations since the last call to initialize();
  int iterations;


  /// Recomputes the jacobian with the current parameters. Can be used
  /// to compute the errors.
  ///
  /// @warning This is almost never implemented ;-)...
  virtual void recomputeJacobian();

  /// @name Fit engine parameters manipulations
  ///
  /// @{

  /// Resets the parameters to their defaults.
  ///
  /// Essentially useless with the current implementation, but... 
  virtual void resetEngineParameters();

  /// Returns the option list for the engine.
  ///
  /// Better make that a static list.
  ///
  /// Can be NULL.
  virtual ArgumentList * engineOptions() const;

  /// Retrieve the current fit engine parameters as a ValueHash
  virtual CommandOptions getEngineParameters() const;

  /// Sets the parameters from a ValueHash
  virtual void setEngineParameters(const CommandOptions & params);

  /// @}
};


  // /// Returns a message pertaining to the status returned by iterate()
  // virtual QString message();
  // ///
  // /// Return code as following:
  // /// 0 -> continue iteration process
  // /// 1 -> everything went fine
  // /// 2 -> some error occurred
  // ///
  // /// In cases 1 and 2, more information can be obtained using the
  // /// message() function

#endif
