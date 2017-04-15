/**
   \file qsoasfitengine.cc
   A Levenberg-Marquardt fit engine for QSoas
   Copyright 2013 by CNRS/AMU

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
#include <fitdata.hh>
#include <fitengine.hh>
#include <exceptions.hh>
#include <utils.hh>

#include <argumentlist.hh>
#include <general-arguments.hh>
#include <debug.hh>

#include <sparsejacobian.hh>
#include <fitparameter.hh>

/// QSoas's own fit engine
class QSoasFitEngine : public FitEngine {
protected:

  /// The number of points
  int m;

  /// The number of parameters
  int n;

  /// The number of successive iterations in which we lowered
  /// lambda. This is used to lower lambda faster -- to some extent.
  int successfulIterations;

  /// Various m vectors.
  gsl_vector * fv[3];


  /// Whether we use scaling or not.
  bool useScaling;
  
  /// The scaling factors for the jacobian and the steps
  gsl_vector * scalingFactors;

  /// The current evaluation of the function (ie length m)
  gsl_vector *& function;

  /// Trial function at target step
  gsl_vector *& testf;

  /// Second trial function at target step
  gsl_vector *& testf2;

  /// The m x n jacobian
  ///
  /// If scaling is on, this matrix is always scaled.
  ///
  ///  (is it ?)
  SparseJacobian * jacobian;

  /// Various temporary n x n matrices
  ///
  /// @todo use std::array ?
  gsl_matrix * matrices[3];

  /// jacobian^T jacobian, a n x n matrix, also called A in the
  /// Marquardt paper.
  gsl_matrix *& jTj;

  /// the tentative matrix to inverse to get the correct direction.
  gsl_matrix *& cur;

  /// A permutation for the LU decomposition
  gsl_permutation * perm;

  /// Various n vectors.
  gsl_vector * vectors[5];

  /// The current parameters
  gsl_vector *& parameters;

  /// The computed gradient
  gsl_vector *& gradient;

  /// The test variation in parameters
  gsl_vector *& deltap;

  /// The test parameters
  gsl_vector *& testp;

  /// Second test parameters
  gsl_vector *& testp2;


  /// The last residuals !
  double lastResiduals;

  /// @name Internal parameters
  ///
  /// @{
  
  /// The lambda parameter
  double lambda;

  /// The scaling factor (called nu in the original paper)
  double scale;

  /// The threshold difference between two steps that trigger a stop
  double endThreshold;

  /// The minimum value for relative difference
  double relativeMin;

  /// The relative threshold of residuals variation that triggers a
  /// continue response
  double residualsThreshold;

  /// The maximum number of tries
  int maxTries;

  /// @}
  
  /// Makes a trial step at the given value of lambda, and store the
  /// results in:
  /// @li @a params for the parameters found
  /// @li @a func for the resulting value
  /// @li @a res for the sum of squares
  ///
  /// It assumes that the gradient vector and the jTj matrix are
  /// correct.
  void trialStep(double l, gsl_vector * params, 
                 gsl_vector * func, double * res);

  /// If \a useScaling is on, determines the scaling factor and
  /// applies them to the jacobian.
  ///
  /// No-op if the \a useScaling is false.
  void scaleJacobian();


  /// Static options
  static ArgumentList * options;
  
public:

  QSoasFitEngine(FitData * data);
  virtual ~QSoasFitEngine();


  virtual void initialize(const double * initialGuess);
  virtual const gsl_vector * currentParameters() const;
  virtual void computeCovarianceMatrix(gsl_matrix * target) const;
  virtual int iterate();
  virtual double residuals() const;
  virtual void recomputeJacobian();

  virtual CommandOptions getEngineParameters() const;
  virtual ArgumentList * engineOptions() const;
  virtual void resetEngineParameters();
  virtual void setEngineParameters(const CommandOptions & params);

};

QSoasFitEngine::QSoasFitEngine(FitData * data) :
  FitEngine(data), 
  function(fv[0]),
  testf(fv[1]),
  testf2(fv[2]),
  jTj(matrices[0]),
  cur(matrices[1]),
  parameters(vectors[0]),
  gradient(vectors[1]),
  deltap(vectors[2]),
  testp(vectors[3]),
  testp2(vectors[4])
{
  n = fitData->freeParameters();
  m = fitData->dataPoints();
  // Do a bit of allocation

  jacobian = new SparseJacobian(fitData);
  perm = gsl_permutation_alloc(n);

  for(size_t i = 0; i < sizeof(fv)/sizeof(gsl_vector *); i++)
    fv[i] = gsl_vector_alloc(m);

  for(size_t i = 0; i < sizeof(vectors)/sizeof(gsl_vector *); i++)
    vectors[i] = gsl_vector_alloc(n);

  /// The scaling factors
  scalingFactors = gsl_vector_alloc(n);

  for(size_t i = 0; i < sizeof(matrices)/sizeof(gsl_matrix *); i++)
    matrices[i] = gsl_matrix_alloc(n,n);

  resetEngineParameters();
}


QSoasFitEngine::~QSoasFitEngine()
{
  delete jacobian;
  gsl_permutation_free(perm);

  for(size_t i = 0; i < sizeof(vectors)/sizeof(gsl_vector *); i++)
    gsl_vector_free(vectors[i]);

  for(size_t i = 0; i < sizeof(fv)/sizeof(gsl_vector *); i++)
    gsl_vector_free(fv[i]);

  gsl_vector_free(scalingFactors);

  for(size_t i = 0; i < sizeof(matrices)/sizeof(gsl_matrix *); i++)
    gsl_matrix_free(matrices[i]);
}


void QSoasFitEngine::initialize(const double * initialGuess)
{
  fitData->packParameters(initialGuess, parameters);
  iterations = 0;
  successfulIterations = 0;
  lastResiduals = -1;
}


void QSoasFitEngine::resetEngineParameters()
{
  lambda = 1e-4;
  scale = 2;
  endThreshold = 1e-5;
  relativeMin = 1e-3;
  residualsThreshold = 1e-5;
  maxTries = 30;
  useScaling = false;
}

ArgumentList * QSoasFitEngine::options = NULL;

ArgumentList * QSoasFitEngine::engineOptions() const
{
  if(! options) {
    options = new ArgumentList;
    *options << new NumberArgument("lambda", "Lambda")
             << new NumberArgument("scale", "Scale")
             << new NumberArgument("end-threshold", "Threshold for ending")
             << new NumberArgument("relative-min", "Min value for relative differences")
             << new IntegerArgument("trial-steps", "Maximum number of trial steps")
             << new BoolArgument("scaling", "Jacobian scaling")
             << new NumberArgument("residuals-threshold", "Threshold for relative changes to residuals");
  }
  return options;
}


CommandOptions QSoasFitEngine::getEngineParameters() const
{
  CommandOptions val;
  updateOptions(val, "lambda", lambda);
  updateOptions(val, "scale", scale);
  updateOptions(val, "end-threshold", endThreshold);
  updateOptions(val, "relative-min", relativeMin);
  updateOptions(val, "trial-steps", maxTries);
  updateOptions(val, "scaling", useScaling);
  updateOptions(val, "residuals-threshold", residualsThreshold);

  return val;
}


void QSoasFitEngine::setEngineParameters(const CommandOptions & val)
{
  updateFromOptions(val, "lambda", lambda);
  updateFromOptions(val, "scale", scale);
  updateFromOptions(val, "end-threshold", endThreshold);
  updateFromOptions(val, "relative-min", relativeMin);
  updateFromOptions(val, "trial-steps", maxTries);
  updateFromOptions(val, "scaling", useScaling);
  updateFromOptions(val, "residuals-threshold", residualsThreshold);
}

const gsl_vector * QSoasFitEngine::currentParameters() const
{
  return parameters;
}

void QSoasFitEngine::scaleJacobian()
{
  if(! useScaling)
    return;
  for(int i = 0; i < n; i++) {
    const FreeParameter * fp = fitData->allParameters[i];
    if(fp->fitIndex != i)
      throw InternalError("Fit parameters numbering inconsistency: %1 vs %2").
        arg(fp->fitIndex).arg(i);
    int ds = fp->dsIndex, prm = fp->paramIndex;
    gsl_vector * v = jacobian->parameterVector(prm, ds);
    double nrm = gsl_blas_dnrm2(v);
    if(nrm == 0)
      nrm = 1;
    nrm = 1/nrm;
    gsl_vector_scale(v, nrm);
    gsl_vector_set(scalingFactors, i, nrm);
  }
  if(fitData->debug > 0) {
    // Dump the scaling factors
    Debug::debug() << "Using scaling factors:"
                   << Utils::vectorString(scalingFactors) << endl;
  }
}



void QSoasFitEngine::recomputeJacobian()
{
  fitData->fdf(parameters, function, jacobian);
  scaleJacobian();              // Do scaling all the times
  jacobian->computejTj(jTj);
  double cur_squares = 0;
  gsl_blas_ddot(function, function, &cur_squares);

  lastResiduals = sqrt(cur_squares);
}


void QSoasFitEngine::computeCovarianceMatrix(gsl_matrix * target) const
{
  /// @todo Recompute the jacobian when necessary ?
  // fitData->df(parameters, jacobian);
  // gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1, 
  //                jacobian, jacobian, 0, jTj);

  gsl_matrix_memcpy(cur, jTj);

  // Unscale if applicable
  if(useScaling) {
    for(int i = 0; i < n; i++) {
      double sf = 1/gsl_vector_get(scalingFactors, i);
      gsl_vector_view v = gsl_matrix_column(cur, i);
      gsl_vector_scale(&v.vector, sf);
      v = gsl_matrix_row(cur, i);
      gsl_vector_scale(&v.vector, sf);
    }
  }

  // Tolerance of 1e-7 for the singular values.
  Utils::invertMatrix(cur, target, 1e-14);
}

void QSoasFitEngine::trialStep(double l, gsl_vector * params, 
                               gsl_vector * func, double * res)
{
  // First, with current lambda
  // We prepare the A + lambda I matrix
  gsl_matrix_memcpy(cur, jTj);
  for(int i = 0; i < n; i++)
    gsl_matrix_set(cur, i, i, gsl_matrix_get(cur, i, i) + l);

  int sign = 0;

  /// @todo error checking !
  gsl_linalg_LU_decomp(cur, perm, &sign);

  // First compute the delta^r:
  gsl_linalg_LU_solve(cur, perm, gradient, deltap);

  // Now, scale the result
  if(useScaling)
    gsl_vector_mul(deltap, scalingFactors);
      
  if(fitData->debug > 0) {
    // Dump the jTj matrix:
    Debug::debug()  << "Trial step at lambda = " << l
                    << "\ncurrent: \t" << Utils::vectorString(parameters)
                    << "\ngradient:\t" << Utils::vectorString(gradient)
                    << "\nstep:    \t" << Utils::vectorString(deltap) << endl;
  }

  // The the step:
  gsl_vector_memcpy(params, parameters);
  gsl_vector_add(params, deltap);

  fitData->f(params, func);
  gsl_blas_ddot(func, func, res);
  if(fitData->debug > 0) {
    // Dump the jTj matrix:
    Debug::debug() << " -> residuals = " << *res << endl;
  }

  // If the residuals are NaN, throw an exception
  if(std::isnan(*res))
    throw RuntimeError("NaN in residuals");
}

int QSoasFitEngine::iterate() 
{
  iterations++;
  // Here is where the fun comes in !
  
  // First, we compute the function and the jacobian

  /// @todo It is crucial that the derivatives are computed correctly !
  ///
  fitData->fdf(parameters, function, jacobian);
  scaleJacobian();

  /// If scaling is on, all the following applies to the scaled
  /// jacobian.

  double cur_squares = 0;
  gsl_blas_ddot(function, function, &cur_squares);

  lastResiduals = sqrt(cur_squares);


  // Now, compute the gradient:
  // g = P^T (Y - f0)

  /// @warning function is the negative of what is expected -
  /// therefore using a -1 sign here !
  jacobian->computeGradient(gradient, function, -1);

  // Then, compute the P^TP matrix:
  jacobian->computejTj(jTj);

  if(fitData->debug > 0) {
    // Dump the jTj matrix:
    Debug::debug() << "jTj matrix: \n"
                   << Utils::matrixString(jTj) << endl;
  }

  // OK, so now we go through various tries.
  int nbtries = 0;
  while(true) {
    
    // Compute target step:

    double ns = 0;
    double ns2 = 0;
    bool didFirst = false;
    try {
      if(fitData->debug > 0) {
        Debug::debug() << "Current residuals: " << cur_squares << endl;
      }
      
      trialStep(lambda, testp, testf, &ns);
      didFirst = true;
      /// @todo Compute that step only if it hasn't been computed
      /// already (on the second iteration, for instance, if the
      /// reason why)
      trialStep(lambda/scale, testp2, testf2, &ns2);

      /// @todo Implement the angle limit ?
    }
    catch(const RuntimeError & re) {
      // Try a smaller step, or, in other words, increase lambda.
      ns2 = 2 * cur_squares;
      ns = 2 * cur_squares;
      if(! didFirst)
        lambda *= scale;
      successfulIterations = 0;
    }

    // Heh !
    if(ns2 < cur_squares) {
      // Iteration went fine

      /// @todo Customize all this
      double fact = 1 + successfulIterations * 0.5;
      if(fact > 10)
        fact = 10;
      lambda /= pow(scale, fact);
      successfulIterations += 1;
      gsl_vector_memcpy(testp, testp2);
      ns = ns2;
    }
    else if(ns > cur_squares) {
      // Here, it goes bad:
      nbtries ++;
      int mt = (iterations == 1 ? maxTries + 10 : maxTries);
      if(nbtries > mt) 
        throw RuntimeError("Failed to find a suitable step after %1 tries").
          arg(nbtries);
      lambda *= scale;
      successfulIterations = 0;
      continue;                 // Try again !
      /// @todo Avoid computing two times the thing at lambda by
      /// reusing the current values for the testp2 things.
    }

    // Now, we have the new parameters in testp
    gsl_vector_memcpy(deltap, testp);
    // Yes, we compute the same things twice, although that may give
    // different results than the previous deltap(2)
    gsl_vector_sub(deltap, parameters);

    lastResiduals = sqrt(ns);

    // ANd we finally switch to the new parameters !
    gsl_vector_memcpy(parameters, testp);

    /// Continue because the residuals have changed too much
    if((cur_squares - ns)/(cur_squares) >= residualsThreshold ) {
      if(fitData->debug > 0) {
        Debug::debug() << "Continuing because residuals variation too large: "
                       << (cur_squares - ns)/(cur_squares) << " (real delta: "
                       << (cur_squares - ns) << ")" << endl; 

      }
      return GSL_CONTINUE;
    }

    
    for(int i = 0; i < n; i++) {
      double dp = gsl_vector_get(deltap, i);
      double p = gsl_vector_get(parameters, i);
      if(fabs(dp)/(relativeMin + fabs(p)) > endThreshold) {
        if(fitData->debug > 0) {
          Debug::debug() << "Continuing because variation of param #"
                         << i <<" too large: "
                         << fabs(dp)/(relativeMin + fabs(p)) << " (real delta: "
                         << dp << ")" << endl; 
        }
        return GSL_CONTINUE;
      }
    }
    return GSL_SUCCESS;
    
  }
  // In principle never reached.
  return 0;
}

double QSoasFitEngine::residuals() const
{
  return lastResiduals;
}

static FitEngine * qsoasFE(FitData * d)
{
  return new QSoasFitEngine(d);
}

static FitEngineFactoryItem lmsder("qsoas", "QSoas", &qsoasFE);
