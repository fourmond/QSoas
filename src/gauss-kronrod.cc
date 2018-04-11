/*
  gauss-kronrod.cc: implementation of the Gauss-Kronrod adaptive integrators
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
#include <multiintegrator.hh>

#include <vector.hh>
#include <dataset.hh>

/// Gauss Kronrod integrators. This code is greatly inspired from the
/// GSL code.
class GaussKronrodMultiIntegrator : public MultiIntegrator {
protected:

  /// The size of the abscissae. Total number of points is 2*size - 1.
  /// Number of Gauss points: size - 1
  int size;
  
  /// The abscissae of the points. Size is @a size
  /// The odd elements are also the gauss positions.
  const double * abscissae;

  /// The kronrod weights. Size @a size
  const double * kWeights;

  /// The gauss weights. Size @a (size+1)/2
  const double * gWeights;

  /// Performs a Gauss-Kronrod quadrature of the given interval, and
  /// stores the results and the error estimate in @a target and @a
  /// errors, resp.
  void gKQuadrature(double a, double b, gsl_vector * target,
                    gsl_vector * errors) {
    // the values (in the order of increasing x values)
    gsl_vector * values[2*size - 1];
    double dx = 0.5 * (b-a);
    double xa =  0.5 + (a+b);

    for(int i = 0; i < size; i++) {
      double x1 = xa + dx * abscissae[i];
      values[i] = functionForValue(x1);
      double x2 = xa - dx * abscissae[i];
      values[2*size - 2 - i] = functionForValue(x2); // sometimes the
                                                     // same as x1 (but not recomputed)
    }

    for(size_t j = 0; j < target->size; j++) {
      double gauss = 0;
      double kronrod = 0;
      for(int i = 0; i < 2*size-1; i++) {
        int idx = i < size ? i : 2*size - 1 - i;
        kronrod += kWeights[idx] * gsl_vector_get(values[i], j);
        if(i % 2 == 1)
          gauss += gWeights[idx/2] * gsl_vector_get(values[i], j);
      }
      /// @todo This is greatly simplified with respect to the GSL
      /// code. See more about that in the GSL source
      gsl_vector_set(errors, j, fabs((kronrod - gauss) * dx));
      gsl_vector_set(target, j, kronrod * dx);
    }
    
  }
  

public:
};

