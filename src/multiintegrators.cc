/*
  multiintegrators.cc: various implementations of the 
  Copyright 2015 by CNRS/AMU

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

/// A base class for integration based on interpolation: the error and
/// the subdivisions are estimated based on the difference between
/// interpolated values and computed values.
class InterpolationBasedMultiIntegrator : public MultiIntegrator {
protected:

  /// Prepares the interpolation using the given nodes, for the given
  /// index.
  virtual void prepareInterpolation(Vector nodes, int idx) = 0;

  /// Returns the value interpolated using the context given to the
  /// last call to prepareInterpolation()
  virtual double interpolatedValue(double value) = 0;

  /// Prepares the integration over the given nodes at the given point.
  virtual void prepareIntegration(Vector nodes, int idx) = 0;

  /// Integrate over the whole segment defined by the nodes given to
  /// prepareIntegration()
  virtual double integrate() = 0;
public:
  InterpolationBasedMultiIntegrator(Function fnc, int dim, double rel = 1e-4, double abs = 0, int maxc = 0) :
    MultiIntegrator(fnc, dim, rel, abs, maxc) {
  };

  virtual double integrate(gsl_vector * res, double a, double b) override {
    Vector nodes;
    Vector subNodes;
    Vector normSubNodes;
    Vector errSubNodes;
    QList<gsl_vector *> subValues;

    int nbnodes = 6;


    // Prepare the nodes
    double delta = b-a;

    double err;
    for(int i = 0; i < nbnodes; i++) {
      double x = a + (delta * i)/(nbnodes - 1);
      functionForValue(x);
      nodes << x;
      if(i > 0) {
        x = a + (delta * (i - 0.5))/(nbnodes - 1);
        subNodes << x;
      }
    }

    while(true) {
      int sn = subNodes.size();
      subValues.clear();
      normSubNodes.clear();
      errSubNodes.clear();
      for(int i = 0; i < sn; i++) {
        subValues << functionForValue(subNodes[i]);
        normSubNodes << 0;
        errSubNodes << 0;
      }

      // o << "Nodes: " << nodes.asText().join(", ") << endl;
      // o << "Subnodes: " << subNodes.asText().join(", ") << endl;


      for(int i = 0; i < dimension; i++) {
        prepareInterpolation(nodes, i);
        for(int j = 0; j < sn; j++) {
          double val = gsl_vector_get(subValues[j], i);
          double inter = interpolatedValue(subNodes[j]);
          // o << "Idx: #" << i << " -> " << val << " <=> " << inter << endl;
          normSubNodes[j] += fabs(inter);
          errSubNodes[j] += fabs(inter - val);
        }
      }

      Vector nnodes = nodes;
      Vector ns;
      int nbdiv = 0;
      err = 0;
      for(int i = 0; i < sn; i++) {
        err += errSubNodes[i];
        if(errSubNodes[i]/dimension > absolutePrec &&
           errSubNodes[i]/normSubNodes[i] > relativePrec) {
          // Go ahead and subdivise
          nnodes << subNodes[i];
          int idx = nodes.findCrossing(0, subNodes[i]);
          ns << 0.5 * (nodes[idx-1] + subNodes[i]);
          ns << 0.5 * (nodes[idx] + subNodes[i]);
          nbdiv ++;
        }
        else {
          ns << subNodes[i];
        }
      }
      // o << "Errors: " << errSubNodes.asText().join(", ") << endl;
      // o << "Values: " << normSubNodes.asText().join(", ") << endl;
      // o << "Relative: " << (errSubNodes/normSubNodes).asText().join(", ") << endl;

      err /= errSubNodes.size();

      if(nbdiv == 0 || (maxfuncalls > 0 && funcalls > maxfuncalls))
        break;

      std::sort(nnodes.begin(), nnodes.end());
      std::sort(ns.begin(), ns.end());
      nodes = nnodes;
      subNodes = ns;
    }

    // Now compute the integral proper
    nodes << subNodes;
    std::sort(nodes.begin(), nodes.end());

    for(int i = 0; i < dimension; i++) {
      prepareIntegration(nodes, i);
      gsl_vector_set(res, i, integrate());
    }

    return err/dimension;
  };

};

//////////////////////////////////////////////////////////////////////

class NaiveMultiIntegrator : public InterpolationBasedMultiIntegrator {
protected:
  DataSet values;
  int idx;

  virtual void prepareInterpolation(Vector nodes, int idx) override {
    Vector vals;
    for(int i = 0; i < nodes.size(); i++) {
      gsl_vector * v = functionForValue(nodes[i]);
      vals << gsl_vector_get(v, idx);
    }
    values = DataSet(nodes, vals);
  }

  virtual double interpolatedValue(double x) override {
    return values.yValueAt(x, true);
  }

  virtual void prepareIntegration(Vector nodes, int idx) override {
    prepareInterpolation(nodes, idx);
  }

  virtual double integrate() override {
    return Vector::integrate(values.x(), values.y());
  };
public:
    NaiveMultiIntegrator(Function fnc, int dim, double rel = 1e-4, double abs = 0, int maxc = 0) :
    InterpolationBasedMultiIntegrator(fnc, dim, rel, abs, maxc) {
  };

  MultiIntegrator * dup() const override {
    NaiveMultiIntegrator * i =
      new NaiveMultiIntegrator(function, dimension, relativePrec,
                               absolutePrec, maxfuncalls);
    return i;
  };
};

static MultiIntegrator::MultiIntegratorFactory
naive("naive",
      "Naive trapezoidal integrator",
      [](MultiIntegrator::Function fnc, int dim, double rel, double abs, int maxc) -> MultiIntegrator * {
        return new NaiveMultiIntegrator(fnc, dim, rel, abs, maxc);
      });

//////////////////////////////////////////////////////////////////////


class SplinesMultiIntegrator : public InterpolationBasedMultiIntegrator {
protected:
  gsl_interp * ws;
  const gsl_interp_type * type;
  gsl_interp_accel * accel;
  Vector n,v;
  int idx;

  virtual void prepareInterpolation(Vector nodes, int idx) override {
    n = nodes;
    v.clear();
    if(ws && ws->size != nodes.size()) {
      gsl_interp_free(ws);
      ws = NULL;
      gsl_interp_accel_reset(accel);
    }
    if(! ws) {
      ws = gsl_interp_alloc(type, n.size());
    }

    for(int i = 0; i < nodes.size(); i++) {
      gsl_vector * vx = functionForValue(nodes[i]);
      v << gsl_vector_get(vx, idx);
    }
    gsl_interp_init(ws, n.data(), v.data(), n.size());
  }

  virtual double interpolatedValue(double x) override {
    return gsl_interp_eval(ws, n.data(), v.data(), x, accel);
  }

  virtual void prepareIntegration(Vector nodes, int idx) override {
    prepareInterpolation(nodes, idx);
  }

  virtual double integrate() override {
    return gsl_interp_eval_integ(ws, n.data(), v.data(), n.first(), n.last(), accel);
  };
  
public:
  SplinesMultiIntegrator(Function fnc, int dim, double rel = 1e-4, double abs = 0, int maxc = 0, const gsl_interp_type * t = gsl_interp_cspline) :
    InterpolationBasedMultiIntegrator(fnc, dim, rel, abs, maxc), ws(NULL), type(t) {
    accel = gsl_interp_accel_alloc();
  };

  MultiIntegrator * dup() const override {
    SplinesMultiIntegrator * i =
      new SplinesMultiIntegrator(function, dimension, relativePrec,
                                 absolutePrec, maxfuncalls, type);
    return i;
  };

  ~SplinesMultiIntegrator() {
    gsl_interp_accel_free(accel);
  }
    
};

static MultiIntegrator::MultiIntegratorFactory
csplines("csplines",
         "Integration based on csplines interpolation",
         [](MultiIntegrator::Function fnc, int dim, double rel, double abs, int maxc) -> MultiIntegrator * {
           return new SplinesMultiIntegrator(fnc, dim, rel, abs, maxc);
         });

static MultiIntegrator::MultiIntegratorFactory
akima("akima",
      "Integration based on Akima interpolation",
      [](MultiIntegrator::Function fnc, int dim, double rel, double abs, int maxc) -> MultiIntegrator * {
        return new SplinesMultiIntegrator(fnc, dim, rel, abs, maxc, gsl_interp_akima);
      });
