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

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
    biExp = false;
    updateFromOptions(opts, "bi-exp", biExp);
  }

  
  virtual QString optionsString() const {
    return (biExp ? "bi-exponential" : "mono-exponential");
  }

public:


  /// Formula:
  virtual void function(const double * a, FitData * , 
                        const DataSet * ds , gsl_vector * target) {

    const Vector & xv = ds->x();

    bool first_bit = true;
    double delta_E = xv[1] - xv[0];
    double E_vertex = 0;
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
                     (a[2] * x + a[3]) * act);

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
    if(biExp) {
      a[7] = 800;
      a[8] = 0.3;
    }
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

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
    plateau = false;
    updateFromOptions(opts, "plateau", plateau);
  }

  
  virtual QString optionsString() const {
    return (plateau ? "reaching plateau" : "not reaching plateau");
  }

public:


  /// Formula:
  virtual void function(const double * params, FitData * , 
                        const DataSet * ds , gsl_vector * target) {

    const Vector & xv = ds->x();
    double f = GSL_CONST_MKSA_FARADAY /(params[0] * GSL_CONST_MKSA_MOLAR_GAS);

    for(int i = 3; i <= 5; i++)
      if(params[i] < 0)
        throw RangeError(QString("Negative rate constant ratio: #%1").arg(i));

    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      double e1 = exp(f * (x - params[1]));
      double e2 = exp(f * (x - params[2]));
      double b = params[4] * (params[5] * 
                              (sqrt(e1) + sqrt(e2)/params[3] * (1 + e1)) +
                              sqrt(e1) * (1 + e2) + e1 * sqrt(e2)/params[3]);
      double a = 1 + e2*(1 + e1);
      double ap = e1*e2/params[5];
      
      double v;
      if(plateau)
        v = params[6] * (1 - ap)/a * 
          (1 + log((params[5] * a + b)/
                   (params[5] * a + b * exp(params[7])))/params[7]);
      else
        v = params[6] * (1 - ap)/a * log((params[5] * a + b)/b);
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
    a[5] = 2;
    a[6] = -1e-5;
    if(plateau)
      a[7] = 1;
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;
    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("E1")
         << ParameterDefinition("E2")
         << ParameterDefinition("k02/k01")
         << ParameterDefinition("k2/k0") // 4
         << ParameterDefinition("k2/k-2"); 
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
                   );
    makeCommands(NULL, NULL, NULL, opts);
  }
};


// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
EECRFit fit_eecr;
