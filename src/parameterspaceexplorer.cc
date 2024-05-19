/*
  parameterspaceexplorer.cc: implementation of the style generators
  Copyright 2013 by Vincent Fourmond

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
#include <parameterspaceexplorer.hh>

#include <exceptions.hh>

#include <commandeffector.hh>
#include <commandeffector-templates.hh>
#include <commandcontext.hh>
#include <command.hh>

#include <fitdata.hh>
#include <fitworkspace.hh>
#include <fittrajectory.hh>

#include <file.hh>

#include <terminal.hh>



double ParameterRangeSpec::center() const
{
  if(log) {
    return sqrt(low * high);
  }
  else
    return 0.5*(low + high);
}

double ParameterRangeSpec::sigma() const
{
  if(log)
    return 0.5 * (log10(high) - log10(low));
  else
    return 0.5 * (high - low);
}

double ParameterRangeSpec::trim(double val) const
{
  if(std::isnan(val))
    return center();          // Safety catch
  return std::max(std::min(val, high), low);
}


double ParameterRangeSpec::distance(double x1, double x2) const
{
  if(log)
    return fabs(log10(x2/x1)/log10(high/low));
  return fabs((x1 - x2)/(high - low));
}




/// Parses the parameter list
QList<ParameterRangeSpec> ParameterRangeSpec::parseSpecs(const QStringList & specs,
                                               FitWorkspace * workSpace,
                                               QStringList * unknowns) {
  QList<ParameterRangeSpec> parameterSpecs;
  QRegExp re("^\\s*(.*):(u,)?([^:]+)\\.\\.([^:,]+)(,log)?\\s*$");

  for(const QString & s : specs) {
    if(re.indexIn(s) != 0)
      throw RuntimeError("Invalid parameter specification: '%1'").
        arg(s);


    // Now handling:
    // monte-carlo-explorer tau_1[#0,#1],tau_2[#1]:1e-2..1e2,log
      
    QStringList pars = Utils::nestedSplit(re.cap(1), ',', "[", "]");
    bool uniform = ! re.cap(2).isEmpty();
    double l = re.cap(3).toDouble();
    double h = re.cap(4).toDouble();
    bool log = ! re.cap(5).isEmpty();
    QList<QPair<int, int> > params;
    for(const QString & pa : pars)
      params << workSpace->parseParameterList(pa, unknowns);
    for(const QPair<int, int> & p : params) {
      ParameterRangeSpec sp = {p, l, h, log, uniform, 0};
      parameterSpecs << sp;
    }
  }
  return parameterSpecs;
};


double ParameterRangeSpec::trajectoryDistance(const QList<ParameterRangeSpec> & specs,
                                              const FitTrajectory & a,
                                              const FitTrajectory & b,
                                              const FitWorkspace * workspace,
                                              bool useFinal)
{
  const Vector & ap = useFinal ? a.finalParameters : a.initialParameters;
  const Vector & bp = useFinal ? b.finalParameters : b.initialParameters;
  return trajectoryDistance(specs, ap, bp, workspace);
}

double ParameterRangeSpec::trajectoryDistance(const QList<ParameterRangeSpec> & specs,
                                              const Vector & ap,
                                              const Vector & bp,
                                              const FitWorkspace * workspace)
{
  double rv = 0;
  int nbParams = workspace->parametersPerDataset();
  int nbDatasets = workspace->datasetNumber();

  for(const ParameterRangeSpec & spec : specs) {
    if(spec.parameter.second == -1) {
      for(int i = 0; i < nbDatasets; i++)
        rv += pow(spec.distance(ap[i*nbParams + spec.parameter.first],
                                bp[i*nbParams + spec.parameter.first]), 2);
    }
    else
      rv += pow(spec.distance(ap[spec.parameter.second*nbParams +
                                 spec.parameter.first],
                              bp[spec.parameter.second*nbParams +
                                 spec.parameter.first]), 2);
  }
  return pow(rv, 0.5);
}

Vector ParameterRangeSpec::averageParameters(const QList<ParameterRangeSpec> & specs,
                                             const QList<Vector> & parameters,
                                             const FitWorkspace * workspace)
{
  if(parameters.size() == 0)
    throw InternalError("Should have at least one element");

  Vector rv(parameters.first().size(), 0);
  int nbParams = workspace->parametersPerDataset();
  int nbDatasets = workspace->datasetNumber();
  int nb = parameters.size();

  for(const ParameterRangeSpec & spec : specs) {
    if(spec.parameter.second == -1) {
      for(int i = 0; i < nbDatasets; i++) {
        int idx = i*nbParams + spec.parameter.first;
        for(const Vector & s: parameters) {
          if(spec.log)
            rv[idx] += log10(s[idx]);
          else
            rv[idx] += s[idx];
        }
        rv[idx] /= nb;
        if(spec.log)
          rv[idx] = pow(10.0, rv[idx]);
      }
    }
    else {
      int idx = spec.parameter.second*nbParams + spec.parameter.first;
      for(const Vector & s: parameters) {
        if(spec.log)
          rv[idx] += log10(s[idx]);
        else
          rv[idx] += s[idx];
      }
      rv[idx] /= nb;
      if(spec.log)
        rv[idx] = pow(10.0, rv[idx]);
    }
  }
  return rv;
}



//////////////////////////////////////////////////////////////////////

ParameterSpaceExplorerFactoryItem::
ParameterSpaceExplorerFactoryItem(const QString & n,
                                  const QString & pn,
                                  const ArgumentList & args,
                                  const ArgumentList & opts,
                                  ParameterSpaceExplorerFactoryItem::Creator c)  :
  Factory(n, c, pn), publicName(description)
{
  QString cmdName = n + "-explorer";
  cmd = new Command(cmdName,
                    ParameterSpaceExplorer::explorerEffector(n), "fits",
                    args, opts,
                    pn,
                    "", "",
                    CommandContext::fitContext());

}

//////////////////////////////////////////////////////////////////////

ParameterSpaceExplorer::ParameterSpaceExplorer(FitWorkspace * ws) :
  workSpace(ws), linearPreFit(false), createdFrom(NULL)
{

  int nbds = workSpace->datasetNumber();
  int nbp = workSpace->parametersPerDataset();

  // Saving the initial state
  for(int i = 0; i < nbds; i++) {
    savedWeights << workSpace->getBufferWeight(i);
    for(int j = 0; j < nbp; j++)
      savedFixed << workSpace->isFixed(j, i);
  }
}

ParameterSpaceExplorer::~ParameterSpaceExplorer()
{
}


void ParameterSpaceExplorer::enableBuffer(int ds, bool enable)
{
  int nbp = workSpace->parametersPerDataset();
  if(enable) {
    workSpace->setBufferWeight(ds, savedWeights[ds]);
    for(int i = 0; i < nbp; i++) {
      if(! workSpace->isGlobal(i)) {
        bool state = savedFixed[ds * nbp + i];
        if(state != workSpace->isFixed(i, ds))
          workSpace->setFixed(i, ds, state);
      }
    }
  }
  else {
    workSpace->setBufferWeight(ds, 0);
    for(int i = 0; i < nbp; i++) {
      if(! workSpace->isGlobal(i) && (!workSpace->isFixed(i, ds)))
        workSpace->setFixed(i, ds, true);
    }
  }
}

ParameterSpaceExplorerFactoryItem * ParameterSpaceExplorer::namedFactoryItem(const QString & name)
{
  return
    static_cast<ParameterSpaceExplorerFactoryItem
                *>(ParameterSpaceExplorerFactoryItem::namedItem(name));
}

ParameterSpaceExplorer *
ParameterSpaceExplorer::createExplorer(const QString & name, 
                                       FitWorkspace * ws)
{
  ParameterSpaceExplorer * expl =
    ParameterSpaceExplorerFactoryItem::createObject(name, ws);
  expl->createdFrom = namedFactoryItem(name);
  return expl;
}

QHash<QString, QString> ParameterSpaceExplorer::availableExplorers()
{
  return ParameterSpaceExplorerFactoryItem::availableDescriptions();
}

bool ParameterSpaceExplorer::runHooks() const
{
  for(const std::function< bool()> &h : preFitHooks) {
    if(! h())
      return false;
  }
  if(linearPreFit) {
    Terminal::out << "Running a linear pre fit" << endl;
    int nb = workSpace->findLinearParameters(NULL).size();;
    Terminal::out << " -> " << nb << " parameters adjusted" << endl;
  }
  return true;
}

void ParameterSpaceExplorer::addHook(const std::function<bool ()> & func)
{
  preFitHooks << func;
}

void ParameterSpaceExplorer::clearHooks()
{
  preFitHooks.clear();
}


CommandEffector * ParameterSpaceExplorer::explorerEffector(const QString & n)
{
  return new RawCommandEffector([n](const QString & commandName, 
                                    const CommandArguments & arguments,
                                    const CommandOptions & options) {
                                  FitWorkspace * ws =
                                    FitWorkspace::currentWorkspace();
                                  ParameterSpaceExplorer * expl =
                                    createExplorer(n, ws);
                                  ws->setExplorer(expl);
                                  expl->setup(arguments, options);
                                });
}

void ParameterSpaceExplorer::writeParametersVector(const Vector & parameters) const
{
  QStringList names = workSpace->parameterNames();
  int nbp = workSpace->parametersPerDataset();
  int nbds = workSpace->datasetNumber();
  for(int i = 0; i < nbp; i++) {
    if(workSpace->isGlobal(i))
      Terminal::out << " * " << names[i] << " = " << parameters[i] << endl;
    else {
      for(int ds = 0; ds < nbds; ds++)
        Terminal::out << " * " << names[i] << "[#"
                      << ds << "] = " << parameters[i + ds * nbp] << endl;
    }
  }
}

//////////////////////////////////////////////////////////////////////

#include <file-arguments.hh>
#include <general-arguments.hh>
#include <soas.hh>
#include <commandwidget.hh>



static void iterateExplorerCommand(const QString & /*name*/,
                                   const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  ParameterSpaceExplorer * explorer = ws->currentExplorer;
  if(! explorer)
    throw RuntimeError("No current explorer");

  QString script;
  updateFromOptions(opts, "script", script);
  QString preScript;
  updateFromOptions(opts, "pre-script", preScript);
  bool justPick = false;
  updateFromOptions(opts, "just-pick", justPick);
  QString impScript;
  updateFromOptions(opts, "improved-script", impScript);

  explorer->linearPreFit = false;
  updateFromOptions(opts, "linear-prefit", explorer->linearPreFit);


  QString autoSave = QString("QSoas-%1-%2.%3.trj.dat").
    arg(ws->fitName(false)).
    arg(explorer->createdFrom->name).
    arg(QCoreApplication::applicationPid());
  bool disableAutoSave = false;
  updateFromOptions(opts, "disable-auto-save", disableAutoSave);
  if(disableAutoSave)
    autoSave = "";

  QStringList lst;
  for(int i = 1; i <= 2; i++) {
    QString n = QString("arg%1").arg(i);
    if(opts.contains(n))
      lst << opts[n]->value<QString>();
    else
      break;
  }

  explorer->clearHooks();
  if(! preScript.isEmpty()) {
    std::function<bool ()> fn = [preScript, lst]() -> bool {
      Terminal::out << "Running pre-iteration script: '" << preScript
                    << "'" << endl;
      CommandWidget::ScriptStatus st =
        soas().prompt().runCommandFile(preScript, lst);
      if(!st == CommandWidget::Success) {
        Terminal::out << "Script '" << preScript
                      << "' failed, stopping iteration" << endl;
        return false;
      }
      return true;
    };
    explorer->addHook(fn);
  }


  while(true) {
    QString lr = "(none yet)";
    double res = -1;
    QString summary;
    if(ws->trajectories.size() > 0) {
      lr = QString("%1 (%2)").
        arg(ws->trajectories.best().residuals).
        arg(ws->trajectories.best().endTime.toString());
      res = ws->trajectories.best().residuals;

      static const double thresholds[] =
        {
         1.15, 1.5, 3.0, 30
        };
      static const int number = sizeof(thresholds)/sizeof(thresholds[0]);
      int nb[number + 1];
      for(int i = 0; i < number + 1; i++)
        nb[i] = 0;
      for(const FitTrajectory & t : ws->trajectories) {
        bool found = false;
        for(int i = 0; i < number; i++) {
          if(t.residuals < thresholds[i] * res) {
            nb[i] += 1;
            found = true;
            break;
          }
        }
        if(! found)
          nb[number] += 1;
        summary =  QString("\n -> residuals: %1").arg(nb[0]);
        for(int i = 0; i < number; i++)
          summary += QString(" < %1|\t %2").arg(thresholds[i], 0, 'f', 2).
            arg(nb[i+1]);
        // below 1.5 ?
        summary += QString(" over (%1% efficiency)").
          arg(100.0*(nb[0] + nb[1])/ws->trajectories.size(), 0, 'g', 3);
      }
      
    
    }
    
    Terminal::out << "Explorer '" << explorer->createdFrom->name
                  << "' iteration: " << explorer->progressText()
                  << " starting " << QDateTime::currentDateTime().toString()
                  << ", current best residuals: "
                  << lr << summary
                  << endl;
    bool cont = explorer->iterate(justPick);
    if(justPick) {
      Terminal::out << " -> stopping just after picking parameters" << endl;
      break;                    // We're done
    }
    if(! script.isEmpty()) {
       Terminal::out << "Running end-of-iteration script: '" << script
                    << "'" << endl;
      CommandWidget::ScriptStatus st =
        soas().prompt().runCommandFile(script, lst);
      if(!st == CommandWidget::Success) {
        Terminal::out << "Script '" << script
                      << "' failed, stopping iteration" << endl;
        cont = false;
      }
    }
    if(! impScript.isEmpty() && 
       ws->trajectories.size() > 0 && 
       ws->trajectories.best().residuals < res) {
      Terminal::out << "Residuals have improved from " << res
                    << " to " << ws->trajectories.best().residuals
                    << ", running script '" << impScript << "'" << endl;
      CommandWidget::ScriptStatus st =
        soas().prompt().runCommandFile(impScript, lst);
      if(!st == CommandWidget::Success) {
        Terminal::out << "Script '" <<
          impScript << "' failed, stopping iteration" << endl;
        cont = false;
      }
      
    }
    if(! autoSave.isEmpty()) {
      try {
        Terminal::out << "Saving trajectories to '"
                      << autoSave << "'";
        File f(autoSave, File::TextOverwrite);
        QTextStream o(f);
        o << "# Fit command-line: " << soas().currentCommandLine() << endl;
        ws->trajectories.exportToFile(o);
        Terminal::out << " -> OK" << endl;
      }
      catch(const RuntimeError & e) {
        Terminal::out << " -> failed: " << e.message() << endl;
      }
    }
    
    if(! cont)
      break;
  }
}


