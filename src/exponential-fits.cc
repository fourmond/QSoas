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

#include <soas.hh>

/// This fit provides for an arbitrary number of exponentials
///
/// @todo Provide a way to choose between k and tau ?
class ExponentialFit : public PerDatasetFit {

  /// The number of exponentials
  int exponentials;

  /// Whether there is an overall film loss
  bool filmLoss;

  /// Whether the amplitudes are relative or absolute
  bool absolute;

  /// Whether or not there is a slow linear phase
  bool slowPhase;

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
    exponentials = 1;
    updateFromOptions(opts, "exponentials", exponentials);

    filmLoss = false;
    updateFromOptions(opts, "loss", filmLoss);

    absolute = true;
    updateFromOptions(opts, "absolute", absolute);

    slowPhase = false;
    updateFromOptions(opts, "slow", slowPhase);
  }

  
  virtual QString optionsString() const {
    return QString("%1 exp, %2, %3, %4").
      arg(exponentials).arg((absolute ? "absolute" : "relative")).
      arg(filmLoss ? "loss" : "no loss").
      arg(slowPhase ? "slow phase" : "no slow phase");
  }

  virtual bool hasSubFunctions () const {
    return exponentials > 1;
  };

  virtual bool displaySubFunctions () const {
    return false;               // Don't display them by default ?
  };

  /// This computes the same thing as function but in addition
  /// computes the annotations should the annotations pointer be NULL.
  void annotatedFunction(const double * a, FitData * /*data*/, 
                         const DataSet * ds , gsl_vector * target,
                         QList<Vector> * annotations = NULL)
  {
    const Vector & xv = ds->x();

    // Checking that the time constants are positive.
    /// @todo This check could optionnally be turned off.
    for(int j = 0; j < exponentials; j++)
      if(a[2*j + 2] < 0)
        throw RangeError("Negative time constant !");

    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i] - a[0];

      double phases = 0;
      double ph;
      int fl_offset = 0;
      for(int j = 0; j < exponentials; j++)  {
        ph = a[2*j + 3] * exp(-x/a[2*j + 2]);
        phases += ph;
        if(annotations) {
          if(! absolute)
            ph *= a[1];
          (*annotations)[j][i] = ph + a[1];
        }
      }
      if(slowPhase) {
        phases += a[2 * exponentials + 2] * x;
        fl_offset += 1;
      }
      if(! absolute)
        phases *= a[1];
      phases += a[1];
      // Most things done
      if(filmLoss)
        phases *= exp(-x * a[2 * exponentials + 2 + fl_offset]);
      if(target)
        gsl_vector_set(target, i, phases);
    }
  };


public:

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;

    /// Current of the active form
    defs << ParameterDefinition("x0", true);

    /// The value at infinite time (barring slow stuff)
    defs << ParameterDefinition("A_inf");

    // Normal exponential forms:
    for(int i = 0; i < exponentials; i++) {
      defs << ParameterDefinition(QString("tau_%1").arg(i+1));

      if(absolute)
        defs << ParameterDefinition(QString("A_%1").arg(i+1));
      else
        defs << ParameterDefinition(QString("alpha_%1").arg(i+1));
    }

    if(slowPhase)
      defs << ParameterDefinition("slow");

    if(filmLoss)
      defs << ParameterDefinition("kloss");
    
   
    return defs;
  };

  

  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target)
  {
    annotatedFunction(a, data, ds, target);
  };

  virtual void initialGuess(FitData * /*data*/, 
                            const DataSet *ds,
                            double * a)
  {
    a[0] = ds->x()[0];
    a[1] = ds->y().last();      
    double delta_x = fabs(ds->x().last() - ds->x().first());
    double delta_y = ds->y().first() - ds->y().last();
    for(int i = 0; i < exponentials; i++) {
      a[2*i + 2] = delta_x/(pow(3, exponentials  - i));
      if(absolute)
        a[2*i + 3] = delta_y/exponentials;
      else
        a[2*i + 3] = delta_y/(exponentials * a[1]);
    }
    int fl_off = 0;
    if(slowPhase) {
      a[2 * exponentials + 2] = 0.2 * delta_y/delta_x / 
        (absolute ? 1 : a[1]);
      fl_off = 1;
    }
    if(filmLoss)
      a[2 * exponentials + 2 + fl_off] = 0.03 / delta_x;
  };

  virtual void computeSubFunctions(const double * parameters,
                                   FitData * data, 
                                   const DataSet * ds,
                                   QList<Vector> * targetData,
                                   QStringList * targetAnnotations)
  {
    targetData->clear();
    targetAnnotations->clear();

    int sz = ds->x().size();
    for(int i = 0; i < exponentials; i++) {
      (*targetData) << Vector(sz, 0);
      *targetAnnotations << QString("Phase %1").arg(i+1);
    }
    
    annotatedFunction(parameters, data, ds, NULL,
                      targetData);
  };

  virtual ArgumentList * fitHardOptions() const {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new IntegerArgument("exponentials", 
                                          "Number of exponentials",
                                          "Number of exponentials")
                   << new 
                   BoolArgument("absolute", 
                                "Absolute",
                                "Amplitude is absolute or relative to "
                                "the asymptote ?")
                   << new 
                   BoolArgument("loss", 
                                "Loss",
                                "Is there an overall fully exponential loss ?")
                   << new 
                   BoolArgument("slow", 
                                "Slow phase",
                                "Is there a very slow phase ?")
                   );
    return opts;
  };

  ExponentialFit() :
    PerDatasetFit("exponential-decay", 
                  "Multi-exponential fits",
                  "...", 1, -1, false) 
  { 
    makeCommands();
  };


};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
ExponentialFit exponentialFit;


