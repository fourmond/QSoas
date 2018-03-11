/**
   \file gslmminfitengine.cc
   The implementation of the simplex algorithm in the GSL
   Copyright 2018 by CNRS/AMU

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

#include <gsl-types.hh>
#include <debug.hh>

#include <gsl/gsl_version.h>
#include <gsl/gsl_multimin.h>

class GSLSimplexFitEngine : public FitEngine {

  /// Number of free parameters;
  int n;

  /// The minimizer used
  gsl_multimin_fminimizer * minimizer;

  /// The function in use
  gsl_multimin_function function;

  /// The current parameters
  mutable GSLVector parameters;

  /// Some storage place
  GSLVector storage, storage2;

  /// scaling with respect to the initial parameters
  GSLVector scalingFactors;

  /// Last evaluation.
  GSLVector eval;

protected:

  double evaluate(const gsl_vector * x) {
    // rescale the vector
    for(int i = 0; i < n; i++)
      storage[i] = gsl_vector_get(x, i) * scalingFactors[i];
        
    fitData->f(storage, eval);
    return gsl_blas_dnrm2(eval);
  };

  static double staticF(const gsl_vector * x, void * params) {
    GSLSimplexFitEngine * engine = static_cast<GSLSimplexFitEngine *>(params);
    return engine->evaluate(x);
  }

public:

  GSLSimplexFitEngine(FitData * data) :
    FitEngine(data), n(fitData->freeParameters()),
    minimizer(NULL),
    parameters(n), storage(n), storage2(n), scalingFactors(n),
    eval(data->dataPoints())
  {
    function.f = GSLSimplexFitEngine::staticF;
    function.n = n;
    function.params = this;
  }
  
  virtual ~GSLSimplexFitEngine() {
    if(minimizer)
      gsl_multimin_fminimizer_free(minimizer);
  };


  virtual void initialize(const double * initialGuess) override {
    fitData->packParameters(initialGuess, parameters);
    for(int i = 0; i < n; i++) {
      double v = parameters[i];
      if(v == 0)
        v = 1;
      scalingFactors[i] = v;
      storage[i] = parameters[i]/v;
      storage2[i] = 0.5;        // initial step size
    }

    // now we allocate the solver
    minimizer =
      gsl_multimin_fminimizer_alloc(gsl_multimin_fminimizer_nmsimplex2, n);
    gsl_multimin_fminimizer_set(minimizer, &function, storage,storage2);
  };
  
  
  virtual const gsl_vector * currentParameters() const {
    gsl_vector * v = gsl_multimin_fminimizer_x(minimizer);
    for(int i = 0; i < n; i++)
      parameters[i] = gsl_vector_get(v, i) * scalingFactors[i];
    return parameters;
  };

  virtual void computeCovarianceMatrix(gsl_matrix * target) const override {
  };

  virtual int iterate() override {
    int status = gsl_multimin_fminimizer_iterate(minimizer);
      
    if(status) 
      return status;

    double size = gsl_multimin_fminimizer_size (minimizer);
    status = gsl_multimin_test_size (size, 1e-3);
    return status;
  }
  
  virtual double residuals() const override {
    // This assumes that the "eval" vector is always the last step.
    return gsl_blas_dnrm2(eval);
  };
};


static FitEngine * gsls(FitData * d)
{
  return new GSLSimplexFitEngine(d);
}

static FitEngineFactoryItem gslSimplex("gsl-simplex", "GSL implementation of the simplex", &gsls);
