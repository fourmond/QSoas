/**
   \file qsoasfitengine.cc
   A Levenberg-Marquardt fit engine for QSoas
   Copyright 2013 by Vincent Fourmond

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

/// QSoas's own fit engine
class QSoasFitEngine : public FitEngine {
protected:

  /// The number of points
  int m;

  /// The number of parameters
  int n;

  /// Various m vectors.
  gsl_vector * fv[3];

  /// The current evaluation of the function (ie length m)
  gsl_vector *& function;

  /// Trial function at target step
  gsl_vector *& testf;

  /// Second trial function at target step
  gsl_vector *& testf2;

  /// The m x n jacobian
  gsl_matrix * jacobian;

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

  /// The lambda parameter
  double lambda;

  /// The scaling factor (called nu in the original paper)
  double scale;

  /// The last residuals !
  double lastResiduals;

  
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

  
public:

  QSoasFitEngine(FitData * data);
  virtual ~QSoasFitEngine();


  virtual void initialize(const double * initialGuess);
  virtual const gsl_vector * currentParameters() const;
  virtual void computeCovarianceMatrix(gsl_matrix * target) const;
  virtual int iterate();
  virtual double residuals() const;
  virtual void recomputeJacobian();
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

  jacobian = gsl_matrix_alloc(m, n);
  perm = gsl_permutation_alloc(n);

  for(size_t i = 0; i < sizeof(fv)/sizeof(gsl_vector *); i++)
    fv[i] = gsl_vector_alloc(m);

  for(size_t i = 0; i < sizeof(vectors)/sizeof(gsl_vector *); i++)
    vectors[i] = gsl_vector_alloc(n);

  for(size_t i = 0; i < sizeof(matrices)/sizeof(gsl_matrix *); i++)
    matrices[i] = gsl_matrix_alloc(n,n);
}


QSoasFitEngine::~QSoasFitEngine()
{
  gsl_matrix_free(jacobian);
  gsl_permutation_free(perm);

  for(size_t i = 0; i < sizeof(vectors)/sizeof(gsl_vector *); i++)
    gsl_vector_free(vectors[i]);

  for(size_t i = 0; i < sizeof(fv)/sizeof(gsl_vector *); i++)
    gsl_vector_free(fv[i]);

  for(size_t i = 0; i < sizeof(matrices)/sizeof(gsl_matrix *); i++)
    gsl_matrix_free(matrices[i]);
}


void QSoasFitEngine::initialize(const double * initialGuess)
{
  fitData->packParameters(initialGuess, parameters);
  iterations = 0;

  lambda = 1e-2;
  scale = 2;

  lastResiduals = -1;
}

const gsl_vector * QSoasFitEngine::currentParameters() const
{
  return parameters;
}

void QSoasFitEngine::recomputeJacobian()
{
  fitData->fdf(parameters, function, jacobian);
  gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1, 
                 jacobian, jacobian, 0, jTj);
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

  // Tolerance of 1e-7 for the singular values.
  Utils::invertMatrix(cur, target, 1e-7);
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

  // The the step:
  gsl_vector_memcpy(params, parameters);
  gsl_vector_add(params, deltap);

  fitData->f(params, func);
  gsl_blas_ddot(func, func, res);
}

int QSoasFitEngine::iterate() 
{
  iterations++;
  // Here is where the fun comes in !
  
  // First, we compute the function and the jacobian

  /// @todo It is crucial that the derivatives are computed correctly !
  ///
  fitData->fdf(parameters, function, jacobian);

  double cur_squares = 0;
  gsl_blas_ddot(function, function, &cur_squares);

  lastResiduals = sqrt(cur_squares);


  // Now, compute the gradient:
  // g = P^T (Y - f0)

  /// @warning function is the negative of what is expected -
  /// therefore using a -1 sign here !
  gsl_blas_dgemv(CblasTrans, -1, 
                 jacobian, function, 0, gradient);

  // Then, compute the P^TP matrix:
  gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1, 
                 jacobian, jacobian, 0, jTj);

  // OK, so now we go through various tries.
  int nbtries = 0;
  while(true) {
    
    // Compute target step:

    double ns = 0;
    double ns2 = 0;
    bool didFirst = false;
    try {
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
    }

    // Heh !
    if(ns2 < cur_squares) {
      // Iteration went fine
      lambda /= scale;
      gsl_vector_memcpy(testp, testp2);
      ns = ns2;
    }
    else if(ns > cur_squares) {
      // Here, it goes bad:
      nbtries ++;
      if(nbtries > 30) 
        throw RuntimeError("Failed to find a suitable step after %1 tries").
          arg(nbtries);
      lambda *= scale;
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
    
    for(int i = 0; i < n; i++) {
      double dp = gsl_vector_get(deltap, i);
      double p = gsl_vector_get(parameters, i);
      if(fabs(dp)/(1e-3 + fabs(p)) > 1e-5)
        return GSL_CONTINUE;
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