///////////////////////////////////////////////////////////////////////////


/// This fit somehow summarizes almost all the others, while being
/// more powerful (but slightly less easy to use).
class MultiExpMultiStepFit : public PerDatasetFit {

  /// The number of distinctSteps
  int distinctSteps;

  /// The steps.
  QList<int> steps;

  /// The number of exponentials
  int exponentials;

  /// Whether film loss depends the "position" or the "step" (ie
  /// potential)
  bool independentLoss;

  /// The base index for step \a i
  int baseStep(int i) const {
    if(independentLoss)
      return i * (exponentials + 1);
    else 
      return i * (exponentials + 2);
  };

  /// The base index for "position" \a i
  int basePosition(int i) const {
    return baseStep(distinctSteps) + 
      i * (exponentials + (independentLoss ? 2 : 1) );
  };

  /// The base index for exponential time constants in step
  int baseTime() {
    if(independentLoss)
      return 1;
    else
      return 2;
  };

  /// The base index for exponential amplitude in position
  int baseAmplitude() {
    if(independentLoss)
      return 2;
    else
      return 1;
  };
  

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
    steps.clear();
    steps << 0 << 1 << 0; // one step forward, one step backward.
    updateFromOptions(opts, "steps", steps);

    exponentials = 2;
    updateFromOptions(opts, "exponentials", exponentials);

    independentLoss = true;
    updateFromOptions(opts, "independent", independentLoss);

    distinctSteps = 0;
    for(int i = 0; i < steps.size(); i++)
      if(distinctSteps < steps[i])
        distinctSteps = steps[i];
    distinctSteps++;            // To get the actual number ! 
  }



public:

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;

    // First, potential-dependent information
    for(int i = 0; i < distinctSteps; i++) {

      /// Current of the active form
      defs << ParameterDefinition(QString("I_%1").arg(i+1));

      if( ! independentLoss)
        defs << ParameterDefinition(QString("kloss_%1").arg(i+1));

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
      if(independentLoss)
        defs << ParameterDefinition(QString("kloss_%1").arg(step));

      /// Amplitudes
      for(int j = 0; j < exponentials; j++) 
        defs << ParameterDefinition(QString("alpha_%1_%2").arg(step).arg(j+1));
    }
    
    return defs;
  };

  virtual void function(const double * a, FitData * /*data*/, 
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
        const double * base = a + baseStep(potential);
        I = base[0];
        for(int j = 0; j < exponentials; j++) {
          timeConstants[j] = base[baseTime() + j];
          if(timeConstants[j] < 0)
            throw RangeError("Negative time constant !");
        }

        if(! independentLoss)
          kloss = base[1];


        // Then, step-dependant stuff:
        base = a + basePosition(cur);

        if(independentLoss)
          kloss = base[1];

        if(kloss < 0)
          throw RangeError("Negative film loss rate constant !");

        
        xstart = base[0];
        if(cur < steps.size() - 1)
          xend = a[basePosition(cur+1)];
        else
          xend = xv.max();

        for(int j = 0; j < exponentials; j++)
          amplitudes[j] = base[j+baseAmplitude()];
      }
      x -= xstart;
      double val = 1;
      for(int j = 0; j < exponentials; j++)
        val -= amplitudes[j] * exp(-x/timeConstants[j]);
      val *= loss * exp(-x * kloss);
      gsl_vector_set(target, i, val * I);
    }
  };

  virtual void initialGuess(FitData * /*data*/, 
                            const DataSet *ds,
                            double * a)
  {
    const Vector & x = ds->x();
    const Vector & y = ds->y();
    for(int i = 0; i < distinctSteps; i++) {
      double * base = a + baseStep(i);
      base[0] = y.max();
      double ts = (x.max() - x.min())*0.3;
      for(int j = exponentials - 1 ; j >= 0; j--) {
        base[baseTime() + j] = ts;
        ts *= 0.3;
      }
      if(! independentLoss)
        base[1] = 1e-4;
    }

    for(int i = 0; i < steps.size(); i++) {
      double * base = a + basePosition(i);

      double xmax = Utils::roundValue(x.max());
      base[0] = i * xmax/steps.size();
      base[1] = 1e-4;
      for(int j = 0; j < exponentials; j++)
        base[baseAmplitude() + j] = 0.2;
    }
    
  };

  virtual ArgumentList * fitHardOptions() const {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new IntegerArgument("exponentials", 
                                          "Number of exponentials",
                                          "Number of exponentials")
                   << new 
                   SeveralIntegersArgument("steps", 
                                           "Steps",
                                           "Step list with numbered conditions")
                   << new 
                   BoolArgument("independent", 
                                "Independent",
                                "Whether irreversible loss is independent on "
                                "each step")
                   );
    return opts;
  };

  MultiExpMultiStepFit() :
    PerDatasetFit("multiexp-multistep", 
                  "Multi-step and multi-exponential",
                  "Fits of exponentials on several steps with "
                  "film loss bookkeeping", 1, -1, false) 
  { 
    makeCommands();
  };
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
MultiExpMultiStepFit fit_multiexp_multistep;

///////////////////////////////////////////////////////////////////////////


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

