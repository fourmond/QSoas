/*
  kineticsystemevolver.cc: implementation of KineticSystemEvolver
  Copyright 2012 by Vincent Fourmond

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
#include <kineticsystemevolver.hh>

#include <kineticsystem.hh>
#include <exceptions.hh>

#include <soas.hh>
#include <terminal.hh>
#include <command.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <vector.hh>
#include <dataset.hh>
#include <datastack.hh>

#include <expression.hh>

// Here come fits !
#include <perdatasetfit.hh>
#include <fitdata.hh>

#include <gsl/gsl_const_mksa.h>


KineticSystemEvolver::KineticSystemEvolver(KineticSystem * sys) :
  system(sys), callback(NULL)
{
  QStringList p = system->allParameters();
  for(int i = 0; i < p.size(); i++)
    parameterIndex[p[i]] = i;
  
  parameters = new double[parameterIndex.size()];

  for(int i = 0; i < parameterIndex.size(); i++)
    parameters[i] = 0;          // Default to 0
}

KineticSystemEvolver::~KineticSystemEvolver()
{
  delete[] parameters;
}

int KineticSystemEvolver::dimension() const
{
  return system->speciesNumber();
}

int KineticSystemEvolver::computeDerivatives(double t, const double * y, 
                                             double * dydt)
{
  if(callback)                  // Tweak the time-dependent parameters
                                // when applicable
    callback(parameters, t, callbackParameter);
  system->computeDerivatives(dydt, y, parameters);
  if(false) {                /// @todo Add real debugging facilities ?
    // Debugging information
    QTextStream o(stdout);
    o << "Time t = " << t << "\n";
    for(int i = 0; i < system->speciesNumber(); i++)
      o << "y[" << i << "] = " << y[i] << "\t dy[" << i << "] = " 
        << dydt[i] << endl;
  }
  return GSL_SUCCESS;
}

void KineticSystemEvolver::setParameter(int index, double value)
{
  parameters[index] = value;
}

QStringList KineticSystemEvolver::setParameters(const QHash<QString, double> & p)
{
  QSet<QString> params = QSet<QString>::fromList(parameterIndex.keys());
  for(QHash<QString, double>::const_iterator i = p.begin();
      i != p.end(); i++) {
    int idx = parameterIndex.value(i.key(), -1);
    if(idx >= 0) {
      parameters[idx] = i.value();
      params.remove(i.key());
    }
  }
  return params.toList();
}

void KineticSystemEvolver::setParameters(const QString & str)
{
  Expression::setParametersFromExpression(system->allParameters(),
                                          str, parameters);
}

void KineticSystemEvolver::setParameters(const double * source, int skip)
{
  int j = 0;
  for(int i = 0; i < parameterIndex.size(); i++)
    if(i != skip)
      parameters[i] = source[j++];
}

void KineticSystemEvolver::setParameters(const double * source, 
                                         const QSet<int> & skipped)
{
  int j = 0;
  for(int i = 0; i < parameterIndex.size(); i++)
    if(! skipped.contains(i))
      parameters[i] = source[j++];
}

void KineticSystemEvolver::initialize(double tstart)
{
  QVarLengthArray<double, 1000> tg(system->speciesNumber());
  system->setInitialConcentrations(tg.data(), parameters);
  ODESolver::initialize(tg.data(), tstart);
}

QHash<QString, double> KineticSystemEvolver::parameterValues() const
{
  QHash<QString, double> ret;
  QStringList p = system->allParameters();
  for(int i = 0; i < p.size(); i++)
    ret[p[i]] = parameters[i];
  return ret;
}


void KineticSystemEvolver::setupCallback(void (*cb)(double *, double, void *), 
                                         void * params)
{
  callback = cb;
  callbackParameter = params;
}


double KineticSystemEvolver::reporterValue() const
{
  if(! system->reporterExpression)
    return 0;                   // but really, this should fail
  QVarLengthArray<double, 1000> tg(system->speciesNumber() + 
                                   parameterIndex.size());
  
  for(int i = 0; i < system->speciesNumber(); i++)
    tg[i] = currentValues()[i];
  int idx = system->speciesNumber();
  for(int i = 0; i < parameterIndex.size(); i++)
    tg[i+idx] = parameters[i];

  return system->reporterExpression->evaluate(tg.data());
}

//////////////////////////////////////////////////////////////////////

static ArgumentList 
ksArgs (QList<Argument *>() 
        << new FileArgument("reaction-file", 
                            "Reaction file",
                            "File describing the kinetic system")
        << new StringArgument("parameters",
                              "Parameters",
                              "Parameters of the model")
        );

static ArgumentList 
ksOpts (QList<Argument *>() 
        << new BoolArgument("dump", 
                            "Dump system",
                            "If on, dumps the system rather than solving")
        << ODEStepperOptions::commandOptions()
        << new BoolArgument("annotate", 
                            "Annotate",
                            "If on, a last column will contain the number of function evaluation for each step")
        );

/// Just replace the time in the expression by its value !
static void setTime(double * params, double t, void * p)
{
  int idx = *((int*)p);
  params[idx] = t;
}

static void kineticSystemCommand(const QString &, QString file,
                                 QString parameters,
                                 const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  bool dump = false;
  updateFromOptions(opts, "dump", dump);
  
  KineticSystem sys; 
  sys.parseFile(file);
  sys.prepareForTimeEvolution();
  if(dump) {
    Terminal::out << "System " << file << ":\n" 
                  << sys.toString() << endl;
    Terminal::out << "System parameters: " 
                  << sys.allParameters().join(", ") << endl;
    return;
  }
  
  KineticSystemEvolver evolver(&sys);
  evolver.setParameters(parameters);

  ODEStepperOptions op = evolver.getStepperOptions();
  op.parseOptions(opts);
  evolver.setStepperOptions(op);

  bool annotate = false;
  updateFromOptions(opts, "annotate", annotate);


  QHash<QString, double> params = evolver.parameterValues();
  int timeIndex = 0;
  if(params.contains("t")) {
    params.remove("t");
    timeIndex = evolver.getParameterIndex("t");
    evolver.setupCallback(::setTime, &timeIndex);
  }
  QStringList p = params.keys();
  qSort(p);

  Terminal::out << "Parameter values: " << endl;
  for(int i = 0; i < p.size(); i++)
    Terminal::out << " * " << p[i] << " = " << params[p[i]] << endl;
  
  evolver.initialize(ds->x()[0]);
  QList<Vector> concentrations = evolver.steps(ds->x(), annotate);
  concentrations.insert(0, ds->x());
  DataSet * nds = new DataSet(concentrations);
  nds->name = QString("ks-%1.dat").arg(QFileInfo(file).fileName());

  Terminal::out << "Total number of function evaluations: " 
                << evolver.evaluations << endl;
  soas().stack().pushDataSet(nds); 

}

static Command 
kineticSystem("kinetic-system", // command name
              effector(kineticSystemCommand), // action
              "simulations",  // group name
              &ksArgs, // arguments
              &ksOpts, // options
              "Kinetic system evolver",
              "Computes the concentration of species of an "
              "arbitrary kinetic system",
              "...");

//////////////////////////////////////////////////////////////////////

/// This small represents a dependence on time of a parameter. It is
/// for now very basic. It is made of several bits that add up to
/// one. All bits have independent amplitude, but when they have other
/// accessory parameters (time constant ?), those can either be shared
/// or independent.
///
/// @questions Maybe this has to be turned into a general virtual
/// hierarchy ?
class TimeDependentParameter {
public:

  typedef enum {
    Exponential,
  } Type;

  /// What kind of funtion is that ?
  Type type;
  
  /// The number of bits
  int number;

  /// Whether the bits have independent accessory parameters or not
  bool independentBits;

  /// The base index of the parameters. @hack Mutable so that it can
  /// be updated from the const functions.
  int baseIndex;


  /// The number of parameters
  int parameterNumber() {
    switch(type) {
    case Exponential:
      return number*2 + (independentBits ? number : 1);
    default:
      return 0;
    };
    return 0;                   // Not needed in principle.
  };

  /// Parameter definitions
  QList<ParameterDefinition> parameters(const QString & prefix) {
    QList<ParameterDefinition> ret;
    switch(type) {
    case Exponential:
      if(! independentBits)
        ret << ParameterDefinition(prefix + "_tau");
      for(int i = 0; i < number; i++) { 
        ret << ParameterDefinition(QString("%2_%1").
                                   arg(i+1).arg(prefix), true);
        ret << ParameterDefinition(QString("%2_t_%1").
                                   arg(i+1).arg(prefix), true);
        if(independentBits)
          ret << ParameterDefinition(QString("%2_tau_%1").
                                     arg(i+1).arg(prefix));
      }
      break;
    default:
      ;
    };
    return ret;
  };

  /// Returns the value at the given time...
  double computeValue(double t, const double * parameters) const {
    double value = 0;
    switch(type) {
    case Exponential:
      for(int i = 0; i < number; i++) {
        double t0   = parameters[baseIndex + (independentBits ? i*3+1 : 2*i+2)];
        double conc = parameters[baseIndex + (independentBits ? i*3   : 2*i+1)];
        double tau  = parameters[baseIndex + (independentBits ? i*3+2 : 0)];
        if(tau < 0)             // Well, the check happens a lot, but
                                // is less expensive than an
                                // exponential anyway
          throw RangeError("Negative tau value");
        if(t >= t0)
          value += conc * exp(-(t - t0)/tau);
      }
      break;
    default:
      ;
    }
    return value;
  };

  /// Sets a reasonable initial guess for these parameters
  void setInitialGuess(double * parameters, const DataSet * ds) const {
    double dx = ds->x().max() - ds->x().min();
    switch(type) {
    case Exponential:
      for(int i = 0; i < number; i++) {
        double & t0   = parameters[baseIndex + (independentBits ? i*3+1 : 2*i+2)];
        double & conc = parameters[baseIndex + (independentBits ? i*3   : 2*i+1)];
        double & tau  = parameters[baseIndex + (independentBits ? i*3+2 : 0)];
        tau = 20;
        conc = 1;
        t0 = ds->x().min() + (i+1) * dx/(number+1);
      }
      break;
    default:
      ;
    }
  };

  /// Returns the time at which there are potential discontinuities
  Vector discontinuities(const double * parameters) const {
    Vector ret;
    switch(type) {
    case Exponential:
      for(int i = 0; i < number; i++)
        ret << parameters[baseIndex + (independentBits ? i*3+1 : 2*i+2)];
      break;
    default:
      ;
    }
    return ret;
  };

  /// Parses a spec for the time-based stuff. Takes a string of the
  /// form number, type, common
  void parseFromString(const QString & str) {
    
    QRegExp parse("^\\s*(\\d+)\\s*,\\s*(\\w+)\\s*(,\\s*common)?\\s*$");
    if(parse.indexIn(str) != 0)
      throw RuntimeError(QString("Invalid specification for time "
                                 "dependence: '%1'").arg(str));
    if(parse.cap(2) != "exp")
      throw RuntimeError(QString("Invalid type of time "
                                 "dependence: '%1'").arg(parse.cap(2)));
    
    type = Exponential;
    number = parse.cap(1).toInt();
    
    independentBits = parse.cap(3).isEmpty();
    
  };
  
  
};


/// Fits a full kinetic system.
class KineticSystemFit : public PerDatasetFit {

  /// System we're working on - deleted and recreated for each fit.
  KineticSystem * system;

  /// The evolver... Created and delete at the same time as each fit.
  KineticSystemEvolver * evolver;

  /// Index of the current's system index for the time. @hack Drop the
  /// mutable by moving parameter computation somewhere else.
  mutable int timeIndex;

  mutable QSet<int> skippedIndices;


  /// Wether or not this is fitting a voltammogram
  bool voltammogram;
  
  mutable int potentialIndex;

  mutable int faraIndex;

  mutable int temperatureIndex;
  

  /// These two parameters are needed for the proper computation of
  /// potentials in intermediate steps:
  double lastTime;
  double lastPot;
  double direction;             // ie algrebrized scan rate.

  /// Time-dependent parameters !
  /// A hash parameter index -> its time dependence
  QHash<int, TimeDependentParameter> timeDependentParameters;

  /// For now, a rather hackish substitute for callbacks with
  /// parameters
  const double * params;

  /// the base of the kinetic systems parameters;
  mutable int parametersBase;

  /// Base index for the time-dependent stuff. 
  mutable int tdBase;

  /// Parameter lists for the time-dependent stuff
  QList<ParameterDefinition> tdParameters;

  /// A hash time dependent parameter name -> index in 
  /// timeDependentParameter 
  QHash<QString, int> tdParameterNames;

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
    // Options get processed when everything else is done.

    QStringList lst;
    updateFromOptions(opts, "with", lst);

    voltammogram = false;
    updateFromOptions(opts, "voltammogram", voltammogram);

    potentialIndex = -1;
    temperatureIndex = -1;
    faraIndex = -1;

    int baseIndex = 0;
    processSoftOptions(opts);


    // Get rid of all time-dependent parameters first.
    tdParameters.clear();
    tdParameterNames.clear();
    timeDependentParameters.clear();

    for(int i = 0; i < lst.size(); i++) {
      Terminal::out << "Parsing spec: " << lst[i] << endl;
      QStringList s2 = lst[i].split(":");
      if(s2.size() != 2)
        throw RuntimeError("Time-dependent parameter '%1' "
                           "should contain a single :").
          arg(lst[i]);
      int idx = evolver->getParameterIndex(s2[0]);
      if(idx < 0)
        throw RuntimeError("Unknown parameter: %1").arg(s2[0]);
      timeDependentParameters[idx] = TimeDependentParameter();
      TimeDependentParameter & param = timeDependentParameters[idx];
      param.parseFromString(s2[1]);
      param.baseIndex = baseIndex;
      QList<ParameterDefinition> defs = param.parameters(s2[0]);
      baseIndex += defs.size();
      tdParameters += defs;
      tdParameterNames[s2[0]] = idx;
    }
  }

  void prepareFit(const QString & fileName)
  {
    delete system;
    system = NULL;
    delete evolver;
    evolver = NULL;             // Prevent segfault upon destruction
                                // if next step fails
    system = new KineticSystem;
    system->parseFile(fileName);

    /// @todo That should join KineticSystemEvolver ?
    system->prepareForTimeEvolution();
    evolver = new KineticSystemEvolver(system);
  }

  void runFit(const QString & name, QString fileName, 
              QList<const DataSet *> datasets,
              const CommandOptions & opts)
  {
    prepareFit(fileName);
    Fit::runFit(name, datasets, opts);
  }

  void runFitCurrentDataSet(const QString & n, 
                            QString fileName, const CommandOptions & opts)
  {
    QList<const DataSet *> ds;
    ds << soas().currentDataSet();
    runFit(n, fileName, ds, opts);
  }

  void computeFit(const QString & name, QString fileName,
                  QString params,
                  QList<const DataSet *> datasets,
                  const CommandOptions & opts)
  {
    prepareFit(fileName);
    Fit::computeFit(name, params, datasets, opts);
  }


  void dumpAllParameters()
  {
    // Dumps all the parameters to standard out
    QTextStream o(stdout);

    QHash<QString, double> vals = evolver->parameterValues();

    QStringList s = vals.keys();
    qSort(s);

    for(int i = 0; i < s.size(); i++)
      o << s[i] << " = " << vals[s[i]] << endl;

    QStringList p = system->allParameters();

    o << "There are " << timeDependentParameters.size() << " td-parameters" << endl;
    for(auto i = timeDependentParameters.begin(); i != timeDependentParameters.end(); i++) {
      o << "TD parameter for parameter index " << i.key()
        << " -> " << p[i.key()] << endl;
    }
  }

public:

  virtual CommandOptions currentSoftOptions() const {
    return evolver->getStepperOptions().asOptions();
  };

  virtual void processSoftOptions(const CommandOptions & opts) {
    // Parse ODEStepperOptions
    ODEStepperOptions op = evolver->getStepperOptions();
    op.fixed = false;           // Drop non-adaptative steps !
    op.parseOptions(opts);

    if(op.fixed) {
      // Decrease drastically the precision !
      op.epsAbs = 1e-2;
      op.epsRel = 1e-2;

      // By default, set the step size to 0 in that case 
      // (meaning make one step by data point)
      op.hStart = 0;
      op.parseOptions(opts);    // Parse again in case the step was
                                // set in the options
    }

    evolver->setStepperOptions(op);
    evolver->resetStepper();

    // Dump the options on the terminal ?
    Terminal::out << "Using integrator parameters: " 
                  << op.description() << endl;
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;

    if(voltammogram) {
      // add temperature and scan rate at the beginning
      defs << ParameterDefinition("temperature", true)
           << ParameterDefinition("v", true);
    }

    QStringList parameters = system->allParameters();
    timeIndex= -1;

    if(! system->reporterExpression) {
      QStringList species = system->allSpecies();
      for(int i = 0; i < species.size(); i++)
        defs << ParameterDefinition(QString("y_%1").
                                    arg(species[i]), i != 0);
    }
    
    parametersBase = defs.size();

    skippedIndices.clear();
    for(int i = 0; i < parameters.size(); i++) {
      if(parameters[i] == "t") {
        timeIndex = i; /// @hack modifying a mutable stuff. Move the
                       /// preparation of the parameter list in its
                       /// own function after options processing.
        skippedIndices.insert(i);
        continue;
      }
      if(tdParameterNames.contains(parameters[i])) {
        skippedIndices.insert(i);
        continue;
      }
      if(voltammogram && parameters[i] == "temperature") {
        temperatureIndex = i;
        skippedIndices.insert(i);
        continue;
      }
      if(voltammogram && parameters[i] == "e") {
        potentialIndex = i;
        skippedIndices.insert(i);
        continue;
      }
      if(voltammogram && parameters[i] == "fara") {
        faraIndex = i;
        skippedIndices.insert(i);
        continue;
      }
      defs << ParameterDefinition(parameters[i], 
                                  i < system->speciesNumber()); 
    }
    tdBase = defs.size();
    defs += tdParameters;
    return defs;
  };

  static void timeDependentRates(double * params, double t, void * p)
  {
    KineticSystemFit * fit = reinterpret_cast<KineticSystemFit *>(p);
    if(fit->timeIndex >= 0)
      params[fit->timeIndex] = t;

    for(QHash<int, TimeDependentParameter>::const_iterator i = 
          fit->timeDependentParameters.begin(); 
        i != fit->timeDependentParameters.end(); i++)
      params[i.key()] = i.value().computeValue(t, fit->params);

    if(fit->voltammogram && fit->potentialIndex >= 0) {
      params[fit->potentialIndex] = fit->lastPot + 
        fit->direction * (t - fit->lastTime);
    }
  }

  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target)
  {
    evolver->resetStepper();

    double sr = 0;

    evolver->setParameters(a + parametersBase,
                           skippedIndices);

    if(voltammogram) {
      double temp = a[0];
      if(temperatureIndex >= 0)
        evolver->setParameter(temperatureIndex, temp);
      if(faraIndex >= 0)
        evolver->setParameter(faraIndex, 
                              GSL_CONST_MKSA_FARADAY/ 
                              (a[0] * GSL_CONST_MKSA_MOLAR_GAS));
      sr = a[1];
    }

    evolver->setupCallback(KineticSystemFit::timeDependentRates, this);
    const Vector & xv = ds->x();
    evolver->initialize(voltammogram ? 0 : xv[0]);
    params = a + tdBase;

    if(data->debug)
      dumpAllParameters();

    Vector discontinuities;
    for(QHash<int, TimeDependentParameter>::const_iterator i = 
          timeDependentParameters.begin(); 
        i != timeDependentParameters.end(); i++)
      discontinuities << i.value().discontinuities(params);
    qSort(discontinuities);

    direction = xv[1] > xv[0] ? sr : -sr;
    lastTime = 0;
    lastPot = xv[0];
    for(int i = 0; i < xv.size(); i++) {
      // Here, we must be wary of the discontinuity points (ie those
      // of the time-evolving stuff)

      if(voltammogram && i > 0) {
        double pot = xv[i];
        if((pot - lastPot) * direction < 0) // at change of direction
          direction = -direction;
        lastTime += (pot - lastPot) / direction;
        lastPot = pot;
      }
      double tg = (voltammogram ? lastTime : xv[i]);

      if(discontinuities.size() > 0) {
        double td = discontinuities[0];
        if(evolver->currentTime() < td && tg > td) {
          // Make a first step to the discontinuity
          evolver->stepTo(td);
          discontinuities.remove(0);
        }
      }

      evolver->stepTo(tg);
      double val = 0;
      const double * cv = evolver->currentValues();
      if(system->reporterExpression)
        val = evolver->reporterValue();
      else {
        for(int j = 0; j < system->speciesNumber(); j++)
          val += cv[j] * a[j + (voltammogram ? 2 : 0)];
      }
      gsl_vector_set(target, i, val);
    }

    if(data->debug) {
      QTextStream o(stdout);
      o << "Number of evaluations: " << evolver->evaluations << endl;
    }
      

  };

  virtual void initialGuess(FitData * params, 
                            const DataSet *ds,
                            double * a)
  {
    const Vector & x = ds->x();
    const Vector & y = ds->y();

    if(voltammogram) {
      a[0] = soas().temperature();
      a[1] = 0.02;
    }

    int nb = system->speciesNumber();
    if(! system->reporterExpression) {
      // All currents to 0 but the first
      for(int i = 0; i < nb; i++)
        a[i + (voltammogram ? 2 : 0)] = (i == 0 ? y.max() : 0);
    }

    double * b = a + parametersBase;

    // All initial concentrations to 0 but the first
    for(int i = 0; i < nb; i++)
      b[i] = (i == 0 ? 1 : 0);
    
    // All rate constants and other to 1 ?
    for(int i = nb + parametersBase; i < params->parameterDefinitions.size();
        i++)
      a[i] = 1;                 // Simple, heh ?


    // And have the parameters handle themselves:
    for(QHash<int, TimeDependentParameter>::const_iterator i = 
          timeDependentParameters.begin(); 
        i != timeDependentParameters.end(); i++)
      i.value().setInitialGuess(a + tdBase, ds);

  };

  virtual ArgumentList * fitArguments() const {
    return new 
      ArgumentList(QList<Argument *>()
                   << new FileArgument("system", 
                                       "System",
                                       "Path to the file describing the "
                                       "system"));
  };

  virtual ArgumentList * fitSoftOptions() const {
    return new ArgumentList(ODEStepperOptions::commandOptions());
  };

  virtual ArgumentList * fitHardOptions() const {
    return new 
      ArgumentList(QList<Argument *>()
                   << new SeveralStringsArgument("with", 
                                                 "Time dependent parameters",
                                                 "Dependency upon time of "
                                                 "various parameters")
                   << new BoolArgument("voltammogram", 
                                       "If on, x is not taken to be time, but "
                                       "potential in a voltammetric experiment")
                   );
  };

  KineticSystemFit() :
    PerDatasetFit("kinetic-system", 
                  "Full kinetic system",
                  "", 1, -1, false), system(NULL), evolver(NULL)
  { 
    makeCommands(NULL, 
                 effector(this, &KineticSystemFit::runFitCurrentDataSet, true),
                 effector(this, &KineticSystemFit::runFit, true),
                 NULL,
                 effector(this, &KineticSystemFit::computeFit)
                 );
  };
};

static KineticSystemFit fit_kinetic_system;
