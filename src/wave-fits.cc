/**
   @file wave-fits.cc
   Fits for the "wave shape", based on the Fourmond JACS 2013 paper
   Copyright 2013, 2014, 2015, 2016 by CNRS/AMU

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
#include <argument-templates.hh>
#include <general-arguments.hh>
#include <soas.hh>

#include <fitdata.hh>

#include <gsl/gsl_const_mksa.h>

typedef enum {
  Nernst,
  SlowET,
  Dispersion,
  Full
} ShapeApproximation;


QStringList approxNames = QStringList()
  << "nernst"
     << "slow-et"
        << "dispersion"
           << "full";

QList<ShapeApproximation> approxValues({Nernst, SlowET, Dispersion, Full});



/// Fits to the ECR model of electrochemical waves
class ECRFit : public PerDatasetFit {
  class Storage : public FitInternalStorage {
  public:
    /// Whether or not the current plateaus off at extreme potentials.
    bool plateau;

    /// Whether we use Eoc or k2/k_m2
    bool useEoc;

    /// Whether the current is a reduction current (default) or an
    /// oxidation current
    bool isOxidation;
  };
    
protected:
    virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const {
    return deepCopy<Storage>(source);
  };


  virtual void processOptions(const CommandOptions & opts, FitData * data)
  {
    Storage * s = storage<Storage>(data);
    s->plateau = false;
    s->useEoc = false;
    s->isOxidation = false;
    updateFromOptions(opts, "plateau", s->plateau);
    updateFromOptions(opts, "use-eoc", s->useEoc);
    updateFromOptions(opts, "oxidation", s->isOxidation);
  }

  
  virtual QString optionsString(FitData * data) const {
    Storage * s = storage<Storage>(data);
    return QString("%1, %2, %3").
      arg(s->plateau ? "reaching plateau" : "not reaching plateau").
      arg(s->useEoc ? "eoc" : "bias").
      arg(s->isOxidation ? "oxidation" : "reduction");
  }

public:


  /// Formula:
  virtual void function(const double * params, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const {
    Storage * s = storage<Storage>(data);

    const Vector & xv = ds->x();
    double f = GSL_CONST_MKSA_FARADAY /(params[0] * GSL_CONST_MKSA_MOLAR_GAS);

    for(int i = 2; i <= 3; i++)
      if(params[i] < 0)
        throw RangeError(QString("Negative rate constant ratio: #%1").arg(i));

    double cur = params[4];
    double bias = params[3];

    if(cur > 0)
      throw RangeError("Positive reduction current");

    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      double eone = exp(f * (x - params[1]));
      
      double b = params[2] * sqrt(eone) * (1 + 1/bias);
      double a = 1 + eone;
      double ap = eone/bias;
      
      double v;
      if(s->plateau)
        v = cur * (1 - ap)/a * 
          (1 + log((bias * a + b)/
                   (bias * a + b * exp(params[5])))/params[5]);
      else
        v = cur * (1 - ap)/a * log((bias * a + b)/b);
      gsl_vector_set(target, i, v);
    }
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * /*ds*/,
                            double * a) const
  {
    Storage * s = storage<Storage>(data);
    a[0] = soas().temperature();
    a[1] = -0.6; 
    a[2] = 1; 
    a[3] = 2; 
    a[4] = -1e-5;
    if(s->plateau)
      a[5] = 1;
  };

  virtual QList<ParameterDefinition> parameters(FitData * data) const {
    Storage * s = storage<Storage>(data);
    QList<ParameterDefinition> defs;
    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("E0")
         << ParameterDefinition("k2/k0");
    defs << ParameterDefinition("k2/k-2"); 
    if(s->plateau)
      defs << ParameterDefinition("ilim")
           << ParameterDefinition("betad0");
    else
      defs << ParameterDefinition("ilim/betad0");
    return defs;
  };

  virtual ArgumentList * fitHardOptions() const {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new 
                   TemplateChoiceArgument<ShapeApproximation>
                   (approxNames, approxValues,
                    "approximation",
                    "Approximation", 
                    "the kind of approximation used for the computation (default: dispersion)")
                   << new 
                   BoolArgument("oxidation", 
                                "Reference is oxidation",
                                "if on, use the oxidation current as reference (default: off)")
                   << new 
                   BoolArgument("use-eoc", 
                                "Use open circuit",
                                "whether to use explicitly the bias or compute "
                                "it using the open circuit potential (default: false)")
                   );
    return opts;
  };

  ECRFit() :
    PerDatasetFit("ecr-wave", 
                  "Fit of an ECR catalytic wave",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
ECRFit fit_ecr;

/// Fits to the EECR model of electrochemical waves
class EECRFit : public PerDatasetFit {

  class Storage : public FitInternalStorage {
  public:
    /// Whether or not the current plateaus off at extreme potentials.
    bool plateau;

    /// Whether we use Eoc or k2/k_m2
    bool useEoc;

    /// Whether the current is a reduction current (default) or an
    /// oxidation current
    bool isOxidation;
  };
  
protected:

  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const {
    return deepCopy<Storage>(source);
  };


  virtual void processOptions(const CommandOptions & opts, FitData *data) const
  {
    Storage * s = storage<Storage>(data);
    s->plateau = false;
    s->useEoc = false;
    s->isOxidation = false;
    updateFromOptions(opts, "plateau", s->plateau);
    updateFromOptions(opts, "use-eoc", s->useEoc);
    updateFromOptions(opts, "oxidation", s->isOxidation);
  }

  
  virtual QString optionsString(FitData * data) const {
    Storage * s = storage<Storage>(data);
    return QString("%1, %2, %3").
      arg(s->plateau ? "reaching plateau" : "not reaching plateau").
      arg(s->useEoc ? "eoc" : "bias").
      arg(s->isOxidation ? "oxidation" : "reduction");
  }

public:


  /// Formula:
  virtual void function(const double * params, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const {
    Storage * s = storage<Storage>(data);

    const Vector & xv = ds->x();
    double f = GSL_CONST_MKSA_FARADAY /(params[0] * GSL_CONST_MKSA_MOLAR_GAS);

    for(int i = 3; i <= (s->useEoc ? 4 : 5); i++)
      if(params[i] < 0)
        throw RangeError(QString("Negative rate constant ratio: #%1").arg(i));

    double cur = params[6];
    double bias = params[5];
    if(s->useEoc)
      bias = exp(f * (params[5] - params[1])) * 
        exp(f * (params[5] - params[2]));
    
    if(s->isOxidation)
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
      if(s->plateau)
        v = cur * (1 - ap)/a * 
          (1 + log((bias * a + b)/
                   (bias * a + b * exp(params[7])))/params[7]);
      else
        v = cur * (1 - ap)/a * log((bias * a + b)/b);
      gsl_vector_set(target, i, v);
    }
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * /*ds*/,
                            double * a) const
  {
    Storage * s = storage<Storage>(data);

    a[0] = soas().temperature();
    a[1] = -0.6; 
    a[2] = -0.3; 
    a[3] = 1; 
    a[4] = 10; 
    a[5] = (s->useEoc ? -0.66 : 2);
    a[6] = (s->isOxidation ? 1e-5 : -1e-5);
    if(s->plateau)
      a[7] = 1;
  };

  virtual QList<ParameterDefinition> parameters(FitData * data) const {
    Storage * s = storage<Storage>(data);

    QList<ParameterDefinition> defs;
    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("E1")
         << ParameterDefinition("E2")
         << ParameterDefinition("k02/k01")
         << ParameterDefinition("k2/k0");
    if(s->useEoc)
      defs << ParameterDefinition("Eoc"); // parameter 5
    else
      defs << ParameterDefinition("k2/k-2"); 
    if(s->plateau)
      defs << ParameterDefinition("ilim")
           << ParameterDefinition("betad0");
    else
      defs << ParameterDefinition("ilim/betad0");
    return defs;
  };

  virtual ArgumentList * fitHardOptions() const {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new 
                   BoolArgument("plateau", 
                                "Plateau",
                                "whether to use the general expression or "
                                "only that valid when plateaus are not reached (default: off)")
                   << new 
                   BoolArgument("oxidation", 
                                "Reference is oxidation",
                                "if on, use the oxidation current as reference (default: off)")
                   << new 
                   BoolArgument("use-eoc", 
                                "Use open circuit",
                                "whether to use explicitly the bias or compute "
                                "it using the open circuit potential (default: false)")
                   );
    return opts;
  };


  EECRFit() :
    PerDatasetFit("eecr-wave", 
                  "Fit of an EECR catalytic wave",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
EECRFit fit_eecr;


/// Fits to the EECR + relay model of electrochemical waves
class EECRRelayFit : public PerDatasetFit {
public:


  /// Formula:
  virtual void function(const double * params, FitData * , 
                        const DataSet * ds , gsl_vector * target) const {

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
                            const DataSet * /*ds*/,
                            double * a) const
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

  virtual QList<ParameterDefinition> parameters(FitData * ) const {
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
    makeCommands();
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
EECRRelayFit fit_ececr_relay;



