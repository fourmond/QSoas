/*
  rubyodesolver.cc: implementation of the ruby-based ODE solver
  Copyright 2012, 2013 by CNRS/AMU

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
#include <rubyodesolver.hh>

#include <odefit.hh>


#include <utils.hh>
#include <soas.hh>
#include <dataset.hh>

#include <commandeffector-templates.hh>
#include <file-arguments.hh>

#include <file.hh>

RubyODESolver::RubyODESolver(const QString & init, const QString & der) :
  initialization(NULL), derivatives(NULL), reporters(NULL)
{
  parseSystem(init, der);
}

RubyODESolver::RubyODESolver() : 
  initialization(NULL), derivatives(NULL), reporters(NULL)
{
}


void RubyODESolver::setParameterValues(const QString & formula)
{
  Expression::setParametersFromExpression(extraParams, formula, 
                                          extraParamsValues.data());
}

void RubyODESolver::parseFromFile(const QString & file)
{
  File f(file, File::TextRead);
  parseFromFile(f);
}


void RubyODESolver::parseFromFile(QIODevice * file)
{
  QStringList lines = Utils::parseConfigurationFile(file);
  QRegExp blnk("^\\s*$");

  QList<QStringList> sections = Utils::splitOn(lines, blnk);
  if(sections.size() < 2)
    throw RuntimeError("File does not contain two sections "
                       "separated by a fully blank line");
  
  parseSystem(sections[0].join("\n"), 
              sections[1].join("\n"), 
              (sections.size() > 2 ? sections[2].join("\n") : QString()) );
}

void RubyODESolver::parseSystem(const QString & init, const QString & der,
                                const QString & rep)
{
  delete initialization;
  initialization = NULL;
  delete derivatives;
  derivatives = NULL;
  delete reporters;
  reporters = NULL;

  // First, we look at the initialization to find out which variables
  // are necessary.

  // Initialization can be a function of t.

  QStringList statements = init.split(QRegExp("[;\n]"));
  QRegExp assignRE("\\s*(\\w+)\\s*=");
  for(int i = 0; i < statements.size(); i++)
    if(assignRE.indexIn(statements[i]) >= 0)
      vars << assignRE.cap(1);

  Utils::makeUnique(vars);
  // Should be fine.

  QString code = QString("%2\n[%1]").arg(vars.join(",")).arg(init);

  QStringList allv;
  allv << "t" << Expression::variablesNeeded(init)
       << Expression::variablesNeeded(der, vars);

  if(! rep.isEmpty())
    allv << Expression::variablesNeeded(rep, vars);

  Utils::makeUnique(allv);

  extraParams = allv;
  extraParams.takeAt(0);

  initialization = new Expression(code, allv);


  // Now, we prepare the derivatives
  QStringList derivs;
  QStringList inits;
  for(int i = 0; i < vars.size(); i++) {
    QString n = QString("d_%1").arg(vars[i]);
    derivs << n;
    inits << QString("%1 = 0").arg(n);
  }
  code = QString("%2\n%3\n[%1]").arg(derivs.join(",")).
    arg(inits.join("\n")).arg(der);

  allv << vars;
  derivatives = new Expression(code, allv);

  if(! rep.isEmpty())
    reporters = new Expression(rep, allv);

  // We initialize all the extram parameters to 0
  extraParamsValues = Vector(extraParams.size(), 0);
}


Vector RubyODESolver::reporterValues() const
{
  const double * vals = currentValues();
  if(reporters) {
    double v[vars.size() + extraParams.size() + 1];
    int j = 0;
    v[j++] = currentTime();
    for(int i = 0; i < extraParams.size(); i++)
      v[j++] = extraParamsValues[i];
    for(int i = 0; i < vars.size(); i++)
      v[j++] = vals[i];

    return reporters->evaluateAsArray(v);
  }

  Vector s;
  for(int i = 0; i < vars.size(); i++)
    s << vals[i];
  return s;
}

int RubyODESolver::dimension() const
{
  return vars.size();
}

int RubyODESolver::computeDerivatives(double t, const double * y, 
                                      double * dydt)
{
  if(callback != NULL)          // Tweak the time-dependent parameters
                                // when applicable
    callback(t, extraParamsValues.data());

  double v[vars.size() + extraParams.size() + 1];
  int j = 0;
  v[j++] = t;
  for(int i = 0; i < extraParams.size(); i++)
    v[j++] = extraParamsValues[i];
  for(int i = 0; i < vars.size(); i++)
    v[j++] = y[i];

  derivatives->evaluateIntoArray(v, dydt, vars.size());
  
  return GSL_SUCCESS;
}

RubyODESolver::~RubyODESolver()
{
  delete initialization;
  delete derivatives;
}

void RubyODESolver::initialize(double t)
{
  double tg[vars.size()];
  double v[extraParams.size() + 1];
  int j = 0;
  v[j++] = t;
  for(int i = 0; i < extraParams.size(); i++)
    v[j++] = extraParamsValues[i];
  initialization->evaluateIntoArray(v, tg, vars.size());
  ODESolver::initialize(tg, t);
}

void RubyODESolver::dump(QTextStream & target) const
{
  target << "Variables: " << vars.join(", ") << "\n"
         << "Extra parameters: " << extraParams.join(", ") << "\n\n"
         << "Initial conditions: \n" << initialization->formula() << "\n\n"
         << "Derivatives: \n" << derivatives->formula() << "\n\n";
  if(reporters)
    target << "Reporters: \n" << reporters->formula() << endl;
  else
    target << "No reporters" << endl;
}

QString RubyODESolver::dump() const
{
  QString tg;
  QTextStream s(&tg);
  dump(s);
  return tg;
}

void RubyODESolver::setParameterValues(const double * source, 
                                       const QSet<int> & skipped)
{
  int j = 0;
  for(int i = 0; i < extraParamsValues.size(); i++)
    if(! skipped.contains(i))
      extraParamsValues[i] = source[j++];
}

void RubyODESolver::setupCallback(const std::function <void (double, double *)> & cb)
{
  callback = cb;
}


//////////////////////////////////////////////////////////////////////

// Now, the implementation of Ruby-based ODE fits

/// Fits a full kinetic system.
class RubyODEFit : public ODEFit {
protected:

  /// The system, for defined fits
  RubyODESolver * mySystem;

  class Storage : public ODEFit::Storage {
  public:
    /// The ODE system
    RubyODESolver * system;

    /// whether this object owns the system
    bool ownSystem;
    
    /// The file name (unsure that's the best place)
    QString fileName;

    Storage() : system(NULL), fileName("??") {
      ownSystem = true;
    };
    
    virtual ~Storage() {
      if(ownSystem) {
        delete system;
      }
    };
  };

  RubyODESolver * getSystem(FitData * data) const {
    if(mySystem)
      return mySystem;
    Storage * s = storage<Storage>(data);
    return s->system;
  };
    
  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const override {
    Storage * s = deepCopy<Storage>(source);

    // We copy the POINTER, so the target should not free the system
    // upon deletion.
    s->ownSystem = false;
    
    return s;
  };


  virtual ODESolver * solver(FitData * data) const override {
    return getSystem(data);
  };

  virtual int getParameterIndex(const QString & name, FitData * data) const  override {
    RubyODESolver * system = getSystem(data);
    QStringList lst = system->extraParameters();
    return lst.indexOf(name);
  };

  virtual QStringList systemParameters(FitData * data) const override {
    RubyODESolver * system = getSystem(data);
    return system->extraParameters();
  };

  virtual QStringList variableNames(FitData * data) const override {
    RubyODESolver * system = getSystem(data);
    return system->variables();
  };

    virtual void initialize(double t0, const double * params, FitData * data) const override {
    Storage * s = storage<Storage>(data);
    RubyODESolver * system = getSystem(data);
    system->setParameterValues(params + s->parametersBase,
              s->skippedIndices);
    system->initialize(t0);
  };

  virtual bool hasReporters(FitData * data) const override {
    RubyODESolver * system = getSystem(data);
    return system->hasReporters();
  };

  virtual double reporterValue(FitData * data) const {
    RubyODESolver * system = getSystem(data);
    Vector v = system->reporterValues();
    // Only returns the first reporter.
    return v[0];
  };
  
  virtual void setupCallback(const std::function<void (double, double * )> & cb, FitData * data) const override {
    RubyODESolver * system = getSystem(data);
    system->setupCallback(cb);
  };
    
    
    
protected:
    
    
  void prepareFit(const QString & fn, FitData * data) const 
  {
    Storage * s = storage<Storage>(data);
    delete s->system;           // In principle, it should always be NULL
    s->system = new RubyODESolver;
    s->fileName = fn;
    s->system->parseFromFile(s->fileName);
    if(s->system->extraParameters().size() == 0)
      throw RuntimeError("There are no extra parameters in file '%1', nothing to fit !").arg(s->fileName);
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
  },name, params, datasets, opts);
  }

  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString("(system: %1)").arg(s->fileName);
  };


  
public:

  virtual void initialGuess(FitData * data, 
                            const DataSet *ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);
    RubyODESolver * system = getSystem(data);
    const Vector & x = ds->x();
    const Vector & y = ds->y();


    int nb = system->variables().size();
    if(! system->hasReporters()) {
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
    for(int i = s->parametersBase; i < s->parametersNumber; i++)
      a[i] = 1;                 // Simple, heh ?

    // And have the parameters handle themselves:
    s->timeDependentParameters.setInitialGuesses(a + s->tdBase, ds);
  };

  virtual ArgumentList fitArguments() const override {
    if(mySystem)
      return ArgumentList();
    return 
      ArgumentList(QList<Argument *>()
                   << new FileArgument("system", 
                                       "System",
                                       "Path to the file describing the "
                                       "ODE system"));
  };

  RubyODEFit() :
    ODEFit("ode", 
           "Fit an ODE system",
           "", 1, -1, false), mySystem(NULL)
  { 
    makeCommands(ArgumentList(), 
                 effector(this, &RubyODEFit::runFitCurrentDataSet, true),
                 effector(this, &RubyODEFit::runFit, true),
                 ArgumentList(),
                 effector(this, &RubyODEFit::computeFit)
                 );
  };

  RubyODEFit(const QString & name, 
             RubyODESolver * sys,
             const QString & file
             ) : 
    ODEFit(name.toLocal8Bit(), 
           QString("Kinetic system of %1").arg(file).toLocal8Bit(),
           "", 1, -1, false)
  {
    mySystem = sys;
    makeCommands();
  }
};

static RubyODEFit fit_ruby_ode;

//////////////////////////////////////////////////////////////////////
