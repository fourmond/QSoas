/**
   @file misc-fits.cc
   Various fits...
   Copyright 2011 by Vincent Fourmond

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

#include <terminal.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <soas.hh>

#include <fitdata.hh>
#include <fitdialog.hh>

#include <gsl/gsl_const_mksa.h>

class SlowScanLowPotFit : public PerDatasetFit {
  bool explicitRate;

protected:
  virtual void processOptions(const CommandOptions & opts)
  {
    explicitRate = false;
    updateFromOptions(opts, "explicit-rate", explicitRate);
  }

  
  virtual QString optionsString() const {
    return explicitRate ? "explicit rate" : "implicit rate";
  }

public:


  /// Formula:
  virtual void function(const double * a, FitData * , 
                        const DataSet * ds , gsl_vector * target) {
    
    double fara = GSL_CONST_MKSA_FARADAY /(a[0] * GSL_CONST_MKSA_MOLAR_GAS);
    double alpha = (explicitRate ? a[6] : a[5]);

    double D = (explicitRate ? pow(10, a[5])/(alpha * fara * a[4])
                : pow(10, a[4]));

    const Vector & xv = ds->x();
    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      double arg = exp(-alpha*fara*x) - exp(-alpha*fara*xv[0]);
      gsl_vector_set(target, i, 
                     (1 - a[3] * exp(-D * arg) )
                     *(a[1] * x + a[2]));
    }
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ,
                            double * a)
  {
    a[0] = soas().temperature();
    a[1] = 1; a[2] = 1; 
    if(explicitRate) {
      a[4] = 5e-4;
      a[5] = 1;
      a[6] = 1;
    }
    else {
      a[4] = 1;
      a[5] = 1;
    }
      
    a[3] = 0.5;
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> params;
    params << ParameterDefinition("temperature", true)
           << ParameterDefinition("a")
           << ParameterDefinition("b")
           << ParameterDefinition("c");
    if(explicitRate)
      params << ParameterDefinition("nu", true)
             << ParameterDefinition("log(k)");
    else
      params << ParameterDefinition("log(D)");
        

    params << ParameterDefinition("alpha");

    return params;
  };


  SlowScanLowPotFit() : PerDatasetFit("slow-scan-lp", 
                                      "Slow scan test",
                                      "Slow scan", 1, -1, false)  { 
    explicitRate = false;
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new 
                   BoolArgument("explicit-rate", 
                                "Explicit rate",
                                "Whether the scan rate is an explicit "
                                "parameter of the fit")
                   );
    makeCommands(NULL, NULL, NULL, opts);

;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
SlowScanLowPotFit fit_slow_scan_low_pot;


class SlowScanHighPotFit : public PerDatasetFit {

  /// Whether we have a bi-exponential decay or not.
  bool biExp;

  /// Whether or not we add an additional constant factor
  bool scaling;

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
    biExp = false;
    updateFromOptions(opts, "bi-exp", biExp);
    scaling = false;
    updateFromOptions(opts, "scaling", scaling);
  }

  
  virtual QString optionsString() const {
    return QString(biExp ? "bi-exponential" : "mono-exponential") + 
      QString(scaling ? " with " : " without ") + "scaling";
  }

public:


  /// Formula:
  virtual void function(const double * a, FitData * , 
                        const DataSet * ds , gsl_vector * target) {

    const Vector & xv = ds->x();

    bool first_bit = true;
    double delta_E = xv[1] - xv[0];
    double E_vertex = 0;
    double scale = 1;
    if(scaling)
      scale = a[biExp ? 9 : 7];
    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      double t;
      if(first_bit)
        t = (x - a[0])/a[1];
      else
        t = (2*E_vertex - a[0] - x)/a[1];
      
      double dec = (biExp ? 
                    (1 - a[8]) * exp(-t/a[6]) + 
                    a[8] * exp(-t/a[7])
                    : exp(-t/a[6]));
      double act = a[4] + (a[5] - a[4]) * dec;

      gsl_vector_set(target, i, 
                     (a[2] * x + a[3]) * act * scale);

      if(first_bit && i < xv.size() - 1 && 
         ( (xv[i+1] - xv[i])*delta_E < 0)) {
        first_bit = false;
        E_vertex = xv[i];
      }
    }

  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = ds->x()[0];
    a[1] = 5e-4; 
    a[2] = 1e-6; 
    a[3] = 1e-6; 
    a[4] = 0.1; 
    a[5] = 1;
    a[6] = 300;
    int base = 7;
    if(biExp) {
      a[7] = 800;
      a[8] = 0.3;
      base = 9;
    }
    if(scaling)
      a[base] = 1;
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> ret;
    ret << ParameterDefinition("E1", true) // Potentiel initial
        << ParameterDefinition("nu", true) // vitesse de balayage
        << ParameterDefinition("a") //2
        << ParameterDefinition("b") //3
        << ParameterDefinition("alpha_e") //4
        << ParameterDefinition("alpha_0") //5
        << ParameterDefinition("tau");
    
    if(biExp)
      ret << ParameterDefinition("tau_slow")
          << ParameterDefinition("frac_slow");
    // Fraction of the slow phase with respect to the total
    // inactivation.
    if(scaling)
      ret << ParameterDefinition("fact"); // prefactor
    
    return ret;
  };


  SlowScanHighPotFit() : 
    PerDatasetFit("slow-scan-hp", 
                  "Slow scan test",
                  "Slow scan", 1, -1, false), biExp(false) { 
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new 
                   BoolArgument("bi-exp", 
                                "Bi exp",
                                "Whether we have a bi-exponential or a "
                                "mono-exponential")
                   << new 
                   BoolArgument("scaling", 
                                "Scaling",
                                "Use an additional scaling factor")
                   );
    makeCommands(NULL, NULL, NULL, opts);
  };
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
SlowScanHighPotFit fit_slow_scan_high_pot;



/// Fits to the EECR model of electrochemical waves
class EECRFit : public PerDatasetFit {
  /// Whether or not the current plateaus off at extreme potentials.
  bool plateau;

  /// Whether we use Eoc or k2/k_m2
  bool useEoc;

  /// Whether the current is a reduction current (default) or an
  /// oxidation current
  bool isOxidation;

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
    plateau = false;
    useEoc = false;
    updateFromOptions(opts, "plateau", plateau);
    updateFromOptions(opts, "use-eoc", useEoc);
    updateFromOptions(opts, "oxidation", isOxidation);
  }

  
  virtual QString optionsString() const {
    return QString("%1, %2, %3").
      arg(plateau ? "reaching plateau" : "not reaching plateau").
      arg(useEoc ? "eoc" : "bias").
      arg(isOxidation ? "oxidation" : "reduction");
  }

public:


  /// Formula:
  virtual void function(const double * params, FitData * , 
                        const DataSet * ds , gsl_vector * target) {

    const Vector & xv = ds->x();
    double f = GSL_CONST_MKSA_FARADAY /(params[0] * GSL_CONST_MKSA_MOLAR_GAS);

    for(int i = 3; i <= (useEoc ? 4 : 5); i++)
      if(params[i] < 0)
        throw RangeError(QString("Negative rate constant ratio: #%1").arg(i));

    double cur = params[6];
    double bias = params[5];
    if(useEoc)
      bias = exp(f * (params[5] - params[1])) * 
        exp(f * (params[5] - params[2]));
    
    if(isOxidation)
      cur = -cur * bias;

    if(cur > 0)
      throw RangeError("Positive reduction current");

    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      double e1 = exp(f * (x - params[1]));
      double e2 = exp(f * (x - params[2]));
      
      double b = params[4] * (bias * 
                              (sqrt(e1) + sqrt(e2)/params[3] * (1 + e1)) +
                              sqrt(e1) * (1 + e2) + e1 * sqrt(e2)/params[3]);
      double a = 1 + e2*(1 + e1);
      double ap = e1*e2/bias;
      
      double v;
      if(plateau)
        v = cur * (1 - ap)/a * 
          (1 + log((bias * a + b)/
                   (bias * a + b * exp(params[7])))/params[7]);
      else
        v = cur * (1 - ap)/a * log((bias * a + b)/b);
      gsl_vector_set(target, i, v);
    }
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = soas().temperature();
    a[1] = -0.6; 
    a[2] = -0.3; 
    a[3] = 1; 
    a[4] = 10; 
    a[5] = (useEoc ? -0.66 : 2);
    a[6] = (isOxidation ? 1e-5 : -1e-5);
    if(plateau)
      a[7] = 1;
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;
    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("E1")
         << ParameterDefinition("E2")
         << ParameterDefinition("k02/k01")
         << ParameterDefinition("k2/k0");
    if(useEoc)
      defs << ParameterDefinition("Eoc"); // parameter 5
    else
      defs << ParameterDefinition("k2/k-2"); 
    if(plateau)
      defs << ParameterDefinition("ilim")
           << ParameterDefinition("betad0");
    else
      defs << ParameterDefinition("ilim/betad0");
    return defs;
  };


  EECRFit() :
    PerDatasetFit("eecr-wave", 
                  "Fit of an EECR catalytic wave",
                  "...", 1, -1, false) 
  { 
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new 
                   BoolArgument("plateau", 
                                "Plateau",
                                "Whether to use the general expression or "
                                "only that valid when plateaus are not reached")
                   << new 
                   BoolArgument("oxidation", 
                                "...",
                                "???")
                   << new 
                   BoolArgument("use-eoc", 
                                "Use open circuit",
                                "Whether to use explicitly the bias or compute "
                                "it using the open circuit potential")
                   );
    makeCommands(NULL, NULL, NULL, opts);
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
EECRFit fit_eecr;


/// Fits to the EECR + relay model of electrochemical waves
class EECRRelayFit : public PerDatasetFit {

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
  }

  
  virtual QString optionsString() const {
    return "";
  }

public:


  /// Formula:
  virtual void function(const double * params, FitData * , 
                        const DataSet * ds , gsl_vector * target) {

    const Vector & xv = ds->x();
    const double f = GSL_CONST_MKSA_FARADAY 
      /(params[0] * GSL_CONST_MKSA_MOLAR_GAS);

    for(int i = 2; i <= 7; i++)
      if(params[i] < 0)
        throw RangeError(QString("Negative rate constant ratio: #%1").arg(i));

    if(params[8] > 0)
      throw RangeError("Positive reduction current");

    // We rename the parameters...
    //
    // Let's keep in mind that k2 is one.
    const double k_0M    = params[2];
    const double k_2     = 1;
    const double k_m2    = params[3];
    const double k_1     = params[4];
    const double k_m1    = params[4]/params[5];
    const double kp_1    = params[6];
    const double kp_m1   = params[6]/params[7];
    const double ilim    = params[8];
    const double beta_d0 = params[9];
    const double k_0m    = k_0M * exp(-beta_d0);


    // We now precompute the  coefficients of all the A,B,C,D,E,F constants:
    // The number is the power of d.
    //
    // These come straight from Maple
    const double A_0 = k_m1*k_2*kp_m1;
    const double A_4 = -k_1*k_m2*kp_1;

    const double B_0 = kp_m1*k_m2+k_2*kp_m1+k_2*k_m1+k_m1*kp_m1;
    const double B_2 = k_2*k_m1 + kp_1*k_m1 + k_2*kp_m1 + k_1*k_m2 + 
      kp_1*k_m2 + kp_m1*k_m2 + k_2*k_1;
    const double B_4 = k_1*kp_1+k_1*k_m2+k_2*k_1+kp_1*k_m2;

    const double C_1 = kp_m1*k_1*k_m2+k_m1*k_2*kp_m1 + 
      kp_1*k_2*k_m1+k_2*kp_m1*k_1;
    const double C_3 = 2*kp_1*k_1*k_m2+k_m2*kp_m1*k_m1+2*kp_m1*k_1*k_m2 + 
      2*k_m1*k_2*kp_m1+kp_1*k_2*k_1+2*k_2*kp_m1*k_1;
    const double C_5 = kp_1*k_1*k_m2+kp_1*k_m1*k_m2 + 
      kp_m1*k_1*k_m2+k_2*kp_m1*k_1;

    const double D_2 = 2*k_2*k_2*kp_m1*k_m1+kp_1*k_2*k_m2*k_m1 +
      k_2*k_2*kp_m1*k_1+2*k_2*k_m2*kp_m1*k_m1 + 
      2*kp_m1*k_1*k_2*k_m2+kp_m1*k_1*k_m2*k_m2;
    const double D_4 = kp_m1*k_1*k_m2*k_m2+2*kp_m1*k_1*k_2*k_m2 + 
      2*kp_1*k_2*k_m2*k_1+2*kp_1*k_1*k_m2*k_m2 + 
      k_2*k_2*kp_m1*k_1+kp_1*k_2*k_m2*k_m1;

    const double E_0 = 1;
    const double E_2 = 1;
    
    const double F_1 = k_2 + k_m2;
     


    // The prefactor, dependent on ilim but not only.
    double pref = ilim * B_0/(A_0);

    for(int i = 0; i < xv.size(); i++) {
      const double x = xv[i];
      const double d = exp(0.5 * f * (x - params[1]));
      
      const double A = A_0 + A_4 * pow(d, 4);
      const double B = B_0 + B_2 * pow(d, 2) + B_4 * pow(d, 4);
      const double C = C_1 * d + C_3 * pow(d, 3) + C_5 * pow(d, 5);
      const double D = D_2 * pow(d, 2) + D_4 * pow(d, 4);
      const double E = E_0 + E_2 * pow(d, 2);
      const double F = F_1 * d;

      const double Gamma1_M = B * k_0M*k_0M * E + k_0M * (B * F + C) + D;
      const double Gamma1_m = B * k_0m*k_0m * E  + k_0m * (B * F + C) + D;
      const double g2       = B*B*F*F + 2 * B*F*C + C*C - 4*D*B*E;
      const double Gamma2   = sqrt(fabs(g2));
      const double Gamma3_M = 2 * B * E * k_0M + B*F + C;
      const double Gamma3_m = 2 * B * E * k_0m + B*F + C;



      double delta;
      // the argument of arctan(h)?
      const double arg = Gamma2 * (Gamma3_M - Gamma3_m)/
        (g2 - Gamma3_M * Gamma3_m);
      if(g2 > 0)
        delta = atanh(arg);
      else 
        delta = atan(arg);

      const double rest = log(Gamma1_M/Gamma1_m);

      // if(fabs(g2) < 1e2)
      //   fprintf(stderr, "Values x=%g\tg2=%g\tdelta=%g\trest=%g\tfact=%g"
      //           "\td/g2=%g\n", 
      //           x, g2, delta, rest, (B*F - C)/Gamma2, delta/Gamma2);

      const double par =  rest + (B*F - C)/Gamma2 * 2 * delta;
      gsl_vector_set(target, i, pref * A/(2 * B) * par);
    }
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = soas().temperature();
    a[1] = -0.6; 
    a[2] = 1; 
    a[3] = 1; 
    a[4] = 10;
    a[5] = 2;
    a[6] = 2;
    a[7] = 1;
    a[8] = -1e-5;
    a[9] = 1;
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;
    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("Er")
         << ParameterDefinition("k0r/k2")
         << ParameterDefinition("km2/k2") // params[3]
         << ParameterDefinition("k1/k2") 
         << ParameterDefinition("k1/km1") 
         << ParameterDefinition("kp1/k2") // params[6]
         << ParameterDefinition("kp1/kpm1") 
         << ParameterDefinition("ilim")
         << ParameterDefinition("betad0");
    return defs;
  };


  EECRRelayFit() :
    PerDatasetFit("eecr-relay-wave", 
                  "Fit of an EECR+relay catalytic wave",
                  "...", 1, -1, false) 
  { 
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   );
    makeCommands(NULL, NULL, NULL, opts);
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
EECRRelayFit fit_ececr_relay;
