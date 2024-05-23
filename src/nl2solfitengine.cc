/**
   \file nl2solfitengine.cc
   An implementation of the NL2SOL fit engine designed to be fast on large number of datasets
   Copyright 2024 by CNRS/AMU

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

#include <sparsejacobian.hh>
#include <fitparameter.hh>

#include <abdmatrix.hh>
#include <argumentlist.hh>
#include <general-arguments.hh>

#include <debug.hh>

/// An implementation of the NL2SOL fit engine, in which an approximation
/// of the Hessian is being maintained.'
class NL2SolFitEngine : public FitEngine {
protected:

  /// J^T J, a n x n matrix
  ABDMatrix * jTj;

  /// THe current approximation of the Hessian, maintained step by
  /// step.
  ABDMatrix * sK;

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
  SparseJacobian * jacobian;

  /// The previous jacobian
  SparseJacobian * previousJacobian;

  /// A permutation for the LU decomposition
  gsl_permutation * perm;

  /// Various n vectors.
  gsl_vector * vectors[6];

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

  /// A temporary test vector
  gsl_vector *& testv;


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

  /// The last residuals !
  double lastResiduals;

  /// The number of successive iterations in which we lowered
  /// lambda. This is used to lower lambda faster -- to some extent.
  int successfulIterations;

  /// The number of successful "S" steps
  int successfulSSteps;


public:

  /// Static options
  static ArgumentList options;


  NL2SolFitEngine(FitData * data)  :
    FitEngine(data),
    function(fv[0]),
    testf(fv[1]),
    testf2(fv[2]),
    parameters(vectors[0]),
    gradient(vectors[1]),
    deltap(vectors[2]),
    testp(vectors[3]),
    testp2(vectors[4]),
    testv(vectors[5])
  {
    n = fitData->freeParameters();
    m = fitData->dataPoints();
    // Do a bit of allocation

    jacobian = new SparseJacobian(fitData);
    previousJacobian = new SparseJacobian(fitData);
    perm = gsl_permutation_alloc(n);

    for(size_t i = 0; i < sizeof(fv)/sizeof(gsl_vector *); i++)
      fv[i] = gsl_vector_alloc(m);

    for(size_t i = 0; i < sizeof(vectors)/sizeof(gsl_vector *); i++)
      vectors[i] = gsl_vector_alloc(n);

    QList<int> sizes;
    for(int i = -1; i < fitData->datasets.size(); i++) {
      int sz = fitData->parametersByDataset[i].size();
      if(sz > 0)
        sizes << sz;
    }
    jTj = new ABDMatrix(sizes);
    cur = new ABDMatrix(sizes);
    sK  = new ABDMatrix(sizes);
    sK->clear();

    resetEngineParameters();
  };

  virtual ~NL2SolFitEngine() {
      delete jacobian;
      delete previousJacobian;
      gsl_permutation_free(perm);

      for(size_t i = 0; i < sizeof(vectors)/sizeof(gsl_vector *); i++)
        gsl_vector_free(vectors[i]);

      for(size_t i = 0; i < sizeof(fv)/sizeof(gsl_vector *); i++)
        gsl_vector_free(fv[i]);

      delete jTj;
      delete cur;
      delete sK;
  };


  virtual void initialize(const double * initialGuess) override {
    fitData->packParameters(initialGuess, parameters);
    iterations = 0;
    successfulIterations = 0;
    lastResiduals = -1;
  };

  virtual const gsl_vector * currentParameters() const override {
    return parameters;
  };

  virtual void computeCovarianceMatrix(gsl_matrix * target) const override {
    jTj->invert(target);
  };



protected:
  /// Makes a trial step at the given value of lambda, and store the
  /// results in:
  /// @li @a params for the parameters found
  /// @li @a func for the resulting value
  /// @li @a res for the sum of squares
  /// @li @a expDelta for storing the expected change in residuals if the
  /// problem was fully linear
  /// @li if @a useHessian is true, then the current approximation of
  /// the Hessian is used. Else it's just a classical Levenberg-Marquardt
  /// step
  ///
  /// It assumes that the gradient vector and the jTj matrix are
  /// correct.
  void trialStep(double l, gsl_vector * params, 
                 gsl_vector * func, double * res,
                 double * expDelta,
                 bool useHessian) {
    cur->copyFrom(*jTj);
    cur->addToDiagonal(l);
    if(useHessian)
      cur->add(*sK);

    gsl_vector_memcpy(deltap, gradient);
    cur->solve(deltap);


    if(fitData->debug > 0) {
      // Dump the jTj matrix:
      Debug::debug()
        << "Trial step " << (useHessian ? "using the Hessian " : "")
        << "at lambda = " << l
        << "\ncurrent: \t" << Utils::vectorString(parameters)
        << "\ngradient:\t" << Utils::vectorString(gradient)
        << "\nstep:    \t" << Utils::vectorString(deltap) << endl;
    }

    // Compute the expected delta
    cur->copyFrom(*jTj);
    if(useHessian)
      cur->add(*sK);

    gsl_blas_ddot(gradient, deltap, expDelta);
    *expDelta *= -2;

    cur->apply(deltap, testv);
    double val = 0.0;
    gsl_blas_ddot(testv, deltap, &val);
    *expDelta += val;


    // The the step:
    gsl_vector_memcpy(params, parameters);
    gsl_vector_add(params, deltap);

    fitData->f(params, func);
    gsl_blas_ddot(func, func, res);

    if(fitData->debug > 0) {
      // Dump the jTj matrix:
      Debug::debug()
        << " -> residuals = " << *res << endl;
    }

    if(! std::isfinite(*res))
      throw RuntimeError("Residuals not finite");
  };

public:

  virtual int iterate() override {

    // Here we should update the S matrix if this is not the first
    // iteration
    if(iterations > 0) {
    }
    
    iterations++;
    // Here is where the fun comes in !
  
    // First, we compute the function and the jacobian
    fitData->fdf(parameters, function, jacobian);

    double cur_squares = Utils::finiteNorm(function);

    lastResiduals = sqrt(cur_squares);


    // Now, compute the gradient:
    // g = P^T (Y - f0)

    /// @warning in fact, the gradient is not the gradient but it's
    /// opposite

    jacobian->computeGradient(gradient, function, -1);
    jacobian->computejTj(jTj);

    // OK, so now we go through various tries.
    int nbtries = 0;
    while(true) {
    
      // Compute target steps:
      double ns = 0;
      double expd = 0;
      double nsk = 0;
      double expdk = 0;

      bool didFirst = false;
      try {
        if(fitData->debug > 0) {
          Debug::debug()
            << "Current residuals: " << cur_squares << endl;
        }
      
        trialStep(lambda, testp, testf, &ns, &expd, false);
        trialStep(lambda, testp2, testf, &nsk, &expdk, true);
        didFirst = true;
      }
      catch(const RuntimeError & re) {
        // Try a smaller step, or, in other words, increase lambda.
        nsk = 2 * cur_squares;
        ns = 2 * cur_squares;
        if(! didFirst)
          lambda *= scale;
      }

      // First, let's determine which step was better
      if(ns >= cur_squares && nsk >= cur_squares) {
        // none of the steps were better, we make the trust region smaller
        
        nbtries ++;
        int mt = (iterations == 1 ? maxTries + 10 : maxTries);
        if(nbtries > mt)
          throw RuntimeError("Failed to find a suitable step after %1 tries").
            arg(nbtries);
        lambda *= scale;
        continue;                 // Try again !
      }
      successfulIterations += 1;
      if(fitData->debug > 0) {
        Debug::debug() << "Sucessful step:\n" 
                       << " * regular: " << cur_squares-ns << " -- "
                       << expd << " expected\n"
                       << " * hessian: " << cur_squares-nsk << " -- "
                       << expdk << " expected" << endl;
      }

      // The "S" step was better than the classical LM step
      if(nsk < ns) {
        gsl_vector_memcpy(testp, testp2);
        ns = nsk;
        successfulSSteps += 1;
      }
      // We increase the trust region
      lambda /= scale;

      std::swap(jacobian, previousJacobian);

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
          Debug::debug()
            << "Continuing because residuals variation too large: "
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
            Debug::debug()
              << "Continuing because variation of param #" << i <<" too large: "
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
  };
  
  virtual double residuals() const override {
    return lastResiduals;
  };
  
  virtual void recomputeJacobian() override {
    fitData->fdf(parameters, function, jacobian);

    jacobian->computejTj(jTj);

    lastResiduals = gsl_blas_dnrm2(function);
  };

  virtual CommandOptions getEngineParameters() const override {
    CommandOptions val;
    updateOptions(val, "lambda", lambda);
    updateOptions(val, "scale", scale);
    updateOptions(val, "end-threshold", endThreshold);
    updateOptions(val, "relative-min", relativeMin);
    updateOptions(val, "trial-steps", maxTries);
    updateOptions(val, "residuals-threshold", residualsThreshold);

    return val;
  };
  
  virtual void setEngineParameters(const CommandOptions & val) override {
    updateFromOptions(val, "lambda", lambda);
    updateFromOptions(val, "scale", scale);
    updateFromOptions(val, "end-threshold", endThreshold);
    updateFromOptions(val, "relative-min", relativeMin);
    updateFromOptions(val, "trial-steps", maxTries);
    updateFromOptions(val, "residuals-threshold", residualsThreshold);
  };

  virtual ArgumentList * engineOptions() const override {
    return &options;
  };

  virtual void resetEngineParameters() override {
    lambda = 1e-4;
    scale = 2;
    endThreshold = 1e-5;
    relativeMin = 1e-3;
    residualsThreshold = 1e-5;
    maxTries = 30;
  };
};


ArgumentList NL2SolFitEngine::
options(QList<Argument*>()
        << new NumberArgument("lambda", "Lambda")
        << new NumberArgument("scale", "Scale")
        << new NumberArgument("end-threshold", "Threshold for ending")
        << new NumberArgument("relative-min",
                              "Min value for relative differences")
        << new IntegerArgument("trial-steps",
                               "Maximum number of trial steps")
        << new NumberArgument("residuals-threshold",
                              "Threshold for relative changes to residuals"));


static FitEngine * nl2solFE(FitData * d)
{
  return new NL2SolFitEngine(d);
}

static FitEngineFactoryItem nl2sol("nl2sol", "NL2SOL",
                                   &nl2solFE,
                                   &NL2SolFitEngine::options, true);


//////////////////////////////////////////////////////////////////////
#include <credits.hh>

static Credits nl2solC("Dennis et al, ACM Trans Math Soft, 1981",
                       "NL2SOL non linear least squares engine",
                       "10.1145/355958.355965");

