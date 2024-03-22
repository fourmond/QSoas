/**
   @file misc-fits.cc
   Various fits...
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014, 2015, 2016 by CNRS/AMU

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
#include <linearkineticsystem.hh>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_poly.h>


class SlowScanLowPotFit : public PerDatasetFit {
  class Storage : public FitInternalStorage {
  public:
    bool explicitRate;
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

    s->explicitRate = false;
    updateFromOptions(opts, "explicit-rate", s->explicitRate);
  }

  
  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return s->explicitRate ? "explicit rate" : "implicit rate";
  }

public:


  /// Formula:
  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override {
    Storage * s = storage<Storage>(data);

    double fara = GSL_CONST_MKSA_FARADAY /(a[0] * GSL_CONST_MKSA_MOLAR_GAS);
    double alpha = (s->explicitRate ? a[6] : a[5]);

    double D = (s->explicitRate ? pow(10, a[5])/(alpha * fara * a[4])
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

  virtual void initialGuess(FitData * data, 
                            const DataSet * ,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);

    a[0] = soas().temperature();
    a[1] = 1; a[2] = 1; 
    if(s->explicitRate) {
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

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);

    QList<ParameterDefinition> params;
    params << ParameterDefinition("temperature", true)
           << ParameterDefinition("a")
           << ParameterDefinition("b")
           << ParameterDefinition("c");
    if(s->explicitRate)
      params << ParameterDefinition("nu", true)
             << ParameterDefinition("log(k)");
    else
      params << ParameterDefinition("log(D)");
        

    params << ParameterDefinition("alpha");

    return params;
  };

  virtual ArgumentList fitHardOptions() const override {
    return
      ArgumentList(QList<Argument *>()
                   << new 
                   BoolArgument("explicit-rate", 
                                "Explicit rate",
                                "whether the scan rate is an explicit "
                                "parameter of the fit (default false)")
                   );
  };

  SlowScanLowPotFit() : PerDatasetFit("slow-scan-lp", 
                                      "Slow scan test",
                                      "Slow scan", 1, -1, false)  { 
    makeCommands();

  };
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
SlowScanLowPotFit fit_slow_scan_low_pot;


class SlowScanHighPotFit : public PerDatasetFit {

  class Storage : public FitInternalStorage {
  public:
    /// Whether we have a bi-exponential decay or not.
    bool biExp;

    /// Whether or not we add an additional constant factor
    bool scaling;
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

    s->biExp = false;
    updateFromOptions(opts, "bi-exp", s->biExp);
    s->scaling = false;
    updateFromOptions(opts, "scaling", s->scaling);
  }

  
  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString(s->biExp ? "bi-exponential" : "mono-exponential") + 
      QString(s->scaling ? " with " : " without ") + "scaling";
  }

public:


  /// Formula:
  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override {
    Storage * s = storage<Storage>(data);

    const Vector & xv = ds->x();

    bool first_bit = true;
    double delta_E = xv[1] - xv[0];
    double E_vertex = 0;
    double scale = 1;
    if(a[6] < 0)
      throw RangeError("Negative time constant: %1").arg(a[6]);
    if(a[4] < 0)
      throw RangeError("Negative final activity: %1").arg(a[4]);
    if(s->scaling)
      scale = a[s->biExp ? 9 : 7];
    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      double t;
      if(first_bit)
        t = (x - a[0])/a[1];
      else
        t = (2*E_vertex - a[0] - x)/a[1];
      
      double dec = (s->biExp ? 
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

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);

    a[0] = ds->x()[0];
    a[1] = 5e-4; 
    a[2] = 1e-6; 
    a[3] = 1e-6; 
    a[4] = 0.1; 
    a[5] = 1;
    a[6] = 300;
    int base = 7;
    if(s->biExp) {
      a[7] = 800;
      a[8] = 0.3;
      base = 9;
    }
    if(s->scaling)
      a[base] = 1;
  };

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    QList<ParameterDefinition> ret;
    ret << ParameterDefinition("E1", true) // Potentiel initial
        << ParameterDefinition("nu", true) // vitesse de balayage
        << ParameterDefinition("a") //2
        << ParameterDefinition("b") //3
        << ParameterDefinition("alpha_e") //4
        << ParameterDefinition("alpha_0") //5
        << ParameterDefinition("tau");
    
    if(s->biExp)
      ret << ParameterDefinition("tau_slow")
          << ParameterDefinition("frac_slow");
    // Fraction of the slow phase with respect to the total
    // inactivation.
    if(s->scaling)
      ret << ParameterDefinition("fact"); // prefactor
    
    return ret;
  };

  virtual ArgumentList fitHardOptions() const override {
    return
      ArgumentList(QList<Argument *>()
                   << new 
                   BoolArgument("bi-exp", 
                                "Bi exp",
                                "whether the relaxation is bi-exponential or "
                                "mono-exponential (false by default)")
                   << new 
                   BoolArgument("scaling", 
                                "Scaling",
                                "if on, use an additional scaling factor "
                                "(off by default)")
                   );
  };

  SlowScanHighPotFit() : 
    PerDatasetFit("slow-scan-hp", 
                  "Slow scan test",
                  "Slow scan", 1, -1, false) { 
    makeCommands();
  };
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
SlowScanHighPotFit fit_slow_scan_high_pot;

//////////////////////////////////////////////////////////////////////

/// Fits a sum of polynomials with prefactors (with various
/// constaints). The polynomial 0 is given by x_0.
class PolynomialFit : public PerDatasetFit {
  class Storage : public FitInternalStorage {
  public:
    /// Whether or not the function should be monotonic
    bool monotonic;

    /// Whether or not the first derivative should be monotonic
    bool firstMonotonic;

    /// Whether there are prefactors. On by default for multiple
    /// polynomials.
    bool prefactor;

    /// The orders of the polynomials
    QList<int> orders;
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

    int nb = -1;
    s->orders.clear();

    updateFromOptions(opts, "order", s->orders);
    updateFromOptions(opts, "number", nb);
    if(s->orders.size() == 0)
      s->orders << 10;
    if(s->orders.size() == 1 && nb > 0) {
      for(int i = 1; i < nb; i++)
        s->orders << s->orders.first();
    }
    s->prefactor = s->orders.size() > 1;
    updateFromOptions(opts, "prefactor", s->prefactor);
       
    processSoftOptions(opts, data);
  }

  
  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);

    QStringList lst;
    for(int i = 0; i < s->orders.size(); i++)
      lst << QString::number(s->orders[i]);

    return QString("orders: %1").
      arg(lst.join("," ));
  }

public:

  virtual void processSoftOptions(const CommandOptions & opts, FitData * data) const override
  {
    Storage * s = storage<Storage>(data);

    s->monotonic = false;
    s->firstMonotonic = false;

    updateFromOptions(opts, "monotonic", s->monotonic);
    updateFromOptions(opts, "without-inflexions", s->firstMonotonic);
  }

  CommandOptions currentSoftOptions(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    CommandOptions opts;
    updateOptions(opts, "monotonic", s->monotonic);
    updateOptions(opts, "without-inflexions", s->firstMonotonic);
    return opts;
  }

  /// Formula:
  virtual void function(const double * params, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override {
    Storage * s = storage<Storage>(data);

    const Vector & xv = ds->x();

    double values[3];

    const double & x0 = params[0];


    int idx = 1;
    for(int j = 0; j < s->orders.size(); j++) {
      if(s->prefactor)
        idx++;
      int od = s->orders[j];
      double d1 = 0;
      double d2 = 0;
      for(int i = 0; i < xv.size(); i++) {
        double x = xv[i] - x0;
        double tg = (j == 0 ? 0 : gsl_vector_get(target, i));
        gsl_poly_eval_derivs(params + idx, od+1, x, values, 3);
        
        if(s->prefactor)
          values[0] *= params[idx-1];
        tg += values[0];
        if(s->monotonic) {
          if(d1 != 0) {
            if(d1 * values[1] < 0)
              throw RangeError("Non-monotonic");
          }
          else {
            d1 = values[1];
          }
        }

        if(s->firstMonotonic) {
          if(d2 != 0) {
            if(d2 * values[2] < 0)
              throw RangeError("Inflexion points");
          }
          else {
            d2 = values[2];
          }
        }
        gsl_vector_set(target, i, tg);
      }
      idx += od+1;
    }
  }

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);

    a[0] = ds->x().min();
    double ymin = ds->y().min();
    double ymax = ds->y().max();
    double dx = ds->x().max() - a[0];

    a++;
    for(int j = 0; j < s->orders.size(); j++) {
      if(s->prefactor) {
        *a = 1;
        a++;
      }
      a[0] = ymin;
      for(int i = 0; i < s->orders[j]; i++) {
        a[1 + i] = (ymax - ymin)/pow(dx, i);
      }
      a+= s->orders[j] + 1;
    }
  };

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);

    QList<ParameterDefinition> defs;
    defs << ParameterDefinition("x_0", true);
    for(int j = 0; j < s->orders.size(); j++) {
      QChar base = 'A' + j;
      if(s->prefactor)
        defs << ParameterDefinition(QString("%1_p").arg(base), true);
      for(int i = 0; i <= s->orders[j]; i++)
        defs << ParameterDefinition(QString("%1_%2").arg(base).arg(i));
    }
    return defs;
  };

  virtual ArgumentList fitHardOptions() const override {
    return
      ArgumentList(QList<Argument *>()
                   << new 
                   SeveralIntegersArgument("order", 
                                           "Order",
                                           "Order of the polynomial functions")
                   << new 
                   IntegerArgument("number", 
                                   "Number",
                                   "Number of distinct polynomial functions")
                   << new 
                   BoolArgument("prefactor", 
                                "Prefactor",
                                "Whether there is a prefactor for each polynomial (on by default for multiple polynomials)")
                   );
  };

  virtual ArgumentList fitSoftOptions() const override {
    return
      ArgumentList(QList<Argument *>()
                   << new 
                   BoolArgument("monotonic", 
                                "Monotonic",
                                "With this on, only monotonic polynomials "
                                "are solutions")
                   << new 
                   BoolArgument("without-inflexions", 
                                "Without inflexion points",
                                "If this is on, there are no inflexion "
                                "points in the polynomials")
                   );
  };

  PolynomialFit() :
    PerDatasetFit("polynomial", 
                  "Fit to a polynomial function",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  }
};
  

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
PolynomialFit fit_polynomial;

//////////////////////////////////////////////////////////////////////

/// Fits to two polynomials connected by a linear segment, of class C1
class TwoPolynomialFit : public PerDatasetFit {

  class Storage : public FitInternalStorage {
  public:
    /// The order of the polynomials
    int order;
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
    s->order = 5;
    updateFromOptions(opts, "order", s->order);
  }

  
  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString("order %1").
      arg(s->order);
  }

public:

  /// Formula:
  virtual void function(const double * params, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override {
    Storage * s = storage<Storage>(data);

    const Vector & xv = ds->x();
    const double & xl = params[0];
    const double & xr = params[1];

    const double & a = params[2];
    const double & b = params[3];

    
    double lbase[s->order+1];
    double rbase[s->order+1];

    // Prepare coefficients:
    lbase[0] = xl * a + b;
    lbase[1] = a;

    rbase[0] = xr * a + b;
    rbase[1] = a;

    for(int i = 0; i < s->order - 1; i++) {
      lbase[i+2] = params[4+i];
      rbase[i+2] = params[4+i + s->order - 1];
    }

    if(xl > xr)
      throw RangeError("xl must be smaller than xr");


    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      double v;
      if(x < xl) {
        double dx = x - xl;
        v = gsl_poly_eval(lbase, s->order + 1, dx);
      }
      else if(x > xr) {
        double dx = x - xr;
        v = gsl_poly_eval(rbase, s->order + 1, dx);
      }
      else {
        v = a*x + b;
      }
      gsl_vector_set(target, i, v);
    }
  }

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);

    double xmin = ds->x().min();
    double xmax = ds->x().max();

    a[0] = 0.7 * xmin + 0.3*xmax;
    a[1] = 0.7 * xmax + 0.3*xmin;

    QPair<double, double> rl = ds->reglin(a[0], a[1]);
    a[2] = rl.first;
    a[3] = rl.second;


    double ymin = ds->y().min();
    double ymax = ds->y().max();

    double ylex = ymin - (a[2] *xmin + a[3]);
    double yrex = ymax - (a[2] *xmax + a[3]);
    double dxl = xmin - a[0];
    double dxr = xmax - a[1];
    for(int i = 0; i < s->order - 1; i++) {
      a[4 + i] = ylex/(s->order * pow(dxl, i));
      a[4 + i + s->order - 1] = yrex/(s->order * pow(dxr, i));
    }
  };

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);

    QList<ParameterDefinition> defs;
    defs << ParameterDefinition("x_l");
    defs << ParameterDefinition("x_r");
    defs << ParameterDefinition("a");
    defs << ParameterDefinition("b");

    for(int i = 2; i <= s->order; i++)
      defs << ParameterDefinition(QString("Al_%1").arg(i));
    for(int i = 2; i <= s->order; i++)
      defs << ParameterDefinition(QString("Ar_%1").arg(i));
    return defs;
  };

  virtual ArgumentList fitHardOptions() const override {
    return
      ArgumentList(QList<Argument *>()
                   << new 
                   IntegerArgument("order", 
                                   "Order",
                                   "Order of the polynomial function")
                   );
  };

  TwoPolynomialFit() :
    PerDatasetFit("two-polynomials", 
                  "Fit to a polynomial function",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  }
};
  

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
TwoPolynomialFit fit_two_polynomial;

///////////////////////////////////////////////////////////////////////////


/// This fit somehow summarizes almost all the others, while being
/// more powerful (but slightly less easy to use).
class LinearKineticSystemFit : public PerDatasetFit {

  class Storage : public FitInternalStorage {
  public:
    /// The number of distinctSteps
    int distinctSteps;

    /// The steps.
    QStringList steps;

    /// The step names
    QHash<QString, int> stepNames;

    /// The number of species
    int species;
    
    /// Whether or not an offset is present
    bool offset;
    
    /// Whether or not we have additional irreversible loss rate
    /// constants (who could go below 0 ?)
    bool additionalLoss;
  };


protected:
  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds*/) const override {
    return deepCopy<Storage>(source);
  };


  virtual void processOptions(const CommandOptions & opts, FitData * data) const override
  {
    Storage * s = storage<Storage>(data);

    s->steps.clear();
    s->steps << "1" << "2" << "1"; // one step forward, one step backward.
    updateFromOptions(opts, "steps", s->steps);


    s->distinctSteps = 0;
    s->stepNames.clear();
    for(const QString & n : s->steps) {
      if(! s->stepNames.contains(n)) {
        s->stepNames[n] = s->distinctSteps;
        ++s->distinctSteps;
      }
    }
    

    s->species = 2;
    updateFromOptions(opts, "species", s->species);

    s->offset = false;
    updateFromOptions(opts, "offset", s->offset);

    s->additionalLoss = false;
    updateFromOptions(opts, "additional-loss", s->additionalLoss);

  }

  const double * currents(Storage * s, const double * params, int step) const {
    return params + (s->species * (s->species + 1) + (s->offset ? 2 : 1)) * step;
  };

  double * currents(Storage * s, double * params, int step) const {
    return params + (s->species * (s->species + 1) + (s->offset ? 2 : 1)) * step;
  };


  const double * stepConstants(Storage * s, const double * params, int step) const {
    return currents(s, params, step) + (s->offset ? s->species +1 : s->species);
  };

  double * stepConstants(Storage * s, double * params, int step) const {
    return currents(s, params, step) + (s->offset ? s->species +1 : s->species);
  };

  const double * initialConcentrations(Storage * s, const double * params) const {
    return currents(s, params, s->distinctSteps);
  };

  double * initialConcentrations(Storage * s, double * params) const {
    return currents(s, params, s->distinctSteps);
  };



