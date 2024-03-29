/**
   @file wave-fits.cc
   Fits for the "wave shape", based on the Fourmond JACS 2013 paper
   Copyright 2013, 2014, 2015, 2016, 2023 by CNRS/AMU

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

// These are the old names
typedef enum {
  Nernst,
  SlowET,
  Dispersion,
  Full
} ShapeApproximation;


QStringList approxNames =
  QStringList()
  << "nernst"
     << "slow-et"
        << "bd0-inf"
           << "disp-k0";

QList<ShapeApproximation> approxValues =
              QList<ShapeApproximation>()
              << Nernst
                 << SlowET
                    << Dispersion
                       << Full;

/// Fits to the ECI model of electrochemical waves
class ECIFit : public PerDatasetFit {
  class Storage : public FitInternalStorage {
  public:
    /// The approximation level to use
    ShapeApproximation approx;
    
    /// Whether the current is a reduction current (default) or an
    /// oxidation current
    bool isOxidation;
  };
    
protected:
    virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const override {
    return deepCopy<Storage>(source);
  };


  virtual void processOptions(const CommandOptions & opts, FitData * data) const override 
  {
    Storage * s = storage<Storage>(data);
    s->approx = Dispersion;
    bool reduction = false;
    updateFromOptions(opts, "model", s->approx);
    updateFromOptions(opts, "reduction", reduction);
    s->isOxidation = ! reduction;
  }

  
  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString("%1, %3").
      arg(s->approx).
      arg(s->isOxidation ? "oxidation" : "reduction");
  }

public:

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    QList<ParameterDefinition> defs;
    const char * knm = (s->isOxidation ? "kox/k0or" : "kred/k0or");
    

    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("Eor");
    switch(s->approx) {
    case Nernst:
      defs << ParameterDefinition("ilim");
      break;
    case SlowET:
    case Full:
      defs << ParameterDefinition("ilim")
           << ParameterDefinition(knm);
      if(s->approx == Full)
        defs << ParameterDefinition("betad0");
          break;
    case Dispersion:
      defs << ParameterDefinition("ilim/betad0")
           << ParameterDefinition(knm);
      break;
    }
    
    return defs;
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);
    a[0] = soas().temperature();
    a[1] = 0.5 * (ds->x().max() + ds->x().min());
    switch(s->approx) {
    case Full:
      a[4] = 10;
    case Dispersion:
    case SlowET:
      a[3] = 0.2;
    case Nernst:
      a[2] = s->isOxidation ?
        ds->y().max() : ds->y().min();                   // ilim/ilim/beta d0
    };
  };


  /// Formula:
  virtual void function(const double * params, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override {
    Storage * s = storage<Storage>(data);

    const Vector & xv = ds->x();
    double f = GSL_CONST_MKSA_FARADAY /(params[0] * GSL_CONST_MKSA_MOLAR_GAS);

    double cur = params[2];

    double k2_k0 = (s->approx == Nernst ? 0 : params[3]);
    if(k2_k0 < 0)
      throw RangeError("Negative kox/k0 or kred/k0");

    double bd0 = 0;
    if(s->approx == Full) {
      bd0 = params[4];
      if(bd0 < 0)
        throw RangeError("Negative bd0");
    }
    double ebd0 = exp(bd0);

    if(s->isOxidation) {
      if(cur < 0)
        throw RangeError("Negative oxidation current");
    }
    else
      if(cur > 0)
        throw RangeError("Positive reduction current");

    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      x -= params[1];
      if(s->isOxidation)
        x = -x;
      
      double d1 = exp(0.5 * f * x);
      double e1 = d1 * d1;
      double a = 1 + e1;
      double v;
      if(s->approx == Nernst) {
        v = cur/a;
      }
      else {
        double b = d1;
        switch(s->approx) {
        case SlowET:
          v = cur/(a + b * k2_k0);
          break;
        case Dispersion:
          v = cur/a * log(1 + a/(b * k2_k0));
          break;
        case Full:
          v = cur/a * (1 + (1/bd0) *
                       log((a + b * k2_k0)/(a + b * k2_k0 * ebd0)));
          break;
        default:
          ;
        }
      }
      gsl_vector_set(target, i, v);
    }
  };


  virtual ArgumentList fitHardOptions() const override {
    return
      ArgumentList(QList<Argument *>()
                   << new 
                   TemplateChoiceArgument<ShapeApproximation>
                   (approxNames, approxValues,
                    "model",
                    "Model", 
                    "the kind of model used for the computation (default: dispersion)")
                   << new 
                   BoolArgument("reduction", 
                                "Reductive direction",
                                "if on, models a reductive wave (default: off, hence oxidative wave)")
                   );
  };

  ECIFit() :
    PerDatasetFit("eci-wave", 
                  "Fit of an EC irreversible catalytic wave",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
ECIFit fit_eci;



//////////////////////////////////////////////////////////////////////

/// Fits to the ECR model of electrochemical waves
class ECRFit : public PerDatasetFit {
  class Storage : public FitInternalStorage {
  public:
    /// The approximation level to use
    ShapeApproximation approx;
    
    /// Whether the current is a reduction current (default) or an
    /// oxidation current
    bool isOxidation;

    /// Whether the bias parameter is a open circuit potential or a
    /// ratio of rate constants.
    bool useEoc;
  };
    
protected:
    virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const override {
    return deepCopy<Storage>(source);
  };


  virtual void processOptions(const CommandOptions & opts, FitData * data) const override 
  {
    Storage * s = storage<Storage>(data);
    s->approx = Dispersion;
    s->useEoc = false;

    bool reduction = false;
    updateFromOptions(opts, "model", s->approx);
    updateFromOptions(opts, "reduction", reduction);
    updateFromOptions(opts, "use-eoc", s->useEoc);
    s->isOxidation = ! reduction;
  }

  
  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString("%1, %2, %3").
      arg(s->approx).
      arg(s->useEoc ? "eoc" : "bias").
      arg(s->isOxidation ? "oxidation" : "reduction");
  }

public:

    virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    QList<ParameterDefinition> defs;
    const char * knm1 = (s->isOxidation ? "kox/k0or" : "kred/k0or");
    const char * knm2 = (s->isOxidation ? "kred/kox" : "kox/kred");

    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("Eor");
    if(s->useEoc)
      defs << ParameterDefinition("Eoc");
    else
      defs << ParameterDefinition(knm2);
    switch(s->approx) {
    case Nernst:
      defs << ParameterDefinition("ilim");
      break;
    case SlowET:
    case Full:
      defs << ParameterDefinition("ilim")
           << ParameterDefinition(knm1);
      if(s->approx == Full)
        defs << ParameterDefinition("betad0");
          break;
    case Dispersion:
      defs << ParameterDefinition("ilim/betad0")
           << ParameterDefinition(knm1);
      break;
    }
    
    return defs;
  };


  virtual void initialGuess(FitData * data, const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);
    a[0] = soas().temperature();
    a[1] = 0.5 * (ds->x().max() + ds->x().min());

    double b = -ds->y().max()/ds->y().min();
    if(b < 0.05)
      b = 0.05;
    if(b > 20)
      b = 20;

    if(s->useEoc) {
      a[2] = - (a[0] * GSL_CONST_MKSA_MOLAR_GAS)/GSL_CONST_MKSA_FARADAY *
        log(b) + a[1];
    }
    else
      a[2] = b;
      
    switch(s->approx) {
    case Full:
      a[5] = 10;
    case Dispersion:
    case SlowET:
      a[4] = 0.2;
    case Nernst:
      a[3] = s->isOxidation ?
        ds->y().max() : ds->y().min();                   // ilim/ilim/beta d0
    };
  };


  /// Formula:
  virtual void function(const double * params, FitData * data, 
                        const DataSet * ds,
                        gsl_vector * target) const override {
    Storage * s = storage<Storage>(data);

    const Vector & xv = ds->x();
    double f = GSL_CONST_MKSA_FARADAY /(params[0] * GSL_CONST_MKSA_MOLAR_GAS);

    const double E1 = params[1];
    double k_m2_k2 = params[2];

    if(s->useEoc)
      k_m2_k2 = exp((s->isOxidation ? -1 : 1)*f * (E1 - k_m2_k2));

    if(k_m2_k2 < 0)
      throw RangeError("Negative bias");

    
    double cur = params[3];
        

    double k2_k0 = (s->approx == Nernst ? 0 : params[4]);
    if(k2_k0 < 0)
      throw RangeError("Negative k2/k0");

    double bd0 = 0;
    if(s->approx == Full) {
      bd0 = params[5];
      if(bd0 < 0)
        throw RangeError("Negative bd0");
    }
    double ebd0 = exp(bd0);

    if(s->isOxidation) {
      if(cur < 0)
        throw RangeError("Negative oxidation current");
    }
    else
      if(cur > 0)
        throw RangeError("Positive reduction current");

    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      x -= E1;

      if(s->isOxidation)
        x = -x;

      double d1 = exp(0.5 * f * x);
      double e1 = d1 * d1;
      double a = 1 + e1;
      double ap = k_m2_k2 * e1;
      double v;
      if(s->approx == Nernst) {
        v = cur * (1 - ap)/a;
      }
      else {
        double b = d1 * (1 + k_m2_k2);
        switch(s->approx) {
        case SlowET:
          v = cur * (1 - ap)/(a + b * k2_k0);
          break;
        case Dispersion:
          v = cur * (1 - ap)/a * log(1 + a/(b * k2_k0));
          break;
        case Full:
          v = cur * (1 - ap)/a * (1 + (1/bd0) *
                       log((a + b * k2_k0)/(a + b * k2_k0 * ebd0)));
          break;
        default:
          ;
        }
      }
      gsl_vector_set(target, i, v);
    }
  };


  virtual ArgumentList fitHardOptions() const  override {
    return
      ArgumentList(QList<Argument *>()
                   << new 
                   TemplateChoiceArgument<ShapeApproximation>
                   (approxNames, approxValues,
                    "model",
                    "Model", 
                    "the kind of model used for the computation (default: dispersion)")
                   << new 
                   BoolArgument("reduction", 
                                "Reference is reduction",
                                "if on, use the reductive direction as reference (default: oxidative direction as reference)")
                   << new 
                   BoolArgument("use-eoc", 
                                "Use open circuit",
                                "whether to use explicitly the bias or compute "
                                "it using the open circuit potential (default: false)")
                   );
  };

  ECRFit() :
    PerDatasetFit("ecr-wave", 
                  "Fit of an EC reversible catalytic wave",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
ECRFit fit_ecr;

//////////////////////////////////////////////////////////////////////

/// Fits to the EECI model of electrochemical waves
class EECIFit : public PerDatasetFit {
  class Storage : public FitInternalStorage {
  public:
    /// The approximation level to use
    ShapeApproximation approx;
    
    /// Whether the current is a reduction current (default) or an
    /// oxidation current
    bool isOxidation;
  };
    
protected:
    virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const override {
    return deepCopy<Storage>(source);
  };


  virtual void processOptions(const CommandOptions & opts, FitData * data) const override 
  {
    Storage * s = storage<Storage>(data);
    s->approx = Dispersion;

    bool reduction = false;
    updateFromOptions(opts, "model", s->approx);
    updateFromOptions(opts, "reduction", reduction);
    updateFromOptions(opts, "model", s->approx);
    updateFromOptions(opts, "oxidation", s->isOxidation);
    s->isOxidation = ! reduction;
  }

  
  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString("%1, %3").
      arg(s->approx).
      arg(s->isOxidation ? "oxidation" : "reduction");
  }

public:

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    QList<ParameterDefinition> defs;

    const char * knm1 = (s->isOxidation ? "kox/k0or" : "kred/k0or");

    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("Eoi")
         << ParameterDefinition("Eir");
    switch(s->approx) {
    case Nernst:
      defs << ParameterDefinition("ilim");
      break;
    case SlowET:
    case Full:
      defs << ParameterDefinition("ilim")
           << ParameterDefinition(knm1)
           << ParameterDefinition("k0oi/k0ir");
      if(s->approx == Full)
        defs << ParameterDefinition("betad0");
          break;
    case Dispersion:
      defs << ParameterDefinition("ilim/betad0")
           << ParameterDefinition(knm1)
           << ParameterDefinition("k0oi/k0ir");
      break;
    }
    
    return defs;
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);
    a[0] = soas().temperature();
    a[1] = 0.6*ds->x().max() + 0.4*ds->x().min();
    a[2] = 0.4*ds->x().max() + 0.6*ds->x().min();
    switch(s->approx) {
    case Full:
      a[6] = 10;
    case Dispersion:
    case SlowET:
      a[4] = 0.2;
      a[5] = 1;
    case Nernst:
      a[3] = s->isOxidation ?
        ds->y().max() : ds->y().min();                   // ilim/ilim/beta d0
    };
  };


  /// Formula:
  virtual void function(const double * params, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override {
    Storage * s = storage<Storage>(data);

    const Vector & xv = ds->x();
    double f = GSL_CONST_MKSA_FARADAY /(params[0] * GSL_CONST_MKSA_MOLAR_GAS);


    const double E1 = params[1];
    const double E2 = params[2];

    double cur = params[3];

    const double k2_k0 = (s->approx == Nernst ? 0 : params[4]);
    if(k2_k0 < 0)
      throw RangeError("Negative k2/k0");

    double k0oi_k0ir_hlf;
    {
      const double k01_k02 = (s->approx == Nernst ? 0 : params[5]);
      if(k01_k02 < 0)
        throw RangeError("Negative k0oi/k0ir");
      
      k0oi_k0ir_hlf = sqrt(k01_k02);
    }

    double bd0 = 0;
    if(s->approx == Full) {
      bd0 = params[6];
      if(bd0 < 0)
        throw RangeError("Negative bd0");
    }
    double ebd0 = exp(bd0);

    if(s->isOxidation) {
      if(cur < 0)
        throw RangeError("Negative oxidation current");
    }
    else
      if(cur > 0)
        throw RangeError("Positive reduction current");

    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      x -= E1;                  // = E_OI
      if(s->isOxidation)
        x = -x;
      double d1 = exp(0.5 * f * x);
      double e1 = d1 * d1;

      x = xv[i];
      x -= E2;                  // = E_IR
      if(s->isOxidation)
        x = -x;
      double d2 = exp(0.5 * f * x);
      double e2 = d2 * d2;

      // For the case of oxidation, e and d are already inverted
      
      double a = s->isOxidation ?
        1 + e1*(1 + e2)
        : 1 + e2*(1 + e1)
        ;
      double v;
      if(s->approx == Nernst) {
        v = cur/a;
      }
      else {
        double b = s->isOxidation ?
          d1/k0oi_k0ir_hlf * (1 + e2) + k0oi_k0ir_hlf * d2 
          : 
          d1/k0oi_k0ir_hlf + k0oi_k0ir_hlf * d2 * (1 + e1);
        switch(s->approx) {
        case SlowET:
          v = cur/(a + b * k2_k0);
          break;
        case Dispersion:
          v = cur/a * log(1 + a/(b * k2_k0));
          break;
        case Full:
          v = cur/a * (1 + (1/bd0) *
                       log((a + b * k2_k0)/(a + b * k2_k0 * ebd0)));
          break;
        default:
          ;
        }
      }
      gsl_vector_set(target, i, v);
    }
  };

  virtual ArgumentList fitHardOptions() const override {
    return
      ArgumentList(QList<Argument *>()
                   << new 
                   TemplateChoiceArgument<ShapeApproximation>
                   (approxNames, approxValues,
                    "model",
                    "Model", 
                    "the kind of model used for the computation (default: dispersion)")
                   << new 
                   BoolArgument("reduction", 
                                "Reductive direction",
                                "if on, models a reductive wave (default: off, hence oxidative wave)")
                   );
  };

  EECIFit() :
    PerDatasetFit("eeci-wave", 
                  "Fit of an EC irreversible catalytic wave",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
EECIFit fit_eeci;

//////////////////////////////////////////////////////////////////////

/// Fits to the EECR model of electrochemical waves
class EECRFit : public PerDatasetFit {
  class Storage : public FitInternalStorage {
  public:
    /// The approximation level to use
    ShapeApproximation approx;
    
    /// Whether the current is a reduction current (default) or an
    /// oxidation current
    bool isOxidation;

    /// Whether the bias parameter is a open circuit potential or a
    /// ratio of rate constants.
    bool useEoc;
  };
    
protected:
    virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const override {
    return deepCopy<Storage>(source);
  };


  virtual void processOptions(const CommandOptions & opts, FitData * data) const override 
  {
    Storage * s = storage<Storage>(data);
    s->approx = Dispersion;
    s->useEoc = false;

    bool reduction = false;
    updateFromOptions(opts, "model", s->approx);
    updateFromOptions(opts, "reduction", reduction);
    updateFromOptions(opts, "use-eoc", s->useEoc);
    s->isOxidation = ! reduction;
  }

  
  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString("%1, %2, %3").
      arg(s->approx).
      arg(s->useEoc ? "eoc" : "bias").
      arg(s->isOxidation ? "oxidation" : "reduction");
  }

public:

    virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    QList<ParameterDefinition> defs;

    const char * knm1 = (s->isOxidation ? "kox/k0or" : "kred/k0or");
    const char * knm2 = (s->isOxidation ? "kred/kox" : "kox/kred");

    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("Eoi")
         << ParameterDefinition("Eir");
    if(s->useEoc)
      defs << ParameterDefinition("Eoc");
    else
      defs << ParameterDefinition(knm2);
        
    switch(s->approx) {
    case Nernst:
      defs << ParameterDefinition("ilim");
      break;
    case SlowET:
    case Full:
      defs << ParameterDefinition("ilim")
           << ParameterDefinition(knm1)
           << ParameterDefinition("k0oi/k0ir");
      if(s->approx == Full)
        defs << ParameterDefinition("betad0");
          break;
    case Dispersion:
      defs << ParameterDefinition("ilim/betad0")
           << ParameterDefinition(knm1)
           << ParameterDefinition("k0oi/k0ir");

      break;
    }
    
    return defs;
  };


  virtual void initialGuess(FitData * data, const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);
    a[0] = soas().temperature();

    a[1] = 0.6*ds->x().max() + 0.4*ds->x().min();
    a[2] = 0.4*ds->x().max() + 0.6*ds->x().min();

    double b = -ds->y().min()/ds->y().max();
    if(b < 0.05)
      b = 0.05;
    if(b > 20)
      b = 20;

    if(s->useEoc) {
      a[3] = 0.5*(a[0] * GSL_CONST_MKSA_MOLAR_GAS)/GSL_CONST_MKSA_FARADAY *
        log(b) + 0.5*(a[1]+a[2]);
    }
    else
      a[3] = b;
      
    switch(s->approx) {
    case Full:
      a[7] = 10;
    case Dispersion:
    case SlowET:
      a[5] = 0.2;
      a[6] = 1;
    case Nernst:
      a[4] = s->isOxidation ?
        ds->y().max() : ds->y().min();                   // ilim/ilim/beta d0
    };
  };


  /// Formula:
  virtual void function(const double * params, FitData * data, 
                        const DataSet * ds,
                        gsl_vector * target) const override {
    Storage * s = storage<Storage>(data);

    const Vector & xv = ds->x();
    double f = GSL_CONST_MKSA_FARADAY /(params[0] * GSL_CONST_MKSA_MOLAR_GAS);

    const double E1 = params[1];
    const double E2 = params[2];
    double k_bias = params[3];

    if(s->useEoc)
      k_bias = exp((s->isOxidation ? -2 : 2)*f * (0.5*(E1+E2) - k_bias));
    

    if(k_bias < 0)
      throw RangeError("Negative bias");

    
    double cur = params[4];
        

    double k2_k0 = (s->approx == Nernst ? 0 : params[5]);
    if(k2_k0 < 0)
      throw RangeError("Negative k2/k01");

    double k0oi_k0ir_hlf;
    {
      const double k01_k02 = (s->approx == Nernst ? 0 : params[6]);
      if(k01_k02 < 0)
        throw RangeError("Negative k0oi/k0ir");
      
      k0oi_k0ir_hlf = sqrt(k01_k02);
    }


    double bd0 = 0;
    if(s->approx == Full) {
      bd0 = params[7];
      if(bd0 < 0)
        throw RangeError("Negative bd0");
    }
    double ebd0 = exp(bd0);

    if(s->isOxidation) {
      if(cur < 0)
        throw RangeError("Negative oxidation current");
    }
    else
      if(cur > 0)
        throw RangeError("Positive reduction current");

    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      x -= E1;                  // = E_OI
      if(s->isOxidation)
        x = -x;
      double d1 = exp(0.5 * f * x);
      double e1 = d1 * d1;

      x = xv[i];
      x -= E2;                  // = E_IR
      if(s->isOxidation)
        x = -x;
      double d2 = exp(0.5 * f * x);
      double e2 = d2 * d2;

      // For the case of oxidation, e and d are already inverted


      double a = s->isOxidation ?
        1 + e1*(1 + e2)
        : 1 + e2*(1 + e1)
        ;
      double ap = k_bias * e1 * e2;
      double v;
      if(s->approx == Nernst) {
        v = cur * (1 - ap)/a;
      }
      else {
        double b = s->isOxidation ?
          d1/k0oi_k0ir_hlf * (1 + (1 + k_bias) * e2) +
          k0oi_k0ir_hlf * d2 * (1 + k_bias*(1+e1))
          :
          d1/k0oi_k0ir_hlf * (1 + k_bias*(1+e2)) +
          k0oi_k0ir_hlf * d2 * (1 + (1 + k_bias) * e1);
          ;
        switch(s->approx) {
        case SlowET:
          v = cur * (1 - ap)/(a + b * k2_k0);
          break;
        case Dispersion:
          v = cur * (1 - ap)/a * log(1 + a/(b * k2_k0));
          break;
        case Full:
          v = cur * (1 - ap)/a * (1 + (1/bd0) *
                       log((a + b * k2_k0)/(a + b * k2_k0 * ebd0)));
          break;
        default:
          ;
        }
      }
      gsl_vector_set(target, i, v);
    }
  };


  virtual ArgumentList fitHardOptions() const override {
    return
      ArgumentList(QList<Argument *>()
                   << new 
                   TemplateChoiceArgument<ShapeApproximation>
                   (approxNames, approxValues,
                    "model",
                    "Model", 
                    "the kind of model used for the computation (default: dispersion)")
                   << new 
                   BoolArgument("reduction", 
                                "Reference is reduction",
                                "if on, use the reductive direction as reference (default: oxidative direction as reference)")
                   << new 
                   BoolArgument("use-eoc", 
                                "Use open circuit",
                                "whether to use explicitly the bias or compute "
                                "it using the open circuit potential (default: false)")
                   );
  };

  EECRFit() :
    PerDatasetFit("eecr-wave", 
                  "Fit of an EEC reversible catalytic wave",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
EECRFit fit_eecr;



//////////////////////////////////////////////////////////////////////


/// Fits to the EECR + relay model of electrochemical waves
class EECRRelayFit : public PerDatasetFit {

  class Storage : public FitInternalStorage {
  public:
    /// Whether the equilibrium constants (false) or the potentials
    /// (true) are used
    bool usePotentials;

    /// Whether the bias parameter is a open circuit potential or a
    /// ratio of rate constants.
    bool useEoc;
  };




protected:
  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const override {
    return deepCopy<Storage>(source);
  };

  virtual void processOptions(const CommandOptions & opts, FitData * data) const override 
  {
    Storage * s = storage<Storage>(data);

    s->usePotentials = false;
    updateFromOptions(opts, "use-potentials", s->usePotentials);

    s->useEoc = false;
    updateFromOptions(opts, "use-eoc", s->useEoc);
  }

  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString("%1").
      arg(s->usePotentials ? "potentials" : "equilibrium constants");
  }

  
  
public:

  virtual ArgumentList fitHardOptions() const override {
    return
      ArgumentList(QList<Argument *>()
                   << new 
                   BoolArgument("use-potentials", 
                                "Use potentials",
                                "if on, use the potentials of the active site electronic transitions rather than the equilibrium constants")
                   << new 
                   BoolArgument("use-eoc", 
                                "Use open circuit",
                                "whether to use explicitly the bias or compute "
                                "it using the open circuit potential (default: false)"));
  };

  

  /// Formula:
  virtual void function(const double * p, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override {

    Storage * s = storage<Storage>(data);

    double params[10];
    for(int i = 0; i < sizeof(params)/sizeof(*params); i++)
      params[i] = p[i];


    const Vector & xv = ds->x();
    const double f = GSL_CONST_MKSA_FARADAY 
      /(params[0] * GSL_CONST_MKSA_MOLAR_GAS);

    // Now convert params if applicable
    if(s->usePotentials) {
      // params 5 is either k1/km1 or Eoi
      // RO <=>[k_m1][k_1] OI
      // -> k_m1/k_1 is RO/OI = exp(E_oi - E_r)
      // so the formula below appears to be correct
      params[5] = exp(f * (params[1] - params[5]));
      params[7] = exp(f * (params[1] - params[7]));
    }

    if(s->useEoc) {
      // From maple, the EOC is:
      // m3_ocp_n_eq_2 := simplify(subs(m3_ocp_ds, d^4));
      //                              k_2 kp_m1 k_m1
      //                              --------------
      //                              kp_1 k_1 k_m2
      // d^4 = exp(2*f*(E_oc - E_r)
      //
      // So that k_m2/k_2 = kp_m1/kp_1 * k_m1/k_1 /d^4
      QTextStream o(stdout);
      o << "Changing from : " << params[3] << endl;
      params[3] = exp(2*f*(params[1] - params[3]))/(params[5] * params[7]);
      o << " -> to " << params[3] << endl;
    }


    for(int i = 2; i <= 7; i++)
      if(params[i] < 0)
        throw RangeError(QString("Negative rate constant ratio: #%1").arg(i));

    if(params[8] > 0)
      throw RangeError("Positive reduction current");

    if(params[9] < 0)
      throw RangeError("Negative beta d0");

    
    // We rename the parameters...
    //
    // Let's keep in mind that k2 is one.
    const double k_0M    = params[2];
    const double k_2     = 1;
    const double k_m2    = params[3];
    const double k_1     = params[4];           // rate of IO to OR
    const double k_m1    = params[4]/params[5]; // rate of OR to IO
    const double kp_1    = params[6];           // rate of RO to IR
    const double kp_m1   = params[6]/params[7]; // rate of IR to RO
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

    const double pref = ilim * B_0/(A_0 * beta_d0);

    // QTextStream o(stdout);
    


    for(int i = 0; i < xv.size(); i++) {
      const double x = xv[i];
      const double d = exp(0.5 * f * (x - params[1]));
      const double d_2 = d*d;
      const double d_3 = d_2 * d;
      const double d_4 = d_2 * d_2;
      const double d_5 = d_4 * d;
      
      const double A = A_0 + A_4 * d_4;
      const double B = B_0 + B_2 * d_2 + B_4 * d_4;
      const double C = C_1 * d + C_3 * d_3 + C_5 * d_5;
      const double D = D_2 * d_2 + D_4 * d_4;
      const double E = E_0 + E_2 * d_2;
      const double F = F_1 * d;


      const double Gamma2_M = 2 * B * E * k_0M + B*F + C;
      const double Gamma2_m = 2 * B * E * k_0m + B*F + C;

      const double Gamma1_M = k_0M * (B * E * k_0M + B*F + C) + D;
      const double Gamma1_m = k_0m * (B * E * k_0m + B*F + C) + D;

      const double alpha = 4 * D * B * E - pow(B*F + C, 2);
      const double o_ov_salpha = 1/sqrt(fabs(alpha));


      const double rest = log(Gamma1_M/Gamma1_m);


      double delta;
      if(alpha > 0)
        delta = atan(Gamma2_M * o_ov_salpha) -
          atan(Gamma2_m * o_ov_salpha);
      else
        delta = -0.5 * log((1 + Gamma2_M * o_ov_salpha) *
                           (1 - Gamma2_m * o_ov_salpha) /
                           (1 - Gamma2_M * o_ov_salpha) /
                           (1 + Gamma2_m * o_ov_salpha));

      double fct = (B*F - C) * o_ov_salpha * 2 * delta;
      const double par =  rest + fct;
      gsl_vector_set(target, i, pref * A/(2 * B) * par);
    }
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);
    double xmin = ds->x().min();
    double xmax = ds->x().max();
    a[0] = soas().temperature();
    a[1] = 0.5 * (xmin + xmax) ; 
    a[2] = 1; 
    a[3] = s->useEoc ? (0.6 * xmin + 0.4 * xmax) : 1.5; 
    a[4] = 10;
    a[5] = s->usePotentials ?  (0.7 * xmin + 0.3 * xmax) : 2;
    a[6] = 2;
    a[7] = s->usePotentials ?  (0.3 * xmin + 0.7 * xmax) : 1;
    a[8] = -1e-5;
    a[9] = 1;
  };

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    QList<ParameterDefinition> defs;
    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("Er")
         << ParameterDefinition("k0r/k2")
         << ParameterDefinition(s->useEoc ? "Eoc" : "km2/k2") // params[3]
         << ParameterDefinition("k1/k2")
         << ParameterDefinition(s->usePotentials ? "Eoi" : "k1/km1") 
         << ParameterDefinition("kp1/k2") // params[6]
         << ParameterDefinition(s->usePotentials ? "Eir" : "kp1/kpm1") 
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

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// Old fits

/// Fits to the ECR model of electrochemical waves
class ECROldFit : public PerDatasetFit {
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
    virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const override {
    return deepCopy<Storage>(source);
  };


  virtual void processOptions(const CommandOptions & opts, FitData * data) const override 
  {
    Storage * s = storage<Storage>(data);
    s->plateau = false;
    s->useEoc = false;
    s->isOxidation = false;
    updateFromOptions(opts, "plateau", s->plateau);
    updateFromOptions(opts, "use-eoc", s->useEoc);
    updateFromOptions(opts, "oxidation", s->isOxidation);
  }

  
  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString("%1, %2, %3").
      arg(s->plateau ? "reaching plateau" : "not reaching plateau").
      arg(s->useEoc ? "eoc" : "bias").
      arg(s->isOxidation ? "oxidation" : "reduction");
  }

public:


  /// Formula:
  virtual void function(const double * params, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override {
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
                            double * a) const override
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

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
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

  virtual ArgumentList fitHardOptions() const override {
    return
      ArgumentList(QList<Argument *>()
                   << new 
                   TemplateChoiceArgument<ShapeApproximation>
                   (approxNames, approxValues,
                    "model",
                    "Model", 
                    "the kind of model used for the computation (default: dispersion)")
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
  };

  ECROldFit() :
    PerDatasetFit("ecro-wave", 
                  "(old) fit of an ECR catalytic wave",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
ECROldFit fit_ecro;
//////////////////////////////////////////////////////////////////////


/// Fits to the EECR model of electrochemical waves
class EECROFit : public PerDatasetFit {

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

  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const override {
    return deepCopy<Storage>(source);
  };


  virtual void processOptions(const CommandOptions & opts, FitData *data) const override 
  {
    Storage * s = storage<Storage>(data);
    s->plateau = false;
    s->useEoc = false;
    s->isOxidation = false;
    updateFromOptions(opts, "plateau", s->plateau);
    updateFromOptions(opts, "use-eoc", s->useEoc);
    updateFromOptions(opts, "oxidation", s->isOxidation);
  }

  
  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString("%1, %2, %3").
      arg(s->plateau ? "reaching plateau" : "not reaching plateau").
      arg(s->useEoc ? "eoc" : "bias").
      arg(s->isOxidation ? "oxidation" : "reduction");
  }

public:


  /// Formula:
  virtual void function(const double * params, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override {
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
                            double * a) const override
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

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
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

  virtual ArgumentList fitHardOptions() const override {
    return
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
  };


  EECROFit() :
    PerDatasetFit("eecro-wave", 
                  "Fit of an EECRO catalytic wave",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
EECROFit fit_eecro;


