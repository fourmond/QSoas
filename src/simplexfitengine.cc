/**
   \file simplexfitengine.cc
   A Nelder-Mead 'fit' engine for QSoas
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


/// A Nelder-Mead fit engine.
///
/// All the parameters here are the "GSL" parameters.
class SimplexFitEngine : public FitEngine {
protected:

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
  double shrink;

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

  
  
public:

  SimplexFitEngine(FitData * data);
  virtual ~SimplexFitEngine();


  virtual void initialize(const double * initialGuess);
  virtual const gsl_vector * currentParameters() const;
  virtual void computeCovarianceMatrix(gsl_matrix * target) const;
  virtual int iterate();
  virtual double residuals() const;
};


StoredParameters SimplexFitEngine::findNeighbourhood(const gsl_vector * orig,
                                                     const gsl_vector * target,
                                                     double threshold)
{
  double factor = 1;
  
  // We need two vectors:

  /// @question Should they be allocated on the heap ? No reason for
  /// that?
  QVarLengthArray<double, 100> deltaVals;
  gsl_vector_view delta = gsl_vector_view_array(deltaVals.data(), 
                                                orig->size());
  
  
  QVarLengthArray<double, 100> curVals;
  gsl_vector_view cur = gsl_vector_view_array(cur.data(), 
                                              orig->size());
  gsl_vector_memcpy(&delta.vector, target);
  gsl_vector_sub(&delta.vector, orig);
  
  while(factor >= threshold) {
    // Make a trial step
    gsl_vector_memcpy(&cur.vector, orig);
    gsl_blas_daxpy(factor, &delta.vector, &cur.vector);

    try {
      
    }
    
  }

  // Never reached ?
  return 
}

// static FitEngine * simplexFE(FitData * d)
// {
//   return new SimplexFitEngine(d);
// }

// static FitEngineFactoryItem simplex("simplex", "Simplex", &simplexFE);
