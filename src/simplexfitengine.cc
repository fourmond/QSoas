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

/// A Nelder-Mead fit engine.
///
/// All the parameters here are the "GSL" parameters.
class SimplexFitEngine : public FitEngine {
protected:

  /// The n + 1 vertices
  QList<StoredParameters> vertices;

  /// The factor for computing a vertex from the initial parameters.
  double initialVertexFactor;

  /// Reflection factor
  double alpha;

  /// Expansion factor
  double beta;

  /// Contraction factor
  double gamma;

  /// Shrink factor
  double delta;

  /// Final threshold
  double threshold;

  /// Whether we use scaling or not
  bool useScaling;

  /// A vector for the function evaluation
  gsl_vector * func;

  /// Attempts to find a position along the given direction where the
  /// parameters are valid, and compute the residuals at that point.
  ///
  /// Rethrows original exceptions if the distance is below threshold
  /// times the original one.
  ///
  /// This function assumes that the origin point is VALID !
  StoredParameters findNeighbourhood(const gsl_vector * orig,
                                     const gsl_vector * target,
                                     double threshold = 1e-3);

  /// Attemps to find a point in the given direction that is
  /// valid. The point will be at most \a scale times \a direction
  /// away from \a origin, and at least \a threshold times \a
  /// direction.
  StoredParameters findTowards(const gsl_vector * orig,
                               const gsl_vector * direction,
                               double threshold = 1e-3,
                               double scale = 1);


  /// Computes the fits/residuals at the given location.
  StoredParameters computeVertex(const gsl_vector * pos);

  /// scaling with respect to the initial parameters.
  ///
  /// If the simplex use scaling, then this has a non-zero size.
  /// By default always use scaling.
  Vector scalingFactors;

public:

  /// Static options
  static ArgumentList options;


  SimplexFitEngine(FitData * data);
  virtual ~SimplexFitEngine();


  virtual void initialize(const double * initialGuess) override;
  virtual const gsl_vector * currentParameters() const override;
  virtual void computeCovarianceMatrix(gsl_matrix * target) const override;
  virtual int iterate() override;
  virtual double residuals() const override;

  virtual CommandOptions getEngineParameters() const override;
  virtual ArgumentList * engineOptions() const override;
  virtual void resetEngineParameters() override;
  virtual void setEngineParameters(const CommandOptions & params) override; 

};

SimplexFitEngine::SimplexFitEngine(FitData * data) :
  FitEngine(data)
{
  func = gsl_vector_alloc(fitData->dataPoints());
  resetEngineParameters();
}

SimplexFitEngine::~SimplexFitEngine()
{
  gsl_vector_free(func);
}

void SimplexFitEngine::resetEngineParameters()
{
  alpha = 1;
  beta = 2;
  gamma = 0.5;
  delta = 0.5;
  threshold = 1e-4;
  useScaling = true;
}

ArgumentList SimplexFitEngine::
options(QList<Argument*>()
        << new NumberArgument("alpha", "Reflection factor")
        << new NumberArgument("beta", "Expansion factor")
        << new NumberArgument("gamma", "Contraction factor")
        << new NumberArgument("delta", "Shrink factor")
        << new NumberArgument("end-threshold", "Threshold for ending")
        << new BoolArgument("use-scaling", "Scale parameters")
        );

ArgumentList * SimplexFitEngine::engineOptions() const
{
  return &options;
}


CommandOptions SimplexFitEngine::getEngineParameters() const
{
  CommandOptions val;
  updateOptions(val, "alpha", alpha);
  updateOptions(val, "beta", beta);
  updateOptions(val, "gamma", gamma);
  updateOptions(val, "delta", delta);
  updateOptions(val, "use-scaling", useScaling);
  updateOptions(val, "end-threshold", threshold);

  return val;
}


void SimplexFitEngine::setEngineParameters(const CommandOptions & val)
{
  updateFromOptions(val, "alpha", alpha);
  updateFromOptions(val, "beta", beta);
  updateFromOptions(val, "gamma", gamma);
  updateFromOptions(val, "delta", delta);
  updateFromOptions(val, "end-threshold", threshold);
  updateFromOptions(val, "use-scaling", useScaling);
}


void SimplexFitEngine::initialize(const double * initialGuess)
{
  int nb = fitData->freeParameters();
  QVarLengthArray<double, 100> paramVals(nb);
  gsl_vector_view params = gsl_vector_view_array(paramVals.data(), nb); 

  QVarLengthArray<double, 100> paramVals2(nb);
  gsl_vector_view params2 = gsl_vector_view_array(paramVals.data(), nb);

  fitData->packParameters(initialGuess, &params.vector);
  if(useScaling) {
    int n = fitData->freeParameters();
    scalingFactors.resize(n);
    for(int i = 0; i < n; i++) {
      double v = paramVals[i];
      if(v == 0)
        v = 1;
      scalingFactors[i] = v;
      paramVals[i] /= v;
    }
  }
    

  // OK, so now, we position the initial vertices
  vertices << computeVertex(&params.vector);
  
  for(int i = 0; i < nb; i++) {
    // We attempt a factor-of-two increase in the parameter
    gsl_vector_memcpy(&params2.vector, &params.vector);
    gsl_vector_set(&params2.vector, i, 
                   2 * gsl_vector_get(&params2.vector, i));
    vertices << findNeighbourhood(&params.vector, &params2.vector);
  }

  qSort(vertices);
  // Now, we have all the vertices !
}

const gsl_vector * SimplexFitEngine::currentParameters() const
{
  // This function assumes that the vertices are sorted
  return vertices[0].toGSLVector();
}

double SimplexFitEngine::residuals() const
{
  return sqrt(vertices[0].residuals);
}

