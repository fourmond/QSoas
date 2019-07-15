/**
   \file gsl2fitengine.cc
   The GSL-based fit engine, for the newer GSL versions
   Copyright 2019 by CNRS/AMU

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

#include <gsl/gsl_multifit_nlinear.h>

class GSL2FitEngine : public FitEngine {
  
  /// The solver in use
  gsl_multifit_nlinear_workspace * workspace;

  /// The parameters of the fit
  gsl_multifit_nlinear_parameters fit_params;

  /// The function in use
  gsl_multifit_nlinear_fdf function;

  /// The current scaling factor of the jacobian...
  /// Argh, we need that again :-(...
  double jacobianScalingFactor;

protected:

  static int staticF(const gsl_vector * x, void * params, gsl_vector * f) {
    GSL2FitEngine * engine = reinterpret_cast<GSL2FitEngine *>(params);
      return engine->fitData->f(x, f);
  };
  
  static int staticDf(const gsl_vector * x, void * params, gsl_matrix * df) {
    GSL2FitEngine * engine = reinterpret_cast<GSL2FitEngine *>(params);
    // build the jacobian every time ?
    SparseJacobian sj(engine->fitData, false, df);
    int status = engine->fitData->df(x, &sj);
    if(engine->jacobianScalingFactor != 1)
      gsl_matrix_scale(df, engine->jacobianScalingFactor);

    return status;
  };

public:

  GSL2FitEngine(FitData * data, const gsl_multifit_nlinear_type * type) :
    FitEngine(data), jacobianScalingFactor(1.0)
  {
    fit_params = gsl_multifit_nlinear_default_parameters();
    workspace = gsl_multifit_nlinear_alloc(type,
                                           &fit_params,
                                           fitData->dataPoints(), 
                                           fitData->freeParameters());
  };
  
  virtual ~GSL2FitEngine() {
    if(workspace)
      gsl_multifit_nlinear_free(workspace);
    workspace = NULL;
  };


  virtual void initialize(const double * initialGuess) {
    function.f = &GSL2FitEngine::staticF;
    function.df = &GSL2FitEngine::staticDf;
    function.fvv = NULL;
    function.n = fitData->dataPoints();
    function.p = fitData->freeParameters();
    function.params = this;

    QVarLengthArray<double, 1000> storage(function.p);
    gsl_vector_view v = gsl_vector_view_array(storage.data(), function.p);
    fitData->packParameters(initialGuess, &v.vector);
    gsl_multifit_nlinear_init(&v.vector, &function, workspace);

    iterations = 0;
  };
  
  virtual const gsl_vector * currentParameters() const {
    return gsl_multifit_nlinear_position(workspace);
  };
  
  virtual void computeCovarianceMatrix(gsl_matrix * target) const {
    /// @todo !!!
  };
  
  virtual int iterate() {
    const double xtol = 1.0e-8;
    const double gtol = 1.0e-8;
    const double ftol = 1.0e-8;
    int nbTries = 0;

    jacobianScalingFactor = 1.0;
    while(1) {
      try {

    
        int info;
        int status = gsl_multifit_nlinear_iterate(workspace);
        if(! status == GSL_SUCCESS) {
          fprintf(stdout, "gsl2 error: %s\n", gsl_strerror(status));
          return status;
        }

        
        if(jacobianScalingFactor != 1.0) {
          // We reset the fit status, that may be altered by the
          // jacobian fiddling...
          jacobianScalingFactor = 1;
          QVarLengthArray<double, 1000> storage(function.p);
          gsl_vector_view v = 
            gsl_vector_view_array(storage.data(), function.p);
          gsl_vector_memcpy(&v.vector, currentParameters());
          gsl_multifit_nlinear_init(&v.vector, &function, workspace);
          jacobianScalingFactor = 2; // Just != 1 is good enough.
          return GSL_CONTINUE;
        }
        
        
        status = gsl_multifit_nlinear_test(xtol, gtol, ftol, &info,
                                           workspace);
        return status;
      }
      catch(const RuntimeError & e) { 
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
        gsl_vector_memcpy(&v.vector, currentParameters());
        gsl_multifit_nlinear_init(&v.vector, &function, workspace);
      }
    }

  };
  
  virtual double residuals() const {
    const gsl_vector * f = gsl_multifit_nlinear_residual(workspace);
    return gsl_blas_dnrm2(f);
  };
};

static FitEngine * gsl2FE(FitData * d)
{
  return new GSL2FitEngine(d, gsl_multifit_nlinear_trust);
}

static FitEngineFactoryItem gsl2("gsl2", "GSL2 non-linear with trust region", &gsl2FE);

