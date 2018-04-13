/*
  gauss-kronrod.cc: implementation of the Gauss-Kronrod adaptive integrators
  Copyright 2018 by CNRS/AMU

  Some parts of this code, namely the xgk, wg and wgk come from the GSL 
  and are
  Copyright (C) 1996, 1997, 1998, 1999, 2000, 2007 Brian Gough


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

#include <possessive-containers.hh>

/// Gauss Kronrod integrators. This code is greatly inspired from the
/// GSL code. It is much simplified, though.
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
    double xa =  0.5 * (a+b);

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
        int idx = i < size ? i : 2*size - 2 - i;
        kronrod += kWeights[idx] * gsl_vector_get(values[i], j);
        if(i % 2 == 1)
          gauss += gWeights[idx/2] * gsl_vector_get(values[i], j);
      }
      /// @todo This is greatly simplified with respect to the GSL
      /// code. See more about that in the GSL source
      gsl_vector_set(errors, j, fabs((kronrod - gauss) * dx));
      gsl_vector_set(target, j, kronrod * dx);
    }
    
  };
protected:
  /// Represents an integration segment
  class Segment {
  public:
    gsl_vector * sum;
    double left;
    double right;
    gsl_vector * error;

    Segment(double l, double r, size_t s) {
      left = l;
      right = r;
      sum = gsl_vector_alloc(s);
      error = gsl_vector_alloc(s);
    };

    ~Segment() {
      if(sum)
        gsl_vector_free(sum);
      if(error)
        gsl_vector_free(error);
    };

    double relativeError() const {
      double min, max;
      gsl_vector_minmax(sum, &min, &max);
      /// Below this threshold, we don't look at relative error
      /// anymore.
      double thresh = 1e-3 * std::max(fabs(min), fabs(max));

      double e = 0;
      for(size_t i = 0; i < sum->size; i++) {
        double v = fabs(gsl_vector_get(sum, i));
        if(v < thresh)
          v = thresh;
        e += gsl_vector_get(error, i)/v;
      }
      return (right - left) * e/sum->size;
    };
  };

  void doSegment(Segment * seg) {
    gKQuadrature(seg->left,seg->right, seg->sum, seg->error);
  }

public:

  GaussKronrodMultiIntegrator(Function fnc, int dim, double rel, double abs, int maxc, int gsSize, const double * ab, const double * kw, const double * gw) :
    MultiIntegrator(fnc, dim, rel, abs, maxc),
    size(gsSize), abscissae(ab), kWeights(kw), gWeights(gw)
  {
  }

  virtual double integrate(gsl_vector * res, double a, double b) override {
    PossessiveList<Segment> segs;
    segs << new Segment(a, b, res->size);
    doSegment(segs[0]);

    int nbit = 0;
    do {
      double error = 0;
      double em = -1;
      int idx = -1;
      for(int i = 0; i < segs.size(); i++) {
        const Segment & s = *(segs[i]);
        double e = s.relativeError();
        if(e > em) {
          idx = i;
          em = e;
        }
        error += e;
      }

      if(error < (b - a) * relativePrec)
        break;
      

      // bisect:
      Segment * sl = segs[idx];
      double l = sl->left;
      double r = sl->right;
      sl->right = (l+r)*0.5;
      doSegment(sl);
      Segment * s = new Segment((l+r)*0.5, r, res->size);
      segs << s;
      doSegment(s);
      nbit += 1;
    } while(nbit < 100);         // yep

    gsl_vector_set_zero(res);
    for(int i = 0; i < segs.size(); i++)
      gsl_blas_daxpy(1, segs[i]->sum, res);
    
    return 0;
  };

public:
  MultiIntegrator * dup() const {
    return new GaussKronrodMultiIntegrator(function, dimension, relativePrec,
                                           absolutePrec, maxfuncalls, size,
                                           abscissae, kWeights, gWeights);
  };
};


//////////////////////////////////////////////////////////////////////

namespace __gk15 {

  /// 
  static const double xgk[8] =    /* abscissae of the 15-point kronrod rule */
    {
      0.991455371120812639206854697526329,
      0.949107912342758524526189684047851,
      0.864864423359769072789712788640926,
      0.741531185599394439863864773280788,
      0.586087235467691130294144838258730,
      0.405845151377397166906606412076961,
      0.207784955007898467600689403773245,
      0.000000000000000000000000000000000
    };

  /* xgk[1], xgk[3], ... abscissae of the 7-point gauss rule. 
     xgk[0], xgk[2], ... abscissae to optimally extend the 7-point gauss rule */

  static const double wg[4] =     /* weights of the 7-point gauss rule */
    {
      0.129484966168869693270611432679082,
      0.279705391489276667901467771423780,
      0.381830050505118944950369775488975,
      0.417959183673469387755102040816327
    };

  static const double wgk[8] =    /* weights of the 15-point kronrod rule */
    {
      0.022935322010529224963732008058970,
      0.063092092629978553290700663189204,
      0.104790010322250183839876322541518,
      0.140653259715525918745189590510238,
      0.169004726639267902826583426598550,
      0.190350578064785409913256402421014,
      0.204432940075298892414161999234649,
      0.209482141084727828012999174891714
    };

  static MultiIntegrator::MultiIntegratorFactory
  gk("gk15",
     "15 points adaptive Gauss-Kronrod",
     [](MultiIntegrator::Function fnc, int dim, double rel, double abs, int maxc) -> MultiIntegrator * {
       return new GaussKronrodMultiIntegrator(fnc, dim, rel, abs, maxc,
                                              8, xgk, wgk, wg);
     }
     );

}