ArgumentList ieOpts(QList<Argument*>() 
                    << new FileArgument("pre-script", 
                                        "Pre-iteration script",
                                        "script file run after choosing the parameters and before choosing the file")
                    << new FileArgument("script", 
                                        "Script",
                                        "script file run after the iteration", false, true)
                    << new FileArgument("improved-script", 
                                        "Script for improvement",
                                        "script file run whenever the best residuals have improved")
                    << new BoolArgument("just-pick", 
                                        "Just pick",
                                        "If true, then just picks the next initial parameters, don't fit, don't iterate")
                    << new BoolArgument("linear-prefit", 
                                        "Linear prefit",
                                        "If true, runs a linear pre-fit on before running the real fit")
                    << new BoolArgument("disable-auto-save",
                                        "Disable auto save",
                                        "If true, the trajectories are not automatically saved at each iteration (default: false)")
                    << new FileArgument("arg1", 
                                        "First argument",
                                        "First argument to the scripts")
                    << new FileArgument("arg2", 
                                        "Second argument",
                                        "Second argument to the scripts")
                      );


static Command 
trim("iterate-explorer", // command name
     effector(iterateExplorerCommand), // action
     "fits",  // group name
     NULL, // arguments
     &ieOpts,
     "Iterate explorer",
     "Run all the iterations of the current explorer",
     "", CommandContext::fitContext());
