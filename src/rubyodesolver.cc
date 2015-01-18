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
  QFile f(file);
  Utils::open(&f, QIODevice::ReadOnly);
  parseFromFile(&f);
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

  /// The ODE system
  RubyODESolver * system;

  virtual ODESolver * solver() const {
    return system;
  };

  virtual int getParameterIndex(const QString & name) const  {
    QStringList lst = system->extraParameters();
    return lst.indexOf(name);
  };

  virtual QStringList systemParameters() const {
    return system->extraParameters();
  };

  virtual QStringList variableNames() const {
    return system->variables();
  };

  virtual void initialize(double t0, const double * params) {
    system->setParameterValues(params + parametersBase, skippedIndices);
    system->initialize(t0);
  };

  virtual bool hasReporters() const {
    return system->hasReporters();
  };

  virtual double reporterValue() const {
    Vector v = system->reporterValues();
    // Only returns the first reporter.
    return v[0];
  };
  
  virtual void setupCallback(const std::function<void (double, double * )> & cb) {
    system->setupCallback(cb);
  };



protected:


  void prepareFit(const QString & fileName)
  {
    delete system;
    system = new RubyODESolver;
    system->parseFromFile(fileName);
    if(system->extraParameters().size() == 0)
      throw RuntimeError("There are no extra parameters in file '%1', nothing to fit !").arg(fileName);
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


  
public:

  virtual void initialGuess(FitData * /*params*/, 
                            const DataSet *ds,
                            double * a)
  {
    const Vector & x = ds->x();
    const Vector & y = ds->y();


    int nb = system->variables().size();
    if(! system->hasReporters()) {
      // All currents to 0 but the first
      for(int i = 0; i < nb; i++)
        a[i + (voltammogram ? 2 : 0)] = (i == 0 ? y.max() : 0);
    }

    double * b = a + parametersBase;
    if(hasOrigTime)
      b[-1] = x[0] - 20;

    // All initial concentrations to 0 but the first
    for(int i = 0; i < nb; i++)
      b[i] = (i == 0 ? 1 : 0);
    
    // All rate constants and other to 1 ?

    // We can't use params->parameterDefinitions.size(), as this will
    // fail miserably in combined fits
    for(int i = nb + parametersBase; i < parametersNumber; i++)
      a[i] = 1;                 // Simple, heh ?

    // And have the parameters handle themselves:
    timeDependentParameters.setInitialGuesses(a + tdBase, ds);
  };

  virtual ArgumentList * fitArguments() const {
    if(system)
      return NULL;
    return new 
      ArgumentList(QList<Argument *>()
                   << new FileArgument("system", 
                                       "System",
                                       "Path to the file describing the "
                                       "ODE system"));
  };

  RubyODEFit() :
    ODEFit("ode", 
           "Fit an ODE system",
           "", 1, -1, false), system(NULL)
  { 
    makeCommands(NULL, 
                 effector(this, &RubyODEFit::runFitCurrentDataSet, true),
                 effector(this, &RubyODEFit::runFit, true),
                 NULL,
                 effector(this, &RubyODEFit::computeFit)
                 );
  };

  RubyODEFit(const QString & name, 
             RubyODESolver * sys,
             const QString & file
             ) : 
    ODEFit(name.toLocal8Bit(), 
           QString("Kinetic system of %1").arg(file).toLocal8Bit(),
           "", 1, -1, false), system(sys)
  {
    makeCommands();
  }
};

static RubyODEFit fit_ruby_ode;

//////////////////////////////////////////////////////////////////////
