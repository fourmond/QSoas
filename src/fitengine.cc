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

//////////////////////////////////////////////////////////////////////

GSLFitEngine::GSLFitEngine(FitData * data, 
                           const gsl_multifit_fdfsolver_type * type) :
  FitEngine(data)
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
  return engine->fitData->df(x, df);
}

int GSLFitEngine::staticFdf(const gsl_vector * x, void * params, gsl_vector * f,
                       gsl_matrix * df)
{
  GSLFitEngine * engine = reinterpret_cast<GSLFitEngine *>(params);
  return engine->fitData->fdf(x, f, df);
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

  iterations = 0;
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
  int status = gsl_multifit_fdfsolver_iterate(solver);
  if(status)
    return status;

  // This is garbage... It doesn't stop where it should.
  return gsl_multifit_test_delta(solver->dx, solver->x,
                                 1e-4, 1e-4);
}

double GSLFitEngine::residuals() const
{
  return gsl_blas_dnrm2(solver->f);
}
