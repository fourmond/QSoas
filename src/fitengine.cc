/*
  fitengine.cc: implementation of FitEngine and derived classes
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
#include <fitdata.hh>
#include <fitengine.hh>
#include <exceptions.hh>

FitEngine::FitEngine(FitData * d) : fitData(d) 
{
}

void FitEngine::copyCurrentParameters(gsl_vector * target) const
{
  gsl_vector_memcpy(target, currentParameters());
}

FitEngine::~FitEngine()
{
}

void FitEngine::pushCurrentParameters()
{
  Vector v;
  v.resize(fitData->freeParameters());
  gsl_vector_view vi  = gsl_vector_view_array(v.data(), v.size());
  copyCurrentParameters(&vi.vector);
  storedParameters << StoredParameters(v, residuals());
}

//////////////////////////////////////////////////////////////////////

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

  // Here, scale the jacobian when necessary !
  int status = engine->fitData->df(x, df);
  if(engine->jacobianScalingFactor != 1)
    gsl_matrix_scale(df, engine->jacobianScalingFactor);
  return status;
}

int GSLFitEngine::staticFdf(const gsl_vector * x, void * params, gsl_vector * f,
                       gsl_matrix * df)
{
  GSLFitEngine * engine = reinterpret_cast<GSLFitEngine *>(params);
  int status = engine->fitData->fdf(x, f, df);
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
}

const gsl_vector * GSLFitEngine::currentParameters() const
{
  return solver->x;
}

void GSLFitEngine::computeCovarianceMatrix(gsl_matrix * target) const
{
  gsl_multifit_covar(solver->J, 0, target);
}

int GSLFitEngine::iterate() 
{
  int nbTries = 0;
  jacobianScalingFactor = 1.0;
  pushCurrentParameters();
  while(1) {
    try {
      int status = gsl_multifit_fdfsolver_iterate(solver);
      if(status)
        return status;
      
      pushCurrentParameters();

      // This is garbage... It doesn't stop where it should.
      if(jacobianScalingFactor != 1.0)
        return GSL_CONTINUE;

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
      nbTries++;
      jacobianScalingFactor *= 2;
      if(nbTries >= 10)
        throw;
      QTextStream o(stdout);
      o << "Scaling problem, scaling the jacobian by : " 
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