void SimplexFitEngine::computeCovarianceMatrix(gsl_matrix * target) const
{
  // This clearly isn't what we want !
  gsl_matrix_set_zero(target);
}

int SimplexFitEngine::iterate()
{
  // OK!

  // I'll follow notation on page 2 of Gao & Han 2010

  // We assume that the vertices are sorted at the beginning of an
  // iteration.

  int nb = fitData->freeParameters();

  Vector initialPosition = vertices[0].parameters;

  /// @todo Maybe I should make a small class to have gsl_vectors
  /// allocated on the stack ?

  Vector center(nb, 0);
  Vector candidate(nb, 0);

  // QVarLengthArray<double, 100> p3(nb);
  // gsl_vector_view delta = gsl_vector_view_array(p3.data(), nb); 


  /// @todo all the the computeVertex functions should be replaced by
  /// appropriate versions with the findNeighbourhood() function.

  gsl_vector_set_zero(center);

  double total = 0;
  // First, compute the barycenter of the n best vertices.
  for(int i = 0; i < nb; i++) {
    /// @todo Use arbitrary weights
    double f = 1;               // To change !
    gsl_blas_daxpy(f, vertices[i].toGSLVector(), center);
    total += f;
  }
  gsl_vector_scale(center, 1/total);

  // Reflection:
  gsl_vector_memcpy(candidate, center);
  gsl_vector_sub(candidate, vertices[nb].toGSLVector());
  gsl_vector_scale(candidate, alpha);
  gsl_vector_add(candidate, center);


  // Idea: instead of antagonizing x_1 and x_n+1, it may be more
  // clever to use a weighted barycenter to get both center and what
  // currently is the x_n+1 point.
  //
  // Weight would be a non-linear function of the residuals linearly
  // mapped to a [0,1] segment. (0 = x_1, 1 = x_n+1). (and the weight
  // for the x_n+1 point would be the 1-complement of the weight for
  // the barycenter)

  // We assume implicitly that the barycenter is within the definition
  // domain of the stuff.
  StoredParameters np = findNeighbourhood(center, candidate);

  if(np < vertices[0]) {
    // Better than the best, do expansion

    gsl_vector_sub(candidate, center);
    gsl_vector_scale(candidate, beta);
    gsl_vector_add(candidate, center);
    StoredParameters np2 = findNeighbourhood(center, candidate);

    if(np2 < np)
      vertices[nb] = np2;
    else
      vertices[nb] = np;
    
  }
  else if(np < vertices[nb-1]) {
    vertices[nb] = np;
  }
  else {
    double scale = gamma;       // outside contraction
    if(vertices[nb] < np)
      scale = -gamma;           // inside contraction

    gsl_vector_sub(candidate, center);
    gsl_vector_scale(candidate, scale);
    gsl_vector_add(candidate, center);
    StoredParameters np2 = findNeighbourhood(center, candidate);
    if(np2 < np)
      vertices[nb] = np2;
    else {
      // We shrink !

      for(int i = 1; i <= nb; i++) {

        gsl_vector_memcpy(candidate, vertices[i].toGSLVector());
        gsl_vector_sub(candidate, vertices[0].toGSLVector());
        gsl_vector_scale(candidate, delta);
        gsl_vector_add(candidate, vertices[0].toGSLVector());
        
        vertices[i] = findNeighbourhood(center, candidate);
      }
    }
  }

  // Now, we sort the vertices
  qSort(vertices);


  if(vertices[0].parameters == initialPosition)
    return GSL_CONTINUE;        // We didn't win

  for(int i = 0; i < nb; i++)
    if(fabs((vertices[0].parameters[i] - initialPosition[i])/
            initialPosition[i]) > threshold)
      return GSL_CONTINUE;

  return GSL_SUCCESS;
}


StoredParameters SimplexFitEngine::computeVertex(const gsl_vector * pos)
{
  int nb = fitData->freeParameters();
  QVarLengthArray<double, 100> paramVals(nb);
  const gsl_vector * v;
  gsl_vector_view params = gsl_vector_view_array(paramVals.data(), nb); 
  if(useScaling) {
    // Back to real values
    for(int i = 0; i < nb; i++) {
      paramVals[i] = gsl_vector_get(pos,i) * scalingFactors[i];
    }
    v = &params.vector;
  }
  else
    v = pos;

  fitData->f(v, func, true);
  double res;
  gsl_blas_ddot(func, func, &res);
  return StoredParameters(pos, res);
}


StoredParameters SimplexFitEngine::findNeighbourhood(const gsl_vector * orig,
                                                     const gsl_vector * target,
                                                     double threshold)
{
  Vector delta(orig->size, 0);
  gsl_vector_memcpy(delta, target);
  gsl_vector_sub(delta, orig);

  return findTowards(orig, delta, threshold, 1);
}

StoredParameters SimplexFitEngine::findTowards(const gsl_vector * orig,
                                               const gsl_vector * direction,
                                               double threshold,
                                               double factor)
{
  Vector cur(orig->size, 0);
  
  while(factor >= threshold) {
    // Make a trial step
    gsl_vector_memcpy(cur, orig);
    gsl_blas_daxpy(factor, direction, cur);

    try {
      return computeVertex(cur);
    }
    catch(RuntimeError & e) {
      // Try smaller
      factor *= 0.5;
    }
  }

  throw RuntimeError("Could not find suitable step");

  // Never reached ?
  return StoredParameters(); 
}

static FitEngine * simplexFE(FitData * d)
{
  return new SimplexFitEngine(d);
}

static FitEngineFactoryItem simplex("simplex", "Simplex",
                                    &simplexFE, &SimplexFitEngine::options);
