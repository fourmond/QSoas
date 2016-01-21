/**
   \file gslintegrator.cc
   The basic adaptive GSL integrator
   Copyright 2013 by CNRS/AMU

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

#include <integrator.hh>
#include <gsl/gsl_integration.h>
#include <exceptions.hh>

class GSLIntegrator : public Integrator {
protected:
  /// Maximum of intervals
  int max;

  /// The integration workspace
  gsl_integration_workspace * workspace;

  /// The integration function
  static double f(double x, void * params);

  /// The function pointing towards f
  gsl_function intf;

  /// The algorithm 'key'
  int key;
 

  virtual int subdivisions() const {
    return max;
  };

public:

  GSLIntegrator(int intervals = 30, 
                double rel = 1e-4, double abs = 0, int key = 3);

  ~GSLIntegrator();

  /// Integrate over a segment
  virtual double integrateSegment(const std::function<double (double)> & f, 
                                  double a, double b, double * error = NULL);
                          

  /// Returns the number of intervals used for the last computation
  int intervals() const {
    return workspace->size;
  };
};

GSLIntegrator::GSLIntegrator(int intervals, double rel, double abs, int k) :
  Integrator(rel, abs), key(k)
{
  workspace = gsl_integration_workspace_alloc(intervals);
  max = intervals;
  intf.params = this;
  intf.function = &GSLIntegrator::f;
}

/// @todo Should join Integrator ?
double GSLIntegrator::f(double x, void * params)
{
  GSLIntegrator * i = reinterpret_cast<GSLIntegrator*>(params);
  i->funcalls++;
  return i->fnc(x);
}

GSLIntegrator::~GSLIntegrator()
{
  gsl_integration_workspace_free(workspace);
}

double GSLIntegrator::integrateSegment(const std::function<double (double)> & f, 
                                       double a, double b, double * error)
{
  double res = 0;
  double err = 0;
  funcalls = 0;
  fnc = f;
  
  int status = gsl_integration_qag(&intf, a, b, absolutePrec, relativePrec,
                                   max, key, workspace, &res, &err);
  if(error)
    *error = err;
  return res;
}

IntegratorFactory gauss15("gauss15", "15-points Gauss-Kronrod",
                          [](int segs, double rel, double abs) -> Integrator * {
                            return new GSLIntegrator(segs, rel, abs, 1);
                          });
IntegratorFactory gauss21("gauss21", "21-points Gauss-Kronrod",
                          [](int segs, double rel, double abs) -> Integrator * {
                            return new GSLIntegrator(segs, rel, abs, 2);
                          });
IntegratorFactory gauss31("gauss31", "31-points Gauss-Kronrod",
                          [](int segs, double rel, double abs) -> Integrator * {
                            return new GSLIntegrator(segs, rel, abs, 3);
                          });
IntegratorFactory gauss41("gauss41", "41-points Gauss-Kronrod",
                          [](int segs, double rel, double abs) -> Integrator * {
                            return new GSLIntegrator(segs, rel, abs, 4);
                          });
IntegratorFactory gauss51("gauss51", "51-points Gauss-Kronrod",
                          [](int segs, double rel, double abs) -> Integrator * {
                            return new GSLIntegrator(segs, rel, abs, 5);
                          });
IntegratorFactory gauss61("gauss61", "61-points Gauss-Kronrod",
                          [](int segs, double rel, double abs) -> Integrator * {
                            return new GSLIntegrator(segs, rel, abs, 6);
                          });

//////////////////////////////////////////////////////////////////////

class GSLQNGIntegrator : public Integrator {
protected:
  /// Maximum of intervals
  int max;

  /// The integration function
  static double f(double x, void * params);

  /// The function pointing towards f
  gsl_function intf;

  /// The algorithm 'key'
  int key;
 
  
public:

  GSLQNGIntegrator(double rel = 1e-4, double abs = 0);

  ~GSLQNGIntegrator();

  /// Integrate over a segment
  virtual double integrateSegment(const std::function<double (double)> & f, 
                                  double a, double b, double * error = NULL);
                          

  /// Returns the number of intervals used for the last computation
  int intervals() const {
    return 1;
  };
};

GSLQNGIntegrator::GSLQNGIntegrator(double rel, double abs) :
  Integrator(rel, abs)
{
  intf.params = this;
  intf.function = &GSLQNGIntegrator::f;
}

GSLQNGIntegrator::~GSLQNGIntegrator()
{
}

double GSLQNGIntegrator::f(double x, void * params)
{
  GSLQNGIntegrator * i = reinterpret_cast<GSLQNGIntegrator*>(params);
  i->funcalls++;
  return i->fnc(x);
}

double GSLQNGIntegrator::integrateSegment(const std::function<double (double)> & f, 
                                          double a, double b, double * error)
{
  double res = 0;
  double err = 0;
  funcalls = 0;
  fnc = f;
  
  size_t nb = 0;
  int status = gsl_integration_qng(&intf, a, b, absolutePrec, relativePrec,
                                   &res, &err, &nb);
  if(error)
    *error = err;
  return res;
}

IntegratorFactory qng("qng", "Non-adaptive Gauss-Kronrod",
                      [](int /*segs*/, double rel, double abs) -> Integrator * {
                        return new GSLQNGIntegrator(rel, abs);
                      });
