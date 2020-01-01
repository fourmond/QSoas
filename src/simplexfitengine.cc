/**
   \file simplexfitengine.cc
   A Nelder-Mead 'fit' engine for QSoas
   Copyright 2013, 2018 by CNRS/AMU

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

#include <simplex.hh>
#include <gsl-types.hh>

#include <debug.hh>

/// A Nelder-Mead fit engine.
///
/// All the parameters here are the "GSL" parameters.
class SimplexFitEngine : public FitEngine {
protected:

  /// The simplex
  Simplex simplex;

  /// A vector for the function evaluation
  gsl_vector * func;

  double evaluate(const Vector & pos) const {
    fitData->f(pos, func, true);
    double res;
    gsl_blas_ddot(func, func, &res);
    return res;
  };

public:


  SimplexFitEngine(FitData * data)  :
    FitEngine(data), simplex([this](const Vector & v) -> double {
        return evaluate(v);
      })
  {
    func = gsl_vector_alloc(fitData->dataPoints());
  };

  
  virtual ~SimplexFitEngine() {
    gsl_vector_free(func);
  };


  virtual void computeCovarianceMatrix(gsl_matrix * target) const {
    gsl_matrix_set_zero(target);
  };

  virtual void initialize(const double * initialGuess) override {
    int nb = fitData->freeParameters();
    Vector params(nb, 0.0);
    fitData->packParameters(initialGuess, params);

    simplex.vertices.clear();
    simplex.vertices << simplex.compute(params);

    for(int i = 0; i < nb; i++)
      simplex.vertices << simplex.initialVertex(params, i);

    simplex.sort();
  };
  
  virtual const gsl_vector * currentParameters() const override {
    return simplex.vertices[0].toGSLVector();
  };
  
  virtual int iterate() override {
    simplex.debug = fitData->debug;
    return simplex.iterate();
  };
  
  virtual double residuals() const override {
    return simplex.vertices[0].residuals;
  };

  virtual CommandOptions getEngineParameters() const override {
    return simplex.getParameters();
  };

  /// Static options
  static ArgumentList options;

  
  virtual ArgumentList * engineOptions() const override {
    return &options;
  };
  
  virtual void resetEngineParameters() override {
    /// hmmm.
  };
  
  virtual void setEngineParameters(const CommandOptions & opts) override {
    simplex.setParameters(opts);
  };
};

ArgumentList SimplexFitEngine::options(Simplex::simplexOptions());

static FitEngine * simplexFE(FitData * d)
{
  return new SimplexFitEngine(d);
}

static FitEngineFactoryItem simplex("simplex", "Simplex",
                                    &simplexFE, &SimplexFitEngine::options, true);
