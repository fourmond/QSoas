/*
  functions.cc: mathematical functions
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
#include <functions.hh>

#include <gsl/gsl_randist.h>
#include <gsl/gsl_sf.h>

#include <credits.hh>

#include <gsl/gsl_integration.h>
#include <odesolver.hh>

// atan:
//
//             3        5        7        9      10
//    x - 1/3 x  + 1/5 x  - 1/7 x  + 1/9 x  + O(x  )
double Functions::atanc(double x)
{
  if(x < 0.01 && x > -0.01)
    return 1 - x*x/3 + gsl_pow_4(x)/5 - gsl_pow_6(x)/7 + gsl_pow_8(x)/9;
  else
    return atan(x)/x;
}

//
// atanh:    
//             3        5        7        9      10
//    x + 1/3 x  + 1/5 x  + 1/7 x  + 1/9 x  + O(x  )
double Functions::atanhc(double x)
{
  if(x < 0.01 && x > -0.01)
    return 1 + x*x/3 + gsl_pow_4(x)/5 + gsl_pow_6(x)/7 + gsl_pow_8(x)/9;
  else
    return atanh(x)/x;
}

double Functions::pseudoVoigt(double x, double w, double mu)
{
  return (1 - mu) * gsl_ran_gaussian_pdf(x, w) + mu *
    gsl_ran_cauchy_pdf(x, w);
}

double Functions::marcusHushChidseyZeng(double lambda, double eta)
{
  double a = 1 + sqrt(lambda);
  return sqrt(M_PI * lambda)/(1 + exp(- eta)) * gsl_sf_erfc((lambda - sqrt(a + eta*eta))/(2 * sqrt(lambda)));
}

static Credits mhz("Zeng et al, JEAC, 2014", "the k_mhc_z function", "10.1016/j.jelechem.2014.09.038");

static double mhc_integrand(double x, void * params)
{
  const double * p = reinterpret_cast<const double*>(params);
  const double & lambda = p[0];
  const double & eta = p[1];
  double i1 = -(x - lambda + eta)*(x - lambda + eta)/(4*lambda);
  return exp(i1) /(1 + exp(x));
}

double Functions::marcusHushChidsey(double lambda, double eta)
{
  static gsl_integration_workspace * ws = NULL;
  if(! ws)
    ws = gsl_integration_workspace_alloc(400);
  gsl_function f;
  double vals[2];
  vals[0] = lambda;
  vals[1] = eta;
  f.function = &::mhc_integrand;
  f.params = vals;
  double rv;
  double er;

  gsl_integration_qagi(&f, 1e-5, 1e-5, 400, ws, &rv, &er);
  return rv;
}


double Functions::trumpetBV(double rate, double alpha, double prec)
{
  double time = -0.5;
  double cur = 0;
  double pos = time;

  double max = 20;

  std::function<double (double, double) > dy = 
    [rate, alpha](double t, double red) -> double {
    double kf = exp(alpha * t)/rate;
    double kb = exp((alpha-1) * t)/rate;
    return (1 - red)*kb - red * kf;
  };

  LambdaODESolver slv(1,
                      [rate, alpha, &dy](double t, const double *y,
                                      double * dydt) -> void {
                        dydt[0] = dy(t, y[0]);
                      });
  ODEStepperOptions opts = slv.getStepperOptions();
  
  /// I think the best solver
  opts.type = gsl_odeiv2_step_bsimp;

  slv.setStepperOptions(opts);

  double val[1] = {1};
  slv.initialize(val, -3);

  slv.stepTo(time);
  while(time < max) {
    time += prec;
    slv.stepTo(time);
    double c = dy(time, slv.currentValues()[0]);
    if(c > cur)
      return pos;
    cur = c;
    pos = time;
  }

  return -1;                    // Hmmm
}