public:

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);

    QList<ParameterDefinition> defs;

    QStringList names;
    for(int i = 0; i < s->distinctSteps; i++)
      names << "";
    for(const QString & k : s->stepNames.keys())
      names[s->stepNames[k]] = k;

    // First, potential-dependent information
    
    for(int i = 0; i < s->distinctSteps; i++) {
      QString suffix = QString("_#%1").arg(names[i]);

      /// Current of each species
      for(int j = 0; j < s->species; j++)
        defs << ParameterDefinition(QString("I_%1%2").arg(j+1).arg(suffix),
                                    false, true, true);

      if(s->offset)
        defs << ParameterDefinition(QString("I_off_%1").arg(suffix), true,
                                    true, true);

      /// Reaction constants
      for(int j = 0; j < s->species; j++)
        for(int k = 0; k < s->species; k++)
          defs << ParameterDefinition(QString("k_%1_%2%3").
                                      arg(j+1).arg(k+1).arg(suffix)); 

      defs << ParameterDefinition(QString("k_loss%1").arg(suffix));
      /// @todo Offer the possibility to set the sum and ratio rather
      /// than each individually
        
    }

    // Then, initial conditions
    for(int j = 0; j < s->species; j++)
      defs << ParameterDefinition(QString("alpha_%1_0").arg(j+1), true);
    
    // Then, steps.
    for(int i = 0; i < s->steps.size(); i++) {
      QChar step('a' + i);
      /// Starting time (including the 0)
      defs << ParameterDefinition(QString("xstart_%1").arg(step), true);
    }

    if(s->additionalLoss) {
      for(int i = 0; i < s->steps.size(); i++) {
        QChar step('a' + i);
        /// Starting time (including the 0)
        defs << ParameterDefinition(QString("kloss_%1").arg(step), true);
      }
    }
    
    return defs;
  };

  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override
  {
    Storage * s = storage<Storage>(data);

    LinearKineticSystem sys(s->species);


    QVarLengthArray<double, 30> concentrations(s->species);
    gsl_vector_view vco = gsl_vector_view_array(concentrations.data(), s->species);

    QVarLengthArray<double, 30> currents(s->species);
    gsl_vector_view vcu = gsl_vector_view_array(currents.data(), s->species);

    double offset_value = 0;
    int cur = -1;
    double xstart = 0;
    double xend = 0;

    const Vector & xv = ds->x();

    // Copy initial concentrations
    const double *c = initialConcentrations(s, a);
    for(int i = 0; i < s->species; i++) {
      concentrations[i] = c[i];
      if(c[i] < 0)
        throw RangeError("Concentration must be positive");
    }

    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      if(cur < 0 || xend < x) {
        cur++;

        // First, potential-dependant-stuff:
        int potential = s->stepNames[s->steps[cur]];
        double loss_add = 0;

        
        // Currents:
        const double  * base = this->currents(s, a, potential);
        for(int i = 0; i < s->species; i++)
          currents[i] = base[i];

        if(s->offset)
          offset_value = base[s->species];


        const double * sc = stepConstants(s, a, potential);

        /// @todo This check could optionnally be turned off.
        // Now, range checking: we don't want rate constants to be
        // negative !
        for(int i = 0; i <= s->species * s->species; i++)
          if(sc[i] < 0)
            throw RangeError(QString("Found a negative rate "
                                     "constant (step: %1): %2 !").
                             arg(potential).arg(i));

        base = initialConcentrations(s, a) + s->species;
        if(s->additionalLoss)
          loss_add = base[cur + s->steps.size()];
        
        // Global loss...
        sys.setConstants(sc, sc[s->species*s->species] + loss_add);

        /// @todo There may be an "off-by-one-data-point" mistake here
        sys.setInitialConcentrations(&vco.vector);

        xstart = base[cur];
        if(cur < s->steps.size() - 1) {
          xend = base[cur+1];
          if(xend < xstart)
            xend = xv.max();
        }
        else
          xend = xv.max();
      }
      x -= xstart;
      double res = 0;
      if(x >= 0) {
        sys.getConcentrations(x, &vco.vector);
        gsl_blas_ddot(&vco.vector, &vcu.vector, &res);
        res += offset_value;
      }
      gsl_vector_set(target, i, res);
    }

  };

  virtual void initialGuess(FitData * data, 
                            const DataSet *ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);

    const Vector & x = ds->x();
    const Vector & y = ds->y();

    double * base = NULL;
    for(int i = 0; i < s->distinctSteps; i++) {
      base = currents(s, a, i);
      for(int j = 0; j < s->species; j++)
        base[j] = (j == 0 ? y.max() : 0);

      if(s->offset)                // Alway set to 0
        base[s->species] = 0;

      base = stepConstants(s, a, i);
      for(int j = 0; j < s->species; j++)
        for(int k = 0; k < s->species; k++)
          base[j * s->species + k] = ( j == k ? 0 : ((j+0.5)*(i+k+1))*1e-2);
      base[s->species * s->species] = 1e-4;
    }

    base = initialConcentrations(s, a);
    for(int i = 0; i < s->species; i++)
      base[i] = (i ? 0 : 1);

    base += s->species;

    double xmax = Utils::roundValue(x.max());
    for(int i = 0; i < s->steps.size(); i++)
      base[i] = i * xmax/s->steps.size();

    
    if(s->additionalLoss) {
      base += s->steps.size();
      for(int i = 0; i < s->steps.size(); i++)
        base[i] = 0;
    }
    
  };

  virtual ArgumentList fitHardOptions() const override {
    return
      ArgumentList(QList<Argument *>()
                   << new IntegerArgument("species", 
                                          "Number of species",
                                          "Number of species")
                   << new BoolArgument("offset", 
                                       "Constant offset",
                                       "If on, allow for a constant offset to be added")
                   << new BoolArgument("additional-loss", 
                                       "Additional loss",
                                       "Additional unconstrained 'irreversible loss' rate constants")
                   << new 
                   SeveralStringsArgument(QRegExp(","),
                                          "steps", 
                                          "Steps",
                                          "Step list with numbered conditions")
                   );
  };

  LinearKineticSystemFit() :
    PerDatasetFit("linear-kinetic-system", 
                  "Several steps with a kinetic evolution",
                  "Fits of exponentials on several steps with "
                  "film loss bookkeeping", 1, -1, false) 
  { 
    makeCommands();
  };
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
LinearKineticSystemFit fit_linear_kinetic_system;
