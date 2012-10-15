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

// Here come fits !
#include <perdatasetfit.hh>
#include <fitdata.hh>


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

void KineticSystemEvolver::setParameters(const double * source, int skip)
{
  int j = 0;
  for(int i = 0; i < parameterIndex.size(); i++)
    if(i != skip)
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


//////////////////////////////////////////////////////////////////////

static ArgumentList 
ksArgs (QList<Argument *>() 
        << new FileArgument("reaction-file", 
                           "File describing the kinetic system")
        << new ParametersHashArgument("parameters", 
                                     "Parameters of the model")
        );

static ArgumentList 
ksOpts (QList<Argument *>() 
        << new NumberArgument("start", 
                              "The starting time")
        << new NumberArgument("end", 
                              "The ending time")
        << new IntegerArgument("number", 
                              "Number of steps")
        << new BoolArgument("dump", 
                            "If on, dumps the system rather than solving")
        );

/// Just replace the time in the expression by its value !
static void setTime(double * params, double t, void * p)
{
  int idx = *((int*)p);
  params[idx] = t;
}

static void kineticSystemCommand(const QString &, QString file,
                                 QHash<QString, double> parameters,
                                 const CommandOptions & opts)
{
  double start = 0;
  double end = 10;
  int nb = 100;

  bool dump = false;
  updateFromOptions(opts, "dump", dump);
  updateFromOptions(opts, "number", nb);
  updateFromOptions(opts, "end", end);
  updateFromOptions(opts, "start", start);
  
  KineticSystem sys; 
  sys.parseFile(file);
  sys.prepare();
  if(dump) {
    Terminal::out << "System " << file << ":\n" 
                  << sys.toString() << endl;
    Terminal::out << "System parameters: " 
                  << sys.allParameters().join(", ") << endl;
    return;
  }
  
  KineticSystemEvolver evolver(&sys);
  QStringList missingParams = evolver.setParameters(parameters);
  if(missingParams.size() > 0)
    Terminal::out << "Missing parameters: " << missingParams.join(", ") 
                  << endl;

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
  
  evolver.initialize(start);
  Vector tValues = Vector::uniformlySpaced(start, end, nb);
  QList<Vector> concentrations = evolver.steps(tValues);
  concentrations.insert(0, tValues);
  DataSet * nds = new DataSet(concentrations);
  nds->name = QString("ks-%1.dat").arg(QFileInfo(file).fileName());
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
class KineticSystemFit : public PerDatasetFit {

  /// System we're working on - deleted and recreated for each fit.
  KineticSystem * system;

  /// Index of the current's system index for the time
  mutable int timeIndex;
  

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
    // For now, no extra stuff !
  }

  void runFit(const QString & name, QString fileName, 
              QList<const DataSet *> datasets,
              const CommandOptions & opts)
  {
    delete system;
    system = new KineticSystem;
    system->parseFile(fileName);
    system->prepare();
    Fit::runFit(name, datasets, opts);
  }

  void runFitCurrentDataSet(const QString & n, 
                            QString fileName, const CommandOptions & opts)
  {
    QList<const DataSet *> ds;
    ds << soas().currentDataSet();
    runFit(n, fileName, ds, opts);
  }


public:

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;

    QStringList parameters = system->allParameters();
    timeIndex= -1;

    QStringList species = system->allSpecies();
    for(int i = 0; i < species.size(); i++)
      defs << ParameterDefinition(QString("i_%1").
                                  arg(species[i]), i != 0);

    for(int i = 0; i < parameters.size(); i++) {
      if(parameters[i] == "t") {
        timeIndex = i;
        continue;
      }
      defs << ParameterDefinition(parameters[i], 
                                  i < system->speciesNumber()); 
    }
    return defs;
  };

  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target)
  {
    KineticSystemEvolver evolver(system);
    evolver.setParameters(a + system->speciesNumber(), 
                          timeIndex);
    const Vector & xv = ds->x();
    evolver.initialize(xv[0]);

    // @todo TIME !

    for(int i = 0; i < xv.size(); i++) {
      evolver.stepTo(xv[i]);
      double val = 0;
      const double * cv = evolver.currentValues();
      for(int j = 0; j < system->speciesNumber(); j++)
        val += cv[j] * a[j];
      gsl_vector_set(target, i, val);
    }

  };

  virtual void initialGuess(FitData * params, 
                            const DataSet *ds,
                            double * a)
  {
    const Vector & x = ds->x();
    const Vector & y = ds->y();

    int nb = system->speciesNumber();

    // All currents to 0 but the first
    for(int i = 0; i < nb; i++)
      a[i] = (i == 0 ? y.max() : 0);

    // All initial concentrations to 0 but the first
    for(int i = 0; i < nb; i++)
      a[i + nb] = (i == 0 ? 1 : 0);
    
    // All rate constants and other to 1 ?
    for(int i = 2*nb; i < params->parameterDefinitions.size(); i++)
      a[i] = 1;                 // Simple, heh ?

  };

  KineticSystemFit() :
    PerDatasetFit("kinetic-system", 
                  "Full kinetic system",
                  "", 1, -1, false) 
  { 

    ArgumentList * al = new 
      ArgumentList(QList<Argument *>()
                   << new FileArgument("system", 
                                       "System",
                                       "Path to the file describing the "
                                       "system"));

    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   );

    makeCommands(al, 
                 effector(this, &KineticSystemFit::runFitCurrentDataSet, true),
                 effector(this, &KineticSystemFit::runFit, true),
                 opts);
  };
};

static KineticSystemFit fit_kinetic_system;
