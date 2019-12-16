/**
   \file gslfitengine.cc
   The GSL-based fit engine
   Copyright 2011 by Vincent Fourmond
             2012 by CNRS/AMU

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

#include <sparsejacobian.hh>
#include <debug.hh>

#include <gsl/gsl_version.h>
#include <gsl/gsl_multifit_nlin.h>

class GSLFitEngine : public FitEngine {
  
  /// The solver in use
  gsl_multifit_fdfsolver * solver;

  /// The function in use
  gsl_multifit_function_fdf function;

  /// The current scaling factor of the jacobian...
  double jacobianScalingFactor;

protected:

  static int staticF(const gsl_vector * x, void * params, gsl_vector * f);
  static int staticDf(const gsl_vector * x, void * params, gsl_matrix * df);
  static int staticFdf(const gsl_vector * x, void * params, gsl_vector * f,
                       gsl_matrix * df);

public:

  GSLFitEngine(FitData * data, const gsl_multifit_fdfsolver_type * T = 
               gsl_multifit_fdfsolver_lmsder);
  virtual ~GSLFitEngine();


  virtual void initialize(const double * initialGuess) override;
  virtual const gsl_vector * currentParameters() const override;
  virtual void computeCovarianceMatrix(gsl_matrix * target) const override;
  virtual int iterate() override;
  virtual double residuals() const override;
};

GSLFitEngine::GSLFitEngine(FitData * data, 
                           const gsl_multifit_fdfsolver_type * type) :
  FitEngine(data), jacobianScalingFactor(1)
{
  solver = gsl_multifit_fdfsolver_alloc(type,
                                        fitData->dataPoints(), 
                                        fitData->freeParameters());
}


GSLFitEngine::~GSLFitEngine()
{
  if(solver)
    gsl_multifit_fdfsolver_free(solver);
  solver = NULL;
}

int GSLFitEngine::staticF(const gsl_vector * x, void * params, gsl_vector * f)
{
  GSLFitEngine * engine = reinterpret_cast<GSLFitEngine *>(params);
  return engine->fitData->f(x, f);
}

int GSLFitEngine::staticDf(const gsl_vector * x, void * params, gsl_matrix * df)
{
  GSLFitEngine * engine = reinterpret_cast<GSLFitEngine *>(params);
  // build the jacobian every time ?
  SparseJacobian sj(engine->fitData, false, df);

  // Here, scale the jacobian when necessary !
  int status = engine->fitData->df(x, &sj);
  if(engine->jacobianScalingFactor != 1)
    gsl_matrix_scale(df, engine->jacobianScalingFactor);
  return status;
}

int GSLFitEngine::staticFdf(const gsl_vector * x, void * params, gsl_vector * f,
                       gsl_matrix * df)
{
  GSLFitEngine * engine = reinterpret_cast<GSLFitEngine *>(params);
  SparseJacobian sj(engine->fitData, false, df);

  int status = engine->fitData->fdf(x, f, &sj);

  if(engine->jacobianScalingFactor != 1)
    gsl_matrix_scale(df, engine->jacobianScalingFactor);
  return status;
}


void GSLFitEngine::initialize(const double * initialGuess)
{
  function.f = &GSLFitEngine::staticF;
  function.df = &GSLFitEngine::staticDf;
  function.fdf = &GSLFitEngine::staticFdf;
  function.n = fitData->dataPoints();
  function.p = fitData->freeParameters();
  function.params = this;

  QVarLengthArray<double, 1000> storage(function.p);
  gsl_vector_view v = gsl_vector_view_array(storage.data(), function.p);
  fitData->packParameters(initialGuess, &v.vector);
  gsl_multifit_fdfsolver_set(solver, &function, &v.vector);

  storedParameters.clear();
  iterations = 0;
  jacobianScalingFactor = 1.0;
  pushCurrentParameters();
}

const gsl_vector * GSLFitEngine::currentParameters() const
{
  return solver->x;
}

void GSLFitEngine::computeCovarianceMatrix(gsl_matrix * target) const
{
#if GSL_MAJOR_VERSION >= 2
  // suboptimal: allocate and then free
  gsl_matrix * m = gsl_matrix_alloc(function.n, function.p);
  gsl_multifit_fdfsolver_jac(solver, m);
  gsl_multifit_covar(m, 1e-8, target);
  gsl_matrix_free(m);
#else
  gsl_multifit_covar(solver->J, 1e-8, target);
#endif
}

int GSLFitEngine::iterate() 
{
  int nbTries = 0;
  jacobianScalingFactor = 1.0;
  while(1) {
    try {
      int status = gsl_multifit_fdfsolver_iterate(solver);
      // o << "Iteration: " << status << " -- " << jacobianScalingFactor << endl;
      pushCurrentParameters();


      if(jacobianScalingFactor != 1.0) {
        // We reset the fit status, that may be altered by the
        // jacobian fiddling...
        jacobianScalingFactor = 1;
        QVarLengthArray<double, 1000> storage(function.p);
        gsl_vector_view v = 
          gsl_vector_view_array(storage.data(), function.p);
        gsl_vector_memcpy(&v.vector, solver->x);
        gsl_multifit_fdfsolver_set(solver, &function, &v.vector);
        jacobianScalingFactor = 2; // Just != 1 is good enough.
        return GSL_CONTINUE;
      }

      if(status)
        return status;

      // We stop if we are progressing less than 1e-4 in relative units.
      status = gsl_multifit_test_delta(solver->dx, solver->x,
                                       0, 1e-4);

      if(status != GSL_SUCCESS && storedParameters.size() > 10) {
        double res = storedParameters[storedParameters.size() - 1].residuals;
        double lastres = storedParameters[storedParameters.size() - 2].residuals;
        double relprog = (lastres - res)/res;
        if(relprog >= 0 && relprog < 1e-4)
          return GSL_SUCCESS;   // No need to go on with hundreds of
                                // small steps !
      }

      /// @todo Maybe use the gradient ? The residuals ?
      
      // if(status != GSL_SUCCESS)
      //   return status;
      // QVarLengthArray<double, 1000> storage(function.p);
      // gsl_vector_view v = gsl_vector_view_array(storage.data(), function.p);
      // gsl_vector_memcpy(&v.vector, solver->x);
      return status;
    }
    catch(const RuntimeError & e) { /// @todo Maybe there should be a
                                    /// specific exception for that ?
      // if(nbTries)
      //   jacobianScalingFactor *= jacobianScalingFactor;
      // else
      //   jacobianScalingFactor = 1.25; // Starting slowly, but...
      
      jacobianScalingFactor *= 1.6;
      nbTries++;
      if(nbTries >= 19)          // That's quite enough
        throw;
      Debug::debug()
        << "Scaling problem:" << e.message()
        << "\n -> scaling the jacobian by : " 
        << jacobianScalingFactor << endl;

      // We apparently need to reinitialize the solver...
      QVarLengthArray<double, 1000> storage(function.p);
      gsl_vector_view v = gsl_vector_view_array(storage.data(), function.p);
      gsl_vector_memcpy(&v.vector, solver->x);
      gsl_multifit_fdfsolver_set(solver, &function, &v.vector);
    }
  }
    return -1;                  // Never reached.
}

double GSLFitEngine::residuals() const
{
  return gsl_blas_dnrm2(solver->f);
}

static FitEngine * lmsderFE(FitData * d)
{
  return new GSLFitEngine(d, gsl_multifit_fdfsolver_lmsder);
}

static FitEngineFactoryItem lmsder("lmsder", "GSL scaled Levenberg-Marquardt", &lmsderFE);

static FitEngine * lmderFE(FitData * d)
{
  return new GSLFitEngine(d, gsl_multifit_fdfsolver_lmder);
}

static FitEngineFactoryItem lmder("lmder", "GSL unscaled Levenberg-Marquardt", &lmderFE);

#if GSL_MAJOR_VERSION >= 2

static FitEngine * lmNiel(FitData * d)
{
  return new GSLFitEngine(d, gsl_multifit_fdfsolver_lmniel);
}

static FitEngineFactoryItem lmniel("lmniel", "GSL Levenberg-Marquardt Nielsen", &lmNiel);

#endif
