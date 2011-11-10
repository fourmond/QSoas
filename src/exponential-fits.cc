/**
   @file exponential-fits.cc
   Exponential-based fits.
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

#include <argumentmarshaller.hh>
#include <fitdata.hh>
#include <commandeffector.hh>
#include <commandeffector-templates.hh>
#include <fitdialog.hh>
#include <general-arguments.hh>

#include <soas.hh>

/// @todo Ideas:
/// @li fits with arbitrary number of exponentials (using a number
/// parameter) ?
/// @li polynomial fits ?
class FilmExpFit : public FunctionFit {
public:

  /// Formula:
  /// a[1] * exp(-(x - a[0]) / a[2]) + a[3];
  ///
  /// a[0] is a redundant parameter, but it can be of use to shift the
  /// X axis without touching the original data.
  virtual double function(const double * a, 
                          FitData * , double x) {
    return (a[1] * exp(-(x - a[0]) / a[2]) + a[3]) * 
      exp(-a[4] * (x - a[0]));
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = ds->x()[0];          // x0 = x[0]
    a[3] = ds->y().last();      
    a[1] = ds->y()[0] - a[3];
    a[2] = (ds->x().last() - a[0])/3;
    a[4] = 1e-3/(ds->x().last() - a[0]);
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("x0", true)
      << ParameterDefinition("A")
      << ParameterDefinition("tau")
      << ParameterDefinition("B")
      << ParameterDefinition("kloss");
  };


  FilmExpFit() : FunctionFit("film-expd", 
                             "Exponential decay with film loss",
                             "Exponential decay with film loss, formula is :...") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
FilmExpFit fit_film_expd;


///////////////////////////////////////////////////////////////////////////

class ExpFit : public FunctionFit {
public:

  /// Formula:
  /// a[1] * exp(-(x - a[0]) / a[2]) + a[3];
  ///
  /// a[0] is a redundant parameter, but it can be of use to shift the
  /// X axis without touching the original data.
  virtual double function(const double * a, 
                          FitData * , double x) {
    return a[1] * exp(-(x - a[0]) / a[2]) + a[3];
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = ds->x()[0];          // x0 = x[0]
    a[3] = ds->y().last();      
    a[1] = ds->y()[0] - a[3];
    a[2] = (ds->x().last() - a[0])/3;
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("x0", true)
      << ParameterDefinition("A")
      << ParameterDefinition("tau")
      << ParameterDefinition("B");
  };


  ExpFit() : FunctionFit("expd", 
                         "Exponential decay",
                         "Exponential decay, formula is :...") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
ExpFit fit_expd;


///////////////////////////////////////////////////////////////////////////

class Exp2Fit : public FunctionFit {
public:

  /// Formula:
  /// a[1] * exp(-(x - a[0]) / a[2]) + a[3] * exp(-(x - a[0]) / a[4]) + a[5];
  ///
  /// a[0] is a redundant parameter, but it can be of use to shift the
  /// X axis without touching the original data.
  virtual double function(const double * a, 
                          FitData * , double x) {
    return a[1] * exp(-(x - a[0]) / a[2]) + a[3] * 
      exp(-(x - a[0]) / a[4]) + a[5];
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = ds->x()[0];          // x0 = x[0]
    a[5] = ds->y().last();      
    a[1] = 0.5*(ds->y()[0] - a[5]);
    a[2] = (ds->x().last() - a[0])/20;
    a[3] = a[1];
    a[4] = (ds->x().last() - a[0])/3;
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("x0", true)
      << ParameterDefinition("A1")
      << ParameterDefinition("tau1")
      << ParameterDefinition("A2")
      << ParameterDefinition("tau2")
      << ParameterDefinition("B");
  };


  Exp2Fit() : FunctionFit("expd2", 
                          "Bi-exponential decay",
                          "Bi-exponential decay, formula is :...") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
Exp2Fit fit_expd2;


///////////////////////////////////////////////////////////////////////////

class FilmExp2Fit : public FunctionFit {
public:

  /// Formula:
  /// a[1] * exp(-(x - a[0]) / a[2]) + a[3] * exp(-(x - a[0]) / a[4]) + a[5];
  ///
  /// a[0] is a redundant parameter, but it can be of use to shift the
  /// X axis without touching the original data.
  virtual double function(const double * a, 
                          FitData * , double x) {
    return (a[1] * exp(-(x - a[0]) / a[2]) + a[3] * 
            exp(-(x - a[0]) / a[4]) + a[5]) * 
      exp(-a[6] * (x - a[0]));
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = ds->x()[0];          // x0 = x[0]
    a[5] = ds->y().last();      
    a[1] = 0.5*(ds->y()[0] - a[5]);
    a[2] = (ds->x().last() - a[0])/20;
    a[3] = a[1];
    a[4] = (ds->x().last() - a[0])/3;
    a[6] = 1e-3/(ds->x().last() - a[0]);
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("x0", true)
      << ParameterDefinition("A1")
      << ParameterDefinition("tau1")
      << ParameterDefinition("A2")
      << ParameterDefinition("tau2")
      << ParameterDefinition("B")
      << ParameterDefinition("kloss");
  };


  FilmExp2Fit() : FunctionFit("film-expd2", 
                              "Bi-exponential decay with film loss",
                              "Bi-exponential decay with film loss, formula is :...") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
FilmExp2Fit fit_film_expd2;

///////////////////////////////////////////////////////////////////////////

/// Same as Exp2Fit, but all amplitudes are relative
class Exp2RelFit : public FunctionFit {
public:

  virtual double function(const double * a, 
                          FitData * , double x) {
    return a[1] * ( a[2] * exp(-(x - a[0]) / a[3]) + 
                    a[4] * exp(-(x - a[0]) / a[5]) + 
                    (1 - a[2] - a[4]));
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = ds->x()[0];          // x0 = x[0]
    a[1] = ds->y().first();      
    a[2] = 0.3;
    a[3] = (ds->x().last() - a[0])/20;
    a[4] = a[2];
    a[5] = (ds->x().last() - a[0])/3;
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("x0", true)
      << ParameterDefinition("A")
      << ParameterDefinition("alpha1")
      << ParameterDefinition("tau1")
      << ParameterDefinition("alpha2")
      << ParameterDefinition("tau2");
  };


  Exp2RelFit() : FunctionFit("expd2-rel", 
                          "Bi-exponential decay with relative amplitude",
                          "Bi-exponential decay, formula is :...") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
Exp2RelFit fit_expd2_rel;

///////////////////////////////////////////////////////////////////////////

/// Same as Exp2Fit, but all amplitudes are relative
class FilmExp2RelFit : public FunctionFit {
public:

  virtual double function(const double * a, 
                          FitData * , double x) {
    return a[1] * (a[2] * exp(-(x - a[0]) / a[3]) + 
                   a[4] * exp(-(x - a[0]) / a[5]) + 
                   (1 - a[2] - a[4])) * 
      exp(-(x - a[0]) * a[6]);
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = ds->x()[0];          // x0 = x[0]
    a[1] = ds->y().first();      
    a[2] = 0.3;
    a[3] = (ds->x().last() - a[0])/20;
    a[4] = a[2];
    a[5] = (ds->x().last() - a[0])/3;
    a[6] = 1e-3/(ds->x().last() - a[0]);
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("x0", true)
      << ParameterDefinition("A")
      << ParameterDefinition("alpha1")
      << ParameterDefinition("tau1")
      << ParameterDefinition("alpha2")
      << ParameterDefinition("tau2")
      << ParameterDefinition("kloss");
  };


  FilmExp2RelFit() : 
    FunctionFit("film-expd2-rel", 
                "Bi-exponential decay with relative amplitude and film loss",
                "Bi-exponential decay with relative amplitude and film loss, "
                "formula is :...") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
FilmExp2RelFit fit_film_expd2_rel;

///////////////////////////////////////////////////////////////////////////


/// Three exponential fits, with relative amplitudes
class Exp3RelFit : public FunctionFit {
public:

  virtual double function(const double * a, 
                          FitData * , double x) {
    return a[1] * ( a[2] * exp(-(x - a[0]) / a[3]) + 
                    a[4] * exp(-(x - a[0]) / a[5]) + 
                    a[6] * exp(-(x - a[0]) / a[7]) + 
                    (1 - a[2] - a[4] - a[6]));
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = ds->x()[0];          // x0 = x[0]
    a[1] = ds->y().first();      
    a[2] = 0.1;
    a[3] = (ds->x().last() - a[0])/50;
    a[4] = a[2];
    a[5] = (ds->x().last() - a[0])/6;
    a[6] = a[2];
    a[7] = (ds->x().last() - a[0])/2;
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("x0", true)
      << ParameterDefinition("A")
      << ParameterDefinition("alpha1")
      << ParameterDefinition("tau1")
      << ParameterDefinition("alpha2")
      << ParameterDefinition("tau2")
      << ParameterDefinition("alpha3")
      << ParameterDefinition("tau3");
  };


  Exp3RelFit() : FunctionFit("expd3-rel", 
                             "Tri-exponential decay with relative amplitude",
                             "Tri-exponential decay, formula is :...") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
Exp3RelFit fit_expd3_rel;


///////////////////////////////////////////////////////////////////////////


/// This fit somehow summarizes almost all the others, while being
/// more powerful (but slightly less easy to use).
/// 
class MultiExpMultiStepFit : public PerDatasetFit {

  /// The number of distinctSteps
  int distinctSteps;

  /// The steps.
  QList<int> steps;

  /// The number of exponentials
  int exponentials;
  

  void runFitCurrentDataSet(const QString &n, const CommandOptions & opts)
  {
    QList<const DataSet *> ds;
    ds << soas().currentDataSet();
    runFit(n, ds, opts);
  }

  void runFit(const QString &, QList<const DataSet *> datasets,
              const CommandOptions & opts)
  {
    steps.clear();
    steps << 0 << 1 << 0; // one step forward, one step backward.
    updateFromOptions(opts, "steps", steps);

    exponentials = 2;
    updateFromOptions(opts, "exponentials", exponentials);

    distinctSteps = 0;
    for(int i = 0; i < steps.size(); i++)
      if(distinctSteps < steps[i])
        distinctSteps = steps[i];
    distinctSteps++;            // To get the actual number ! 

    
    {
      FitData data(this, datasets);
      FitDialog dlg(&data);
      dlg.exec();
    }
  }

public:

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;

    // First, potential-dependent information
    for(int i = 0; i < distinctSteps; i++) {

      /// Current of the active form
      defs << ParameterDefinition(QString("I_%1").arg(i+1));

      /// Exponentials time constants:
      for(int j = 0; j < exponentials; j++) 
        defs << ParameterDefinition(QString("tau_%1_%2").arg(i+1).arg(j+1));
    }
    
    // Then, step-based information
    for(int i = 0; i < steps.size(); i++) {
      QChar step('a' + i);
      /// Starting time (including the 0)
      defs << ParameterDefinition(QString("xstart_%1").arg(step), true);

      /// Film loss.
      defs << ParameterDefinition(QString("kloss_%1").arg(step));

      /// Amplitudes
      for(int j = 0; j < exponentials; j++) 
        defs << ParameterDefinition(QString("alpha_%1_%2").arg(step).arg(j+1));
    }
    
    return defs;
  };

  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target)
  {
    // Stuff only pertaining to a given step
    
    QVarLengthArray<double, 30> timeConstants(exponentials);
    QVarLengthArray<double, 30> amplitudes(exponentials);
    double kloss;
    double I;                   // Current
    double xstart;
    double xend;
    int cur = -1;

    // The overall loss on all the previous steps.
    double loss = 1;
    

    const Vector & xv = ds->x();

    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      if(cur < 0 || xend < x) {
        // Next step
        if(cur >= 0)            // loss bookkeeping
          loss *= exp(kloss * (xstart - x));
        cur++;


        // First, potential-dependant-stuff:
        int potential = steps[cur];
        const double * base = a + potential * (exponentials + 1);
        I = base[0];
        for(int j = 0; j < exponentials; j++)
          timeConstants[j] = base[1 + j];

        // Then, step-dependant stuff:
        base = a + distinctSteps * (exponentials + 1) + 
          (exponentials + 2)*cur;
        
        xstart = base[0];
        if(cur < steps.size() - 1)
          xend = base[exponentials + 2];
        else
          xend = xv.max();

        kloss = base[1];
        for(int j = 0; j < exponentials; j++)
          amplitudes[j] = base[j+2];
      }
      x -= xstart;
      double val = 1;
      for(int j = 0; j < exponentials; j++)
        val -= amplitudes[j] * exp(-x/timeConstants[j]);
      val *= loss * exp(-x * kloss);
      gsl_vector_set(target, i, val * I);
    }
  };

  virtual void initialGuess(FitData * params, 
                            const DataSet *ds,
                            double * a)
  {
    const Vector & x = ds->x();
    const Vector & y = ds->y();
    for(int i = 0; i < distinctSteps; i++) {
      double * base = a + i * (exponentials + 1);
      base[0] = y.max();
      double ts = (x.max() - x.min())*0.3;
      for(int j = exponentials - 1 ; j >= 0; j--) {
        base[1 + j] = ts;
        ts *= 0.3;
      }
    }
    for(int i = 0; i < steps.size(); i++) {
      double * base = a + distinctSteps * (exponentials + 1) + 
        (exponentials + 2)*i;

      double xmax = Utils::roundValue(x.max());
      base[0] = i * xmax/steps.size();
      base[1] = 1e-4;
      for(int j = 0; j < exponentials; j++)
        base[j+2] = 0.2;
    }
    
  };

  MultiExpMultiStepFit() :
    PerDatasetFit("multiexp-multistep", 
                  "Multi-step and multi-exponential",
                  "Fits of exponentials on several steps with "
                  "film loss bookkeeping", 1, -1, false) 
  { 
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new IntegerArgument("exponentials", 
                                          "Number of exponentials",
                                          "Number of exponentials")
                   << new 
                   SeveralIntegersArgument("steps", 
                                           "Steps",
                                           "Step list with numbered conditions"));
    makeCommands(NULL, 
                 effector(this, 
                          &MultiExpMultiStepFit::runFitCurrentDataSet),
                 effector(this, 
                          &MultiExpMultiStepFit::runFit),
                 opts);
  };
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
MultiExpMultiStepFit fit_multiexp_multistep;
