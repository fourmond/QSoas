/*
  kineticsystemevolver.cc: implementation of KineticSystemEvolver
  Copyright 2012, 2013, 2014 by CNRS/AMU

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
#include <odefit.hh>
#include <fitdata.hh>

#include <debug.hh>

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
  if(callback != NULL)          // Tweak the time-dependent parameters
                                // when applicable
    callback(t, parameters);
  system->computeDerivatives(dydt, y, parameters);
  if(false) {                /// @todo Add real debugging facilities ?
    // Debugging information
    Debug::debug()
      << "Time t = " << t << "\n";
    for(int i = 0; i < system->speciesNumber(); i++)
      Debug::debug()
        << "y[" << i << "] = " << y[i] << "\t dy[" << i << "] = " 
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
  Utils::skippingCopy(source, parameters, parameterIndex.size(), skipped);
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


void KineticSystemEvolver::setupCallback(const std::function <void (double, double *)> & cb)
{
  callback = cb;
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
                            "if on, prints a description of the system rather than solving (default: false)")
        << ODEStepperOptions::commandOptions()
        << new BoolArgument("annotate", 
                            "Annotate",
                            "If on, a last column will contain the number of function evaluation for each step (default false)")
        );

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
    evolver.setupCallback([timeIndex](double t, double * params) {
        params[timeIndex] = t;
      });
  }
  QStringList p = params.keys();
  qSort(p);

  Terminal::out << "Parameter values: " << endl;
  for(int i = 0; i < p.size(); i++)
    Terminal::out << " * " << p[i] << " = " << params[p[i]] << endl;
  
  evolver.initialize(ds->x()[0]);
  // evolver.autoSetHMin(ds->x());
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

/// Fits a full kinetic system.
class KineticSystemFit : public ODEFit {
protected:
    /// The system, for defined fits
  KineticSystem * mySystem;

  /// The evolver... Created and delete at the same time as each fit.
  KineticSystemEvolver * myEvolver;


  class Storage : public ODEFit::Storage {
  public:
    /// The system
    KineticSystem * system;

    /// The evolver... Created and delete at the same time as each fit.
    KineticSystemEvolver * evolver;

    /// whether this object owns the system
    bool ownSystem;
    
    /// The file name (unsure that's the best place)
    QString fileName;

    Storage() : system(NULL), evolver(NULL) {
      ownSystem = true;
      fileName = "??";
    };
    
    virtual ~Storage() {
      if(ownSystem) {
        delete system;
        delete evolver;
      }
    };
    

  };

  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const {
    Storage * s = deepCopy<Storage>(source);

    // We copy the POINTER, so the target should not free the system
    // upon deletion.
    s->ownSystem = false;
    
    return s;
  };



  virtual QString optionsString(FitData * data) const {
    Storage * s = storage<Storage>(data);
    return QString("system: %1").arg(s->fileName);
  };

  KineticSystem * getSystem(FitData * data) const {
    if(mySystem)
      return mySystem;
    Storage * s = storage<Storage>(data);
    return s->system;
  };

  KineticSystemEvolver * getEvolver(FitData * data) const {
    if(myEvolver)
      return myEvolver;
    Storage * s = storage<Storage>(data);
    return s->evolver;
  };

  virtual ODESolver * solver(FitData * data) const {
    return getEvolver(data);
  };

  virtual int getParameterIndex(const QString & name, FitData * data) const  {
    KineticSystemEvolver * evolver = getEvolver(data);
    return evolver->getParameterIndex(name);
  };

  virtual QStringList systemParameters(FitData * data) const {
    KineticSystem * system = getSystem(data);
    return system->allParameters();
  };

  virtual QStringList variableNames(FitData * data) const {
    KineticSystem * system = getSystem(data);
    return system->allSpecies();
  };

  virtual bool isFixed(const QString & n, FitData * ) const {
    return n.startsWith("c0_");
  };

  virtual void initialize(double t0, const double * params, FitData * data) const {
    Storage * s = storage<Storage>(data);
    KineticSystemEvolver * evolver = getEvolver(data);

    evolver->setParameters(params + s->parametersBase, s->skippedIndices);
    evolver->initialize(t0);
  };

  virtual bool hasReporters(FitData * data) const {
    KineticSystem * system = getSystem(data);
    return system->reporterExpression;
  };

  virtual double reporterValue(FitData * data) const {
    KineticSystemEvolver * evolver = getEvolver(data);
    return evolver->reporterValue();
  };
  
  virtual void setupCallback(const std::function<void (double, double * )> & cb, FitData * data) const{
    KineticSystemEvolver * evolver = getEvolver(data);
    evolver->setupCallback(cb);
  };



protected:


  void prepareFit(const QString & fileName, FitData * data)
  {
    Storage * s = storage<Storage>(data);
    delete s->system;
    s->system = NULL;
    delete s->evolver;
    s->evolver = NULL;             // Prevent segfault upon destruction
                                // if next step fails
    s->system = new KineticSystem;
    s->system->parseFile(fileName);

    /// @todo That should join KineticSystemEvolver ?
    s->system->prepareForTimeEvolution();
    s->evolver = new KineticSystemEvolver(s->system);
  }

  void runFit(const QString & name, QString fileName, 
              QList<const DataSet *> datasets,
              const CommandOptions & opts)
  {
    Fit::runFit([this, fileName](FitData * data) {
        prepareFit(fileName, data);
      }, name, datasets, opts);
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
    Fit::computeFit([this, fileName](FitData * data) {
        prepareFit(fileName, data);
      }, name, params, datasets, opts);
  }


  
  void dumpAllParameters(FitData * data) const
  {
    Storage * s = storage<Storage>(data);
    KineticSystemEvolver * evolver = getEvolver(data);
    KineticSystem * system = getSystem(data);

    QHash<QString, double> vals = evolver->parameterValues();

    QStringList ps = vals.keys();
    qSort(ps);

    for(int i = 0; i < ps.size(); i++)
      Debug::debug()
        << ps[i] << " = " << vals[ps[i]] << endl;

    QStringList p = system->allParameters();

    Debug::debug()
      << "There are " << s->timeDependentParameters.size() << " td-parameters" << endl;
    for(auto i = s->timeDependentParameters.begin(); i != s->timeDependentParameters.end(); i++) {
      Debug::debug()
        << "TD parameter for parameter index " << i.key()
        << " -> " << p[i.key()] << endl;
    }
  }

public:

  virtual void initialGuess(FitData * data, 
                            const DataSet *ds,
                            double * a) const
  {
    Storage * s = storage<Storage>(data);
    KineticSystem * system = getSystem(data);

    const Vector & x = ds->x();
    const Vector & y = ds->y();


    int nb = system->speciesNumber();
    if(! system->reporterExpression) {
      // All currents to 0 but the first

      for(int i = 0; i < nb; i++)
        a[i + (s->voltammogram ? 2 : 0)] = (i == 0 ? y.max() : 0);
    }

    double * b = a + s->parametersBase;
    if(s->hasOrigTime)
      b[-1] = x[0] - 20;

    // All initial concentrations to 0 but the first
    for(int i = 0; i < nb; i++)
      b[i] = (i == 0 ? 1 : 0);
    
    // All rate constants and other to 1 ?

    // We can't use params->parameterDefinitions.size(), as this will
    // fail miserably in combined fits
    for(int i = nb + s->parametersBase; i < s->parametersNumber; i++)
      a[i] = 1;                 // Simple, heh ?

    // And have the parameters handle themselves:
    s->timeDependentParameters.setInitialGuesses(a + s->tdBase, ds);

  };

  virtual ArgumentList * fitArguments() const {
    if(mySystem)
      return NULL;
    return new 
      ArgumentList(QList<Argument *>()
                   << new FileArgument("system", 
                                       "System",
                                       "file describing the "
                                       "system"));
  };

  KineticSystemFit() :
    ODEFit("kinetic-system", 
                  "Full kinetic system",
           "", 1, -1, false), mySystem(NULL), myEvolver(NULL)
  { 
    makeCommands(NULL, 
                 effector(this, &KineticSystemFit::runFitCurrentDataSet, true),
                 effector(this, &KineticSystemFit::runFit, true),
                 NULL,
                 effector(this, &KineticSystemFit::computeFit)
                 );
  };

  KineticSystemFit(const QString & name, 
                   KineticSystem * sys,
                   const QString & file
                   ) : 
    ODEFit(name.toLocal8Bit(), 
                  QString("Kinetic system of %1").arg(file).toLocal8Bit(),
                  "", 1, -1, false)
  {
    mySystem = sys;
    mySystem->prepareForTimeEvolution();
    myEvolver = new KineticSystemEvolver(mySystem);
    makeCommands();
  }
};

static KineticSystemFit fit_kinetic_system;

//////////////////////////////////////////////////////////////////////

static ArgumentList 
dLWFArgs(QList<Argument *>() 
         << new FileArgument("file", 
                             "System",
                              "System to load")
         << new StringArgument("name", 
                               "Name",
                               "Name of the newly created fit")
         );


static ArgumentList 
dkfsOpts(QList<Argument *>() 
         << new BoolArgument("redefine", 
                             "Redefine",
                             "If the fit already exists, redefines it")
       );

static void defineKSFitCommand(const QString &, QString file, 
                               QString name, const CommandOptions & opts)
{
  bool overwrite = false;
  updateFromOptions(opts, "redefine", overwrite);
  Fit::safelyRedefineFit(name, overwrite);

  /// @todo exception safe (ie guarded pointer detached in the end)
  KineticSystem * ks = new KineticSystem;
  ks->parseFile(file);

  new KineticSystemFit(name, ks, file);
}


static Command 
dlwf("define-kinetic-system-fit",  // command name
     effector(defineKSFitCommand), // action
     "fits",                       // group name
     &dLWFArgs,                    // arguments
     &dkfsOpts,                    // options
     "Define a fit based on a kinetic mode",
     "Defines a new fit based on kinetic model",
     "...");
