/**
   @file exponential-fits.cc
   Exponential-based fits.
   Copyright 2011, 2013 by Vincent Fourmond

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
#include <perdatasetfit.hh>
#include <dataset.hh>
#include <vector.hh>

#include <argumentmarshaller.hh>
#include <commandeffector.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>

#include <odesolver.hh>
#include <solver.hh>

#include <gsl/gsl_const_mksa.h>

#include <soas.hh>



/// A fit to study the reactivations of enzymes...
class ExponentialReactivationFit : public FunctionFit {

  /// The number of exponentials
  int exponentials;

  /// Whether we include a dead phase or not.
  bool dead;

  virtual void processOptions(const CommandOptions & opts)
  {
    exponentials = 2;
    updateFromOptions(opts, "exponentials", exponentials);

    dead = false;
    updateFromOptions(opts, "dead", dead);
  }

public:

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;

    // Starting time, always at t = 0;
    defs << ParameterDefinition("t0", true);
    defs << ParameterDefinition("ilim", true); // In general, fixed
    defs << ParameterDefinition("alpha_active");
    if(dead)
      defs << ParameterDefinition("alpha_dead");

    /// Exponentials time constants:
    for(int j = 0; j < exponentials; j++) {
      defs << ParameterDefinition(QString("tau_%1").arg(j+1));
      if(j != exponentials - 1)
        defs << ParameterDefinition(QString("alpha_%1").arg(j+1));
    }

    return defs;
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    double deltax = ds->x().last() - ds->x().first();
    a[0] = ds->x()[0];          // x0 = x[0]
    a[1] = ds->y().last();      // i0 to be given
    a[2] = 0.1;                 // arbitrary
    int i = 3;
    if(dead)
      a[i++] = 0.1;
    
    for(int j = 0; j < exponentials; j++) {
      a[i++] = deltax*pow(4, j-1);
      if(j != exponentials -1)
        a[i++] = 0.1;
    }
  };

  virtual double function(const double * a, 
                          FitData * , double x) {
    double val = 1;
    double tt0 = x - a[0];
    int i = 2;
    double last = 1 - a[i++];

    if(dead) {
      double d = a[i++];
      last -= d;
      val -= d;
    }
    for(int j = 0; j < exponentials; j++) {
      double tau = a[i++];
      double ampl = (j < exponentials -1 ? a[i++] : last);
      last -= ampl;
      val += - ampl * exp(-tt0/tau);
    }
      
    return a[1] * val;
  };

  virtual ArgumentList * fitHardOptions() const {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new IntegerArgument("exponentials", 
                                          "Number of exponentials",
                                          "Number of exponentials")
                   << new 
                   BoolArgument("dead", 
                                "Dead",
                                "Whether we include a dead phase or not"));
    return opts;
  };

  ExponentialReactivationFit() :
    FunctionFit("react-exp", 
                "Exponential reactivation",
                "Reactivation", 1, -1, false) 
  { 
    makeCommands();
  };
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
ExponentialReactivationFit fit_react_exp;


//////////////////////////////////////////////////////////////////////

/// Private fits for the anaerobic inactivation of FeFe hydrogenases.
///
/// Model: [0, 1, 2] is [A, I', I'']
///
/// * k'_i and k''_i constant
/// * k'_a = k'_a_overox + k'_a_ox0 * exp( - F * E/RT)
/// * k'' = ...
/// * k_loss (applying to A only) = k_loss_0 * exp( - F * E/2*RT)
///
/// Additional parameters:
/// * scan rate
/// * temperature of course
///
///
/// function() is not thread-safe.
class FeFeHighPotentialInactivation : 
  public PerDatasetFit, public ODESolver {

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
  }

  
  virtual QString optionsString() const {
    return "";
  }

  /// Internal parameter storage
  double kp_i, kpp_i;
  double kp_a_overox, kpp_a_overox;
  double kp_a_ox0, kpp_a_ox0;
  double k_loss_0;
  double scan_rate;
  double fara;
  double overall_loss;

  double vertex_value;


  void copyParameters(const double * params, const Vector&x) {
    fara = GSL_CONST_MKSA_FARADAY 
      /(params[0] * GSL_CONST_MKSA_MOLAR_GAS);

    for(int i = 1; i <= 7; i++)
      if(params[i] < 0)
        throw RangeError(QString("Negative rate constant ratio: #%1").arg(i));
    
    kp_i = params[1];
    kpp_i = params[2];
    kp_a_overox = params[3];
    kp_a_ox0 = params[4];
    kpp_a_overox = params[5];
    kpp_a_ox0 = params[6];
    k_loss_0 = params[7];
    overall_loss = params[8];
    scan_rate = params[9];
    vertex_value = x.max();     // Enough ?  Don't try successive
                                // scans, though ;-)...
  };

public:

  virtual int dimension() const {
    return 3;
  };

  virtual int computeDerivatives(double t, const double *y, double *dydt) {
    const double e = vertex_value - fabs(t * scan_rate);

    const double kloss = k_loss_0 * exp(fara * e * 0.5);
    double kp_a = kp_a_overox + kp_a_ox0 * exp(-fara * e);
    // We implement a cutoff for the activation rate constants
    if(kp_a > 1e2)
      kp_a = 1e2;
    double kpp_a = kpp_a_overox + kpp_a_ox0 * exp(-fara * e);
    if(kpp_a > 1e2)
      kpp_a = 1e2;

    dydt[0] = - y[0] * (kp_i + kpp_i + kloss + overall_loss) 
      + y[1] * kp_a + y[2] * kpp_a;
    dydt[1] = y[0] * kp_i  - y[1] * (kp_a + overall_loss);
    dydt[2] = y[0] * kpp_i - y[2] * (kpp_a + overall_loss);

    // QTextStream o(stdout);
    // o << "Potential " << e << "\n"
    //   << "kp_a = " << kp_a << "\tkp_i = " << kp_i << "\n"
    //   << "kpp_a = " << kpp_a << "\tkpp_i = " << kpp_i << endl;
    
    return GSL_SUCCESS;
  };
    


  /// Formula:
  virtual void function(const double * params, FitData * , 
                        const DataSet * ds , gsl_vector * target) {

    const Vector & xv = ds->x();
    copyParameters(params, xv);

    double mul = -1/scan_rate;
    double t0 = mul * (vertex_value - xv[0]);
    double init[3] = {1, 0, 0};
    initialize(init, t0);
    

    for(int i = 0; i < xv.size(); i++) {
      const double x = xv[i];
      const double t = mul * (vertex_value - x);
      if(t == 0)
        mul *= -1;              // Switch at 0 !
      stepTo(t);
      gsl_vector_set(target, i, currentValues()[0]);
    }
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = soas().temperature();
    a[1] = 2e-2; 
    a[2] = 5e-2; 
    a[3] = 1e-2; 
    a[4] = 1;
    a[5] = 1e-1;
    a[6] = 1;
    a[7] = 1e-4;
    a[8] = 1e-4;
    a[9] = 0.02;
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;
    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("k'_i")
         << ParameterDefinition("k''_i")
         << ParameterDefinition("k'_a_overox") // params[3]
         << ParameterDefinition("k'_a_ox0") 
         << ParameterDefinition("k''_a_overox") // params[5]
         << ParameterDefinition("k''_a_ox0") 
         << ParameterDefinition("k_loss0") 
         << ParameterDefinition("k_film") 
         << ParameterDefinition("nu", true); 
    return defs;
  };


  FeFeHighPotentialInactivation() :
    PerDatasetFit("fefe-hp-inact", 
                  "High potential inactivation of FeFe hydrogenases",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
FeFeHighPotentialInactivation fit_fefe_inact;

//////////////////////////////////////////////////////////////////////

/// Fits to an irreversible EEC irreversible model with transport
class EECTransportFit : public PerDatasetFit {
  /// Whether or not the current plateaus off at extreme potentials.
  bool plateau;

  /// Whether or not we actually take transport into account.
  bool transport;

  /// Whether the current is a reduction current (default) or an
  /// oxidation current
  bool isOxidation;

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
    plateau = false;
    updateFromOptions(opts, "plateau", plateau);

    transport = true;
    updateFromOptions(opts, "transport", transport);

    isOxidation = true;
    updateFromOptions(opts, "oxidation", isOxidation);
  }

  
  virtual QString optionsString() const {
    return QString("%1, %2 transport, %3").
      arg(plateau ? "reaching plateau" : "not reaching plateau").
      arg(transport ? "with" : "without").
      arg(isOxidation ? "oxidation" : "reduction");
  }

public:

  double current(double a, double b_rl, double kcat_ov_k0, 
                 double cur, double bd0 = 0)
  {
    double b = b_rl * kcat_ov_k0;
    if(plateau)
      return cur / a * 
        (1 + log((a + b)/
                 (a + b * exp(bd0)))/bd0);
      else
        return cur / a * log((a + b)/b);
  }

  /// Formula:
  virtual void function(const double * params, FitData * , 
                        const DataSet * ds , gsl_vector * target) {

    const Vector & xv = ds->x();
    double f = GSL_CONST_MKSA_FARADAY /(params[0] * GSL_CONST_MKSA_MOLAR_GAS);

      if(params[3] < 0 || params[4] < 0)
        throw RangeError("Negative rate constant ratio");

    double cur = params[5];
    double kappa = params[3];
    double bd0 = (plateau ? params[6] : 0);

    double km = 0;
    double s_inf = 0;
    // Coefficient to convert concentration gradient in mM to current
    double conv = 0;
    if(transport) {
      // Now, we elaborate...
      //
      // 1 mM = 1 mol/m^3 = 1e-6 mol.cm^-3
      const double * base = params + (plateau ? 7 : 6);
      const double &D = base[3];
      const double &S = base[2];
      double omega = base[4] * 2 * M_PI/60; // rads/second
      const double &nu = base[5];

      // double fct = pow(D * D * D * D * omega * omega * omega / nu, 1.0/6.0);
      double fct = pow(D, 2.0/3.0) *sqrt(omega)/pow(nu, 1.0/6.0);

      // QTextStream o(stdout);
      // o << "D = " << D << "\tS = " << S 
      //   << "\tomega = " << omega << " (= " << base[4] << " RPMS)"
      //   << "\tnu = " << nu << "\tfct = " << fct
      //   << endl;

      conv = 2 * GSL_CONST_MKSA_FARADAY * 0.620 * 1e-6 * S * fct;
      if(! isOxidation)
        conv = - conv;

      km = base[0];
      s_inf = base[1];
    }

    double last = 0;
    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];

      double d1 = exp(f * 0.5 * (x - params[1]));
      double e1 = d1 * d1;
      double d2 = exp(f * 0.5 * (x - params[2]));
      double e2 = d2 * d2;
      
      // b without the kcat/k0 factor
      double b_rl = (isOxidation ?
                     (d1 * (1 + e2) + e1 * d2/kappa)/(e1 * e2) :
                     (d1 + d2/kappa * (1 + e1))
                     );
      double a = 1 + e2*(1 + e1);
      if(isOxidation)
        a /= (e1 * e2);

      double v = current(a, b_rl, params[4], cur,
                         bd0);

      // Hmmm, now transport.
      if(transport) {
        LambdaSolver slv([&, this](double x) -> double {
            double s0 = s_inf - x/conv;
            double mm_fact = s0/(s0 + km);
            double kc_ov_k0 = params[4] * mm_fact;
            return  x - mm_fact * current(a, b_rl, kc_ov_k0, cur,
                                          bd0);
          });
        double ov = v;

        // Actually, the initial parameter is critical. Using the last
        // one just works if the voltammogram starts from 0. If not,
        // maybe we can use a little less than s_inf as the starting value ?
        double i_lim = conv * s_inf;

        if(fabs(v) > fabs(i_lim))
          v = i_lim;
        v = slv.solve((i > 0 ? last : v));
      }

      last = v;
      gsl_vector_set(target, i, v);
    }
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = soas().temperature();
    a[1] = -0.3; 
    a[2] = -0.6; 
    a[3] = 1; 
    a[4] = 10; 
    a[5] = (isOxidation ? 1e-5 : -1e-5);

    double * base = a + 6;
    if(plateau) {
      a[6] = 1;
      base++;
    }

    // Transport-related stuff:
    if(transport) {
      base[0] = 0.01;           // Km
      base[1] = 1;              // s_inf
      base[2] = 0.07;           // cm^2, 3 mm diameter
      base[3] = 1e-5;           // cm^2/s
      base[4] = 5000;           // 5 krpm
      base[5] = 0.01;           // cm^2/s, that of water
    }
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;
    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("E1")
         << ParameterDefinition("E2")
         << ParameterDefinition("k02/k01")
         << ParameterDefinition("kcat/k0");
    if(plateau)
      defs << ParameterDefinition("ilim")
           << ParameterDefinition("betad0");
    else
      defs << ParameterDefinition("ilim/betad0");

    // Transport related things
    if(transport)
      defs << ParameterDefinition("km")          // in mM
           << ParameterDefinition("s_inf", true) // bulk concentration (mM)
           << ParameterDefinition("S", true)     // electrode surface (cm^2)
           << ParameterDefinition("D", true) // diffusion coefficent cm^2/s
           << ParameterDefinition("omega", true) // electrode rotation (rpm)
           << ParameterDefinition("nu", true); // kinematic viscosity cm^2/s
    
    // Add the transport-related stuff
    return defs;
  };

  virtual ArgumentList * fitHardOptions() const {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new 
                   BoolArgument("plateau", 
                                "Plateau",
                                "Whether to use the general expression or "
                                "only that valid when plateaus are not reached")
                   << new 
                   BoolArgument("transport", 
                                "Transport",
                                "If transport is truly on")
                   << new 
                   BoolArgument("oxidation", 
                                "...",
                                "???")
                   );
    return opts;
  };

  EECTransportFit() :
    PerDatasetFit("eec-transport", 
                  "Fit of an EEC catalytic wave with transport",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  }
};
  

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
EECTransportFit fit_eectranport;
