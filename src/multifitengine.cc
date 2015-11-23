/**
   \file multifitengine.cc
   A Levenberg-Marquardt fit engine for QSoas, with emphasis on fast massive multi-buffer fits
   Copyright 2013-2014 by AMU/CNRS

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

#include <abdmatrix.hh>
#include <argumentlist.hh>
#include <general-arguments.hh>

/// A fit engine with an emphasis on fast fitting for massively multi
/// stuff.
class MultiFitEngine : public FitEngine {
protected:

  /// jacobian^T jacobian, a n x n matrix, also called A in the
  /// Marquardt paper. Here implemented using the special almost
  /// block-diagonal matrix.
  ABDMatrix * jTj;

  /// What is actually inverted in the trial steps.
  ABDMatrix * cur;

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

  /// @}

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


  /// Static options
  static ArgumentList * options;

public:

  MultiFitEngine(FitData * data);
  virtual ~MultiFitEngine();


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

MultiFitEngine::MultiFitEngine(FitData * data) :
  FitEngine(data), 
  function(fv[0]),
  testf(fv[1]),
  testf2(fv[2]),
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

  QList<int> sizes;
  // QTextStream o(stdout);
  for(int i = -1; i < fitData->datasets.size(); i++) {
    int sz = fitData->parametersByDataset[i].size();
    if(sz > 0)
      sizes << sz;
  }
  jTj = new ABDMatrix(sizes);
  cur = new ABDMatrix(sizes);

  resetEngineParameters();
}


MultiFitEngine::~MultiFitEngine()
{
  gsl_matrix_free(jacobian);
  gsl_permutation_free(perm);

  for(size_t i = 0; i < sizeof(vectors)/sizeof(gsl_vector *); i++)
    gsl_vector_free(vectors[i]);

  for(size_t i = 0; i < sizeof(fv)/sizeof(gsl_vector *); i++)
    gsl_vector_free(fv[i]);

  for(size_t i = 0; i < sizeof(matrices)/sizeof(gsl_matrix *); i++)
    gsl_matrix_free(matrices[i]);

  delete jTj;
  delete cur;
}

void MultiFitEngine::resetEngineParameters()
{
  lambda = 1e-2;
  scale = 2;
  endThreshold = 1e-5;
  relativeMin = 1e-3;
}

ArgumentList * MultiFitEngine::options = NULL;

ArgumentList * MultiFitEngine::engineOptions() const
{
  if(! options) {
    options = new ArgumentList;
    *options << new NumberArgument("lambda", "Lambda")
             << new NumberArgument("scale", "Scale")
             << new NumberArgument("end-threshold", "Threshold for ending")
             << new NumberArgument("relative-min", "Min value for relative differences");
  }
  return options;
}

CommandOptions MultiFitEngine::getEngineParameters() const
{
  CommandOptions val;
  updateOptions(val, "lambda", lambda);
  updateOptions(val, "scale", scale);
  updateOptions(val, "end-threshold", endThreshold);
  updateOptions(val, "relative-min", relativeMin);

  return val;
}

void MultiFitEngine::setEngineParameters(const CommandOptions & val)
{
  updateFromOptions(val, "lambda", lambda);
  updateFromOptions(val, "scale", scale);
  updateFromOptions(val, "end-threshold", endThreshold);
  updateFromOptions(val, "relative-min", relativeMin);
}


void MultiFitEngine::initialize(const double * initialGuess)
{
  fitData->packParameters(initialGuess, parameters);
  iterations = 0;

  lastResiduals = -1;
}

const gsl_vector * MultiFitEngine::currentParameters() const
{
  return parameters;
}

void MultiFitEngine::recomputeJacobian()
{
  fitData->fdf(parameters, function, jacobian);
  jTj->setFromProduct(jacobian);

  double cur_squares = 0;
  gsl_blas_ddot(function, function, &cur_squares);

  lastResiduals = sqrt(cur_squares);
}


void MultiFitEngine::computeCovarianceMatrix(gsl_matrix * target) const
{
  /// @todo Recompute the jacobian when necessary ?
  // fitData->df(parameters, jacobian);
  // gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1, 
  //                jacobian, jacobian, 0, jTj);

  // jTj->expandToFullMatrix(matrices[0]);

  jTj->invert(target);

  // // Tolerance of 1e-7 for the singular values.
  // Utils::invertMatrix(matrices[0], target, 1e-7);
}

void MultiFitEngine::trialStep(double l, gsl_vector * params, 
                               gsl_vector * func, double * res)
{
  cur->copyFrom(*jTj);
  cur->addToDiagonal(l);

  gsl_vector_memcpy(deltap, gradient);
  cur->solve(deltap);

  // The the step:
  gsl_vector_memcpy(params, parameters);
  gsl_vector_add(params, deltap);

  fitData->f(params, func);
  gsl_blas_ddot(func, func, res);
}

int MultiFitEngine::iterate() 
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

  jTj->setFromProduct(jacobian);

  if(fitData->debug) {
    // Dump the jTj matrix:
    QTextStream o(stdout);
    // o << "jTj matrix: \n"
    //   << Utils::matrixString(jTj) << endl;
  }

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
      if(fabs(dp)/(relativeMin + fabs(p)) > endThreshold)
        return GSL_CONTINUE;
    }
    return GSL_SUCCESS;
    
  }
  // In principle never reached.
  return 0;
}

double MultiFitEngine::residuals() const
{
  return lastResiduals;
}

static FitEngine * multiFE(FitData * d)
{
  return new MultiFitEngine(d);
}

static FitEngineFactoryItem lmsder("multi", "Multi", &multiFE);