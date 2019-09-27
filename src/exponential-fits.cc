/**
   @file exponential-fits.cc
   Exponential-based fits.
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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

  class Storage : public FitInternalStorage {
  public:
    /// The number of exponentials
    int exponentials;
    
    /// Whether there is an overall film loss
    bool filmLoss;

    /// Whether the amplitudes are relative or absolute
    bool absolute;

    /// Whether or not there is a slow linear phase
    bool slowPhase;
  };

protected:
  
  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/,
                                           FitInternalStorage * source,
                                           int /*ds*/) const override {
    return deepCopy<Storage>(source);
  };

  virtual bool threadSafe() const override {
    return true;
  };


  virtual void processOptions(const CommandOptions & opts, FitData * data) const override
  {
    Storage * s = storage<Storage>(data);

    s->exponentials = 1;
    s->filmLoss = false;
    s->absolute = true;
    s->slowPhase = false;

    updateFromOptions(opts, "slow", s->slowPhase);
    updateFromOptions(opts, "exponentials", s->exponentials);
    updateFromOptions(opts, "loss", s->filmLoss);
    updateFromOptions(opts, "absolute", s->absolute);
  }

  
  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString("%1 exp, %2, %3, %4").
      arg(s->exponentials).arg((s->absolute ? "absolute" : "relative")).
      arg(s->filmLoss ? "loss" : "no loss").
      arg(s->slowPhase ? "slow phase" : "no slow phase");
  }

  virtual bool hasSubFunctions(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return s->exponentials > 1;
  };

  virtual bool displaySubFunctions(FitData *) const override {
    return false;               // Don't display them by default ?
  };

  /// This computes the same thing as function but in addition
  /// computes the annotations if the annotations pointer is not NULL.
  void annotatedFunction(const double * a, FitData * data, 
                         const DataSet * ds , gsl_vector * target,
                         QList<Vector> * annotations = NULL) const
  {
    Storage * s = storage<Storage>(data);
    const Vector & xv = ds->x();

    // Checking that the time constants are positive.
    /// @todo This check could optionnally be turned off.
    for(int j = 0; j < s->exponentials; j++)
      if(a[2*j + 2] < 0)
        throw RangeError("Negative time constant !");

    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i] - a[0];

      double phases = 0;
      double ph;
      int fl_offset = 0;
      for(int j = 0; j < s->exponentials; j++)  {
        ph = a[2*j + 3] * exp(-x/a[2*j + 2]);
        phases += ph;
        if(annotations) {
          if(! s->absolute)
            ph *= a[1];
          (*annotations)[j][i] = ph + a[1];
        }
      }
      if(s->slowPhase) {
        phases += a[2 * s->exponentials + 2] * x;
        fl_offset += 1;
      }
      if(! s->absolute)
        phases *= a[1];
      phases += a[1];
      // Most things done
      if(s->filmLoss)
        phases *= exp(-x * a[2 * s->exponentials + 2 + fl_offset]);
      if(target)
        gsl_vector_set(target, i, phases);
    }
  };


public:

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    QList<ParameterDefinition> defs;

    /// Current of the active form
    defs << ParameterDefinition("x0", true);

    /// The value at infinite time (barring slow stuff)
    defs << ParameterDefinition("A_inf");

    // Normal exponential forms:
    for(int i = 0; i < s->exponentials; i++) {
      defs << ParameterDefinition(QString("tau_%1").arg(i+1));

      if(s->absolute)
        defs << ParameterDefinition(QString("A_%1").arg(i+1));
      else
        defs << ParameterDefinition(QString("alpha_%1").arg(i+1));
    }

    if(s->slowPhase)
      defs << ParameterDefinition("slow");

    if(s->filmLoss)
      defs << ParameterDefinition("kloss");
    
   
    return defs;
  };

  

  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override
  {
    annotatedFunction(a, data, ds, target);
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet *ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);

    a[0] = ds->x()[0];
    a[1] = ds->y().last();      
    double delta_x = fabs(ds->x().last() - ds->x().first());
    double delta_y = ds->y().first() - ds->y().last();
    for(int i = 0; i < s->exponentials; i++) {
      a[2*i + 2] = delta_x/(pow(3, s->exponentials  - i));
      if(s->absolute)
        a[2*i + 3] = delta_y/s->exponentials;
      else
        a[2*i + 3] = delta_y/(s->exponentials * a[1]);
    }
    int fl_off = 0;
    if(s->slowPhase) {
      a[2 * s->exponentials + 2] = 0.2 * delta_y/delta_x / 
        (s->absolute ? 1 : a[1]);
      fl_off = 1;
    }
    if(s->filmLoss)
      a[2 * s->exponentials + 2 + fl_off] = 0.03 / delta_x;
  };

  virtual void computeSubFunctions(const double * parameters,
                                   FitData * data, 
                                   const DataSet * ds,
                                   QList<Vector> * targetData,
                                   QStringList * targetAnnotations) const override
  {
    Storage * s = storage<Storage>(data);

    targetData->clear();
    targetAnnotations->clear();

    int sz = ds->x().size();
    for(int i = 0; i < s->exponentials; i++) {
      (*targetData) << Vector(sz, 0);
      *targetAnnotations << QString("Phase %1").arg(i+1);
    }
    
    annotatedFunction(parameters, data, ds, NULL,
                      targetData);
  };

  virtual ArgumentList * fitHardOptions() const override {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new IntegerArgument("exponentials", 
                                          "Number of exponentials",
                                          "Number of exponentials")
                   << new 
                   BoolArgument("absolute", 
                                "Absolute",
                                "whether the amplitude is absolute or relative to "
                                "the asymptote (defaults to true)")
                   << new 
                   BoolArgument("loss", 
                                "Loss",
                                "wether the sum of exponentials should be multiplied by an exp(-kt) function (default: false)")
                   << new 
                   BoolArgument("slow", 
                                "Slow phase",
                                "whether there is a very slow phase (that shows up as a linear change in Y against time, defaults: false)")
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

  class Storage : public FitInternalStorage {
  public:
    /// The number of distinctSteps
    int distinctSteps;

    /// The steps.
    QList<int> steps;

    /// The number of exponentials
    int exponentials;

    /// Whether film loss depends the "position" or the "step" (ie
    /// potential)
    bool independentLoss;
  };

  /// The base index for step \a i
  int baseStep(int i, Storage * s) const {
    if(s->independentLoss)
      return i * (s->exponentials + 1);
    else 
      return i * (s->exponentials + 2);
  };

  /// The base index for "position" \a i
  int basePosition(int i, Storage * s) const {
    return baseStep(s->distinctSteps, s) + 
      i * (s->exponentials + (s->independentLoss ? 2 : 1) );
  };

  /// The base index for exponential time constants in step
  int baseTime(Storage * s) const {
    if(s->independentLoss)
      return 1;
    else
      return 2;
  };

  /// The base index for exponential amplitude in position
  int baseAmplitude(Storage * s) const {
    if(s->independentLoss)
      return 2;
    else
      return 1;
  };
  

protected:

  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds*/) const  override{
    return deepCopy<Storage>(source);
  };

  virtual void processOptions(const CommandOptions & opts, FitData * data) const override
  {
    Storage * s = storage<Storage>(data);

    s->steps.clear();
    s->steps << 0 << 1 << 0; // one step forward, one step backward.
    updateFromOptions(opts, "steps", s->steps);

    s->exponentials = 2;
    updateFromOptions(opts, "exponentials", s->exponentials);

    s->independentLoss = true;
    updateFromOptions(opts, "independent", s->independentLoss);

    s->distinctSteps = 0;
    for(int i = 0; i < s->steps.size(); i++)
      if(s->distinctSteps < s->steps[i])
        s->distinctSteps = s->steps[i];
    s->distinctSteps++;            // To get the actual number ! 
  }



public:

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    QList<ParameterDefinition> defs;

    // First, potential-dependent information
    for(int i = 0; i < s->distinctSteps; i++) {

      /// Current of the active form
      defs << ParameterDefinition(QString("I_%1").arg(i+1));

      if( ! s->independentLoss)
        defs << ParameterDefinition(QString("kloss_%1").arg(i+1));

      /// Exponentials time constants:
      for(int j = 0; j < s->exponentials; j++) 
        defs << ParameterDefinition(QString("tau_%1_%2").arg(i+1).arg(j+1));
        
    }
    
    // Then, step-based information
    for(int i = 0; i < s->steps.size(); i++) {
      QChar step('a' + i);
      /// Starting time (including the 0)
      defs << ParameterDefinition(QString("xstart_%1").arg(step), true);

      /// Film loss.
      if(s->independentLoss)
        defs << ParameterDefinition(QString("kloss_%1").arg(step));

      /// Amplitudes
      for(int j = 0; j < s->exponentials; j++) 
        defs << ParameterDefinition(QString("alpha_%1_%2").arg(step).arg(j+1));
    }
    
    return defs;
  };

  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString("%1 steps, %2 exponentials, %3 loss").
      arg(s->steps.size()).arg(s->exponentials).arg(s->independentLoss ? "independent" : "common");
  };

  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override
  {
    Storage * s = storage<Storage>(data);

    // Stuff only pertaining to a given step
    
    QVarLengthArray<double, 30> timeConstants(s->exponentials);
    QVarLengthArray<double, 30> amplitudes(s->exponentials);
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
        int potential = s->steps[cur];
        const double * base = a + baseStep(potential, s);
        I = base[0];
        for(int j = 0; j < s->exponentials; j++) {
          timeConstants[j] = base[baseTime(s) + j];
          if(timeConstants[j] < 0)
            throw RangeError("Negative time constant !");
        }

        if(! s->independentLoss)
          kloss = base[1];


        // Then, step-dependant stuff:
        base = a + basePosition(cur, s);

        if(s->independentLoss)
          kloss = base[1];

        if(kloss < 0)
          throw RangeError("Negative film loss rate constant !");

        
        xstart = base[0];
        if(cur < s->steps.size() - 1)
          xend = a[basePosition(cur+1, s)];
        else
          xend = xv.max();

        for(int j = 0; j < s->exponentials; j++)
          amplitudes[j] = base[j+baseAmplitude(s)];
      }
      x -= xstart;
      double val = 1;
      for(int j = 0; j < s->exponentials; j++)
        val -= amplitudes[j] * exp(-x/timeConstants[j]);
      val *= loss * exp(-x * kloss);
      gsl_vector_set(target, i, val * I);
    }
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet *ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);

    const Vector & x = ds->x();
    const Vector & y = ds->y();
    for(int i = 0; i < s->distinctSteps; i++) {
      double * base = a + baseStep(i, s);
      base[0] = y.max();
      double ts = (x.max() - x.min())*0.3;
      for(int j = s->exponentials - 1 ; j >= 0; j--) {
        base[baseTime(s) + j] = ts;
        ts *= 0.3;
      }
      if(! s->independentLoss)
        base[1] = 1e-4;
    }

    for(int i = 0; i < s->steps.size(); i++) {
      double * base = a + basePosition(i, s);

      double xmax = Utils::roundValue(x.max());
      base[0] = i * xmax/s->steps.size();
      base[1] = 1e-4;
      for(int j = 0; j < s->exponentials; j++)
        base[baseAmplitude(s) + j] = 0.2;
    }
    
  };

  virtual ArgumentList * fitHardOptions() const override {
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


