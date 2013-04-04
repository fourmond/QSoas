/**
   \file fitengine.hh
   Base class for engine driving the fits
   Copyright 2012 by Vincent Fourmond

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

class Fit;  
class FitData;
class FitEngine;


class FitEngineFactoryItem {
public:
  typedef FitEngine * (*Creator)(FitData * data);

  /// The creation function
  Creator creator;

  /// The creation name
  QString name;

  /// The public name
  QString publicName;
  /// Creates and register a factory item.
  FitEngineFactoryItem(const QString & n, const QString & pn, Creator c);
};


/// A simple wrapper class around parameters found after an iteration.
class StoredParameters {

  /// A view around the current parameters.
  gsl_vector_view view;
public:

  /// The parameters
  Vector parameters;

  /// The corresponding residuals
  double residuals;

  StoredParameters() {;};
  StoredParameters(const Vector & v, double r) : 
    parameters(v), residuals(r) {;};
  StoredParameters(const gsl_vector *, double r);

  /// Compares on residuals
  bool operator<(const StoredParameters & o) const {
    return residuals < o.residuals;
  };

  /// Returns a gsl vector for the 
  gsl_vector * toGSL();

};



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

  friend class FitEngineFactoryItem;

  /// The application-wide FitEngine factory
  static QHash<QString, FitEngineFactoryItem*> * factory;

  static void registerFactoryItem(FitEngineFactoryItem * item);


  /// The underlying fit data
  FitData * fitData;

  /// Stored parameters
  QList<StoredParameters> storedParameters;

  /// Pushes the current parameters onto the stack. It uses
  /// currentParameters() and residuals();
  void pushCurrentParameters();

public:
  FitEngine(FitData * data);
  virtual ~FitEngine();

  /// Returns the short name of all available fit engines
  static QStringList availableEngines();

  /// Creates a named fir engine (or return NULL)
  static FitEngine * createEngine(const QString & name, FitData * data);

  /// Returns the named factory item - or NULL if there isn't.
  static FitEngineFactoryItem * namedFactoryItem(const QString & name);

  /// Returns the default factory item.
  static FitEngineFactoryItem * defaultFactoryItem();

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

  /// Returns the residuals as computed by the last step
  ///
  /// Residuals are the square root of the sum of squares.
  virtual double residuals() const = 0;

  /// The number of iterations since the last call to initialize();
  int iterations;

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
