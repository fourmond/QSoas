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


#ifndef __FITENGINE_HH
#define __FITENGINE_HH

class Fit;  
class FitData;

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

public:
  FitEngine(FitData * data);
  virtual ~FitEngine();


  /// Initialize the fit engine and set the initial parameters
  virtual void initialize(const double * initialGuess) = 0;

  /// Returns the current parameters
  virtual const gsl_vector * currentParameters() const = 0;

  /// Copies the current parameters to \a target
  virtual void copyCurrentParameters(gsl_vector * target) const;

  /// Computes the covariance matrix into target, which should be an n
  /// x n matrix (n being the number of free parameters)
  virtual void computeCovarianceMatrix(gsl_matrix * target) const = 0;

  /// Performs the next iteration (possibly cheating). 
  virtual int iterate() = 0;

  /// Returns the residuals as computed by the last step
  virtual double residuals() const = 0;

  /// The number of iterations since the last call to initialize();
  int iterations;

};


class GSLFitEngine : public FitEngine {
  
  /// The solver in use
  gsl_multifit_fdfsolver * solver;

  /// The function in use
  gsl_multifit_function_fdf function;

protected:

  static int staticF(const gsl_vector * x, void * params, gsl_vector * f);
  static int staticDf(const gsl_vector * x, void * params, gsl_matrix * df);
  static int staticFdf(const gsl_vector * x, void * params, gsl_vector * f,
                       gsl_matrix * df);

public:

  GSLFitEngine(FitData * data, const gsl_multifit_fdfsolver_type * T = 
               gsl_multifit_fdfsolver_lmsder);
  virtual ~GSLFitEngine();


  virtual void initialize(const double * initialGuess);
  virtual const gsl_vector * currentParameters() const;
  virtual void computeCovarianceMatrix(gsl_matrix * target) const;
  virtual int iterate();
  virtual double residuals() const;
};

#endif
