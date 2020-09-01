/*
  functions.cc: mathematical functions
  Copyright 2015, 2018 by CNRS/AMU

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


/// Validation: functions/laviron.cmds, matches de predictions of equation
/// (21) from Laviron, JEAC 101, 19-28, 1979.
/// rate is what Laviron calls "m" in his paper

double Functions::trumpetBV(double rate, double alpha, double prec)
{
  alpha = 1 - alpha;
  double time = std::max(-0.5, 1/alpha*(log(alpha*rate)) - 3);
  double cur = 0;
  double pos = time;

  double max = time + 9;

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
  int i = 0;
  while(time < max) {
    time += prec;
    slv.stepTo(time);
    double c = dy(time, slv.currentValues()[0]);
    if(c > cur && i > 3)
      return pos;
    cur = c;
    pos = time;
    ++i;
  }

  return -1;                    // Hmmm
}

//////////////////////////////////////////////////////////////////////
// Numerical integration of the MHC integral.
//
// See Fourmond and Léger, 2020, submitted, for more information.


static Credits mhc_z("Zeng et al, JEAC, 2014", "the k_mhc_z function", "10.1016/j.jelechem.2014.09.038");

double Functions::marcusHushChidseyZeng(double lambda, double eta)
{
  double a = 1 + sqrt(lambda);
  return sqrt(M_PI * lambda)/(1 + exp(- eta)) *
    gsl_sf_erfc((lambda - sqrt(a + eta*eta))/(2 * sqrt(lambda)));
}

static Credits mhc_n("Nahir, JEAC, 2002", "the k_mhc_n function", "10.1016/j.jelechem.2014.09.038");

double Functions::marcusHushChidseyNahir(double lambda, double eta)
{
  
  double d = lambda - eta;
  if(eta < 0)
    return exp(eta) * marcusHushChidseyNahir(lambda, -eta);
  return
    sqrt(M_PI * lambda) * gsl_sf_erfc((lambda - eta)/(2 * sqrt(lambda))) +
    M_PI * M_PI/(12) * d/lambda * exp(- d*d/(4*lambda));
}

// The bieniasz series
static Credits mhc_b("Bieniasz, JEAC, 2012", "the k_mhc_double function", "10.1016/j.jelechem.2012.08.015");

static double bienasz_coeffs[] = {
   0.99999999999999997912, 
  -0.99999999999997901750,
   0.99999999999649137345,
  -0.99999999976654504822,
   0.99999999175111244184,
  -0.99999982086713785850,
   0.99999738969001426153,
  -0.99997294898751059018,
   0.99979231988822602472,
  -0.99878284608559524864,
   0.99442927053002735641,
  -0.97973079156980086861,
   0.94050839583467421321,
  -0.85733314214466591696,
   0.71694987446987527278,
  -0.52884842856283162473,
   0.33020050014355901079,
  -0.16702464086638103149,
   0.065001468903985759528,
  -0.018122105322750317162,
   0.0032045362066109155393,
  -0.00026902324137910826429
};

static double log_eerfc(double y)
{
  return y*y +  gsl_sf_log_erfc(y);
}


/**
   Bieniasz variant of the Oldham and Myland series with much faster
   convergence.
 */
double Functions::marcusHushChidseyDouble(double lambda, double eta)
{
  if(eta < 0)
    return exp(eta) * marcusHushChidseyDouble(lambda, -eta);
  double sl = sqrt(lambda);
  double sum = 0;
  double fact = 1;
  double d = lambda - eta;
  double pf = d*d/(4*lambda);
  for(int i = 0; i < sizeof(bienasz_coeffs)/sizeof(double); i++) {
    double e1 = exp(log_eerfc(sl * (i + 0.5 + eta/(2*lambda))) - pf);
    double e2 = exp(log_eerfc(sl * (i + 0.5 - eta/(2*lambda))) - pf);
    sum += bienasz_coeffs[i] * (e1 + e2);
  }
  return sum * sqrt(M_PI * lambda);
}


// Smart trapezoids
static double mhc_trapezoid(double lambda, double eta, double step)
{
  
  double v = 0;
  double c = lambda - eta;
  double w = 9*sqrt(lambda);
  if(lambda > eta) {
    if(w < lambda - eta + 0.5*w)
      w = lambda - eta + 0.5*w;
  }
  int nbsteps = 3*ceil(w/step);

  double alpha = exp(-step*step/(4*lambda));
  double alpha_2 = alpha*alpha;
  double a_cur = alpha;

  double beta = exp(step);
  double beta_m1 = 1/beta;
  double gamma = exp(c);
  double den = 1;
  double den_m1 = 1;
  double num = 1;

  for(int i = 0; i<= nbsteps; i++) {
    if(i > 0) {
      num *= a_cur;
      a_cur *= alpha_2;
      double dv = num/(1 + gamma * den) +
        num/(1 + gamma * den_m1);
      v += dv;
      if(dv < v*1e-12)
        break;
    }
    else
      v += 1/(1+gamma);
    den *= beta;
    den_m1 *= beta_m1;
  }
  return v*step;
}

double Functions::marcusHushChidsey(double lambda, double eta)
{
  if(eta < 0)
    return exp(eta) * mhc_trapezoid(lambda, -eta, 1);
  else
    return mhc_trapezoid(lambda, eta, 1);
}
