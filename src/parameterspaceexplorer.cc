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

#include <fitworkspace.hh>
#include <fittrajectory.hh>

#include <terminal.hh>

ParameterSpaceExplorerFactoryItem::
ParameterSpaceExplorerFactoryItem(const QString & n,
                                  const QString & pn,
                                  ArgumentList * args,
                                  ArgumentList * opts,
                                  ParameterSpaceExplorerFactoryItem::Creator c)  :
  Factory(n, c, pn), publicName(description)
{
  QString cmdName = n + "-explorer";
  cmd = new Command(cmdName.toLocal8Bit().data(),
                    ParameterSpaceExplorer::explorerEffector(n), "fits",
                    args, opts,
                    pn.toLocal8Bit().data(),
                    "", "",
                    CommandContext::fitContext());

}

//////////////////////////////////////////////////////////////////////

ParameterSpaceExplorer::ParameterSpaceExplorer(FitWorkspace * ws) :
  workSpace(ws), createdFrom(NULL)
{
}

ParameterSpaceExplorer::~ParameterSpaceExplorer()
{
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
  for(const std::function< bool()> h : preFitHooks) {
    if(! h())
      return false;
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
    if(ws->trajectories.size() > 0) {
      lr = QString("%1 (%2)").
        arg(ws->trajectories.best().residuals).
        arg(ws->trajectories.best().endTime.toString());
      res = ws->trajectories.best().residuals;
    }
    
    Terminal::out << "Explorer '" << explorer->createdFrom->name
                  << "' iteration: " << explorer->progressText()
                  << " starting " << QDateTime::currentDateTime().toString()
                  << ", current best residuals: "
                  << lr
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
    
    if(! cont)
      break;
  }
}


ArgumentList ieOpts(QList<Argument*>() 
                    << new FileArgument("pre-script", 
                                        "Pre-iteration script",
                                        "script file run after choosing the parameters and before choosing the file", false, true)
                    << new FileArgument("script", 
                                        "Script",
                                        "script file run after the iteration", false, true)
                    << new FileArgument("improved-script", 
                                        "Script for improvement",
                                        "script file run whenever the best residuals have improved", false, true)
                    << new BoolArgument("just-pick", 
                                        "Just pick",
                                        "If true, then just picks the next initial parameters, don't fit, don't iterate")
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
