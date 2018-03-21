/*
  fit-commands.cc: implementation of many fit commands
  Copyright 2018 by CNRS/AMU

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
#include <fitworkspace.hh>
#include <fittrajectory.hh>
#include <terminal.hh>
#include <soas.hh>

#include <utils.hh>
#include <debug.hh>

#include <command.hh>
#include <commandcontext.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>

#include <fwexpression.hh>
#include <mruby.hh>


// static Group fit("fit", 0,
//                  "Fit",
//                  "Commands for fitting");


static void quitCommand(const QString & /*name*/)
{
  FitWorkspace::currentWorkspace()->quit();
}

static Command 
quit("quit", // command name
     optionLessEffector(quitCommand), // action
     "fit",  // group name
     NULL, // arguments
     NULL, // options
     "Quit",
     "Closes the fit window",
     "", CommandContext::fitContext());


//////////////////////////////////////////////////////////////////////

static void saveCommand(const QString & /*name*/, QString file)
{
  FitWorkspace::currentWorkspace()->saveParameters(file);
}

ArgumentList sA(QList<Argument*>() 
                << new FileArgument("file", 
                                    "Parameter file",
                                    "name of the file for saving the parameters")
                );

static Command 
save("save", // command name
     optionLessEffector(saveCommand), // action
     "fit",  // group name
     &sA, // arguments
     NULL, // options
     "Save",
     "Save the current parameters to file",
     "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void loadCommand(const QString & /*name*/, QString file)
{
  FitWorkspace::currentWorkspace()->loadParameters(file);
}

ArgumentList lA(QList<Argument*>() 
                << new FileArgument("file", 
                                    "Parameter file",
                                    "name of the file to load the parameters from")
                );

static Command 
load("load", // command name
     optionLessEffector(loadCommand), // action
     "fit",  // group name
     &lA, // arguments
     NULL, // options
     "Load",
     "Load the parameters from a file",
     "", CommandContext::fitContext());


//////////////////////////////////////////////////////////////////////

static void fitCommand(const QString & /*name*/, const CommandOptions & opts)
{
  int iterations = 50;
  updateFromOptions(opts, "iterations", iterations);
  FitWorkspace::Ending st = FitWorkspace::currentWorkspace()->runFit(iterations);

  Terminal::out << "Fit ended, status: "
                << FitTrajectory::endingName(st) << endl;
  // Throw an exception if the fit was cancelled, so that hitting
  // abort also aborts scripts.
  if(st == FitWorkspace::Cancelled)
    throw RuntimeError("Fit cancelled");
}

ArgumentList fOpts(QList<Argument*>() 
                   << new IntegerArgument("iterations", 
                                          "Number of iterations",
                                          "the maximum number of iterations of the fitting process"));

static Command 
fit("fit", // command name
    effector(fitCommand), // action
    "fit",  // group name
    NULL, // arguments
    &fOpts, // options
    "Fit",
    "Run the fit",
    "", CommandContext::fitContext());


//////////////////////////////////////////////////////////////////////



/// An argument that represents a list of parameter "names"
class FitParameterArgument : public Argument {
public:

  FitParameterArgument(const char * cn, const char * pn,
                       const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  }; 
  
  /// Returns a wrapped QList<QPair<int, int> >
  virtual ArgumentMarshaller * fromString(const QString & str) const override {
    QList<QPair<int, int> > rv = FitWorkspace::currentWorkspace()->parseParameterList(str);
    return new ArgumentMarshallerChild< QList<QPair<int, int> > >(rv);
  }

  virtual QStringList proposeCompletion(const QString & starter) const override {
    return Utils::stringsStartingWith(FitWorkspace::currentWorkspace()->parameterNames(), starter);
  }


  virtual QString typeName() const override {
    return "parameter-name";
  };

  virtual QString typeDescription() const override {
    return "...";
  };

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const {
    return Argument::convertRubyString(value);
  };

};

//////////////////////////////////////////////////////////////////////

static void setCommand(const QString & /*name*/, QList<QPair<int, int> > params,
                       QString value, const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  bool expression = false;
  updateFromOptions(opts, "expression", expression);
  if(expression) {
    MRuby * mr = MRuby::ruby();
    FWExpression exp(value, ws);
    for(QPair<int, int> ps : params) {
      mrb_value v = exp.evaluate(ps.second);
      /// @todo Here: check for a string return value
      ws->setValue(ps.first, ps.second, mr->floatValue(v));
    }
  }
  else {
    for(QPair<int, int> ps : params) {
      ws->setValue(ps.first, ps.second, value);
    }
  }
}

ArgumentList sArgs(QList<Argument*>() 
                   << new FitParameterArgument("parameter", 
                                               "Parameter",
                                               "the parameters of the fit")
                   << new StringArgument("value", 
                                         "Value",
                                         "the value")
                   );

ArgumentList sOpts(QList<Argument*>()
                   << new BoolArgument("expression", 
                                       "Expression",
                                       "whether the value is evaluated as an expression")
                   );

static Command 
set("set", // command name
    effector(setCommand), // action
    "fit",  // group name
    &sArgs, // arguments
    &sOpts, // options
    "Set parameter",
    "Sets the parameters",
    "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void fixUnfix(const QString & name, QList<QPair<int, int> > params)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  bool fix = (name == "fix");
  for(QPair<int, int> ps : params)
    ws->setFixed(ps.first, ps.second, fix);
}

ArgumentList fuArgs(QList<Argument*>() 
                    << new FitParameterArgument("parameter", 
                                                "Parameter",
                                                "the parameters to fix/unfix")
                   );

static Command 
fix("fix", // command name
    optionLessEffector(fixUnfix), // action
    "fit",  // group name
    &fuArgs, // arguments
    NULL, // options
    "Fix parameter",
    "Fixs the parameter",
    "", CommandContext::fitContext());

static Command 
ufix("unfix", // command name
     optionLessEffector(fixUnfix), // action
     "fit",  // group name
     &fuArgs, // arguments
     NULL, // options
     "Unfix parameter",
     "Lets the parameter be free again",
     "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void globalLocal(const QString & name, QList<QPair<int, int> > params)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  bool global = (name == "global");
  QSet<int> done;
  for(QPair<int, int> ps : params) {
    if(! done.contains(ps.first)) {
      // Do it only once
      ws->setGlobal(ps.first, global);
      done.insert(ps.first);
    }
  }
}

ArgumentList glArgs(QList<Argument*>() 
                    << new FitParameterArgument("parameter", 
                                                "Parameter",
                                                "the parameters whose global/local status to change")
                   );

static Command 
glb("global", // command name
    optionLessEffector(globalLocal), // action
    "fit",  // group name
    &glArgs, // arguments
    NULL, // options
    "Global parameter",
    "Sets the parameter to be a global one",
    "", CommandContext::fitContext());

static Command 
loc("local", // command name
    optionLessEffector(globalLocal), // action
    "fit",  // group name
    &glArgs, // arguments
    NULL, // options
    "Local parameter",
    "Sets the parameter to be a local one",
    "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void exportCommand(const QString & /*name*/, const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  QString file;
  updateFromOptions(opts, "file", file);
  bool errors = true;
  updateFromOptions(opts, "errors", errors);
  if(file.isEmpty()) {
    Terminal::out << "Exporting parameters to output file" << endl;
    ws->exportToOutFile(errors);
  }
  else {
    QFile f(file);
    Utils::open(&f, QIODevice::WriteOnly);
    Terminal::out << "Exporting parameters to the file '"
                  << file << "'" << endl;
    ws->exportParameters(&f, errors);
  }
}

ArgumentList eOpts(QList<Argument*>()
                   << new FileArgument("file", 
                                       "Parameter file",
                                       "name of the file for saving the parameters", false, true)
                   << new BoolArgument("errors", 
                                       "Errors",
                                       "whether the errors are exported too")
                   );

static Command 
expt("export", // command name
    effector(exportCommand), // action
    "fit",  // group name
    NULL, // arguments
    &eOpts, // options
    "Export parameters",
    "Export the parameters to a file/to the output file",
    "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void evalCommand(const QString & /*name*/, QString formula,
                        const CommandOptions & /*opts*/)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  MRuby * mr = MRuby::ruby();
  mrb_value value;
  FWExpression exp(formula, ws);
  value = exp.evaluate();
  Terminal::out << " => " << mr->inspect(value) << endl;
}

ArgumentList eArgs(QList<Argument*>() 
                   << new StringArgument("expression", 
                                         "Expression",
                                         "the expression to evaluate")
                   );

static Command 
evl("eval", // command name
    effector(evalCommand), // action
    "fit",  // group name
    &eArgs, // arguments
    NULL, // options
    "Evaluate",
    "Evaluate a Ruby expression in the current context",
    "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void selectCommand(const QString & /*name*/, int ds,
                          const CommandOptions & /*opts*/)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  ws->selectDataSet(ds, false);
}

ArgumentList slArgs(QList<Argument*>() 
                   << new IntegerArgument("dataset", 
                                          "Dataset",
                                          "the number of the dataset in the fit (not in the stack)")
                   );

static Command 
sel("select", // command name
    effector(selectCommand), // action
    "fit",  // group name
    &slArgs, // arguments
    NULL, // options
    "Select",
    "Selects the current dataset",
    "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void resetCommand(const QString & /*name*/,
                         const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  QString source = "initial";
  updateFromOptions(opts, "source", source);
  if(source == "initial") {
    ws->resetAllToInitialGuess();
  }
  else {
    ws->resetToBackup();
  }
}

ArgumentList resetOpts(QList<Argument*>()
                       << new ChoiceArgument(QStringList()
                                             << "initial" << "backup",
                                             "source", "Source"
                                             "source of the parameters")
                       );

static Command 
res("reset", // command name
    effector(resetCommand), // action
    "fit",  // group name
    NULL, // arguments
    &resetOpts, // options
    "Reset",
    "Reset parameters to previous values",
    "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void listTrajectoriesCommand(const QString & /*name*/,
                                    const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  QString flag;
  updateFromOptions(opts, "flag", flag);
  int idx = 0;
  for(const FitTrajectory & t : ws->trajectories) {
    if(flag.isEmpty() || t.flagged(flag))
      Terminal::out << "#" << idx++ << ":\t"
                    << t.relativeResiduals << "\tstarted: "
                    << t.startTime.toString() << " (" 
                    << t.engine << ") -> "
                    << FitTrajectory::endingName(t.ending) << endl;
  }
}

ArgumentList lTOpts(QList<Argument*>()
                    << new ChoiceArgument([]() -> QStringList {
                        FitWorkspace * ws =
                          FitWorkspace::currentWorkspace();
                        QStringList a = ws->trajectories.allFlags().toList();
                        return a;
                      }
                      ,
                      "flag", "Flag"
                      "only show trajectories with the given flag")
                    );

static Command 
st("list-trajectories", // command name
   effector(listTrajectoriesCommand), // action
   "fit",  // group name
   NULL, // arguments
   &lTOpts, // options
   "List trajectories",
   "List briefly all the trajectories",
   "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void trajectoriesNameCommand(const QString & /*name*/,
                                    const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  QStringList flags;
  updateFromOptions(opts, "flags", flags);
  ws->setTrajectoryFlags(flags.toSet());
}

ArgumentList tNOpts(QList<Argument*>()
                    << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                                  "flags", 
                                                  "Flags",
                                                  "Flags to set on the new trajectories", true)
                    );

static Command 
tn("flag-trajectories", // command name
   effector(trajectoriesNameCommand), // action
   "fit",  // group name
   NULL, // arguments
   &tNOpts, // options
   "Flag trajectories",
   "Define flags for subsequent trajectories",
   "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void saveTrajectoriesCommand(const QString & /*name*/,
                                    QString file,
                                    const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  QString flag;
  updateFromOptions(opts, "flag", flag);
  FitTrajectories trjs = ws->trajectories;
  if(! flag.isEmpty())
    trjs = trjs.flaggedTrajectories(flag);

  QString mode = "fail";
  updateFromOptions(opts, "mode", mode);

  if(QFile::exists(file)) {
    if(mode == "fail")
      throw RuntimeError("Not overwriting existing file '%1'").
        arg(file);
    if(mode == "update") {
      Terminal::out << "Updating trajectories from file '"
                << file
                << "'" << endl;
      try {
        QFile fl(file);
        Utils::open(&fl, QIODevice::ReadOnly);
    
        int nb;
        FitTrajectories update(ws);
        QTextStream in(&fl);
        nb = update.importFromFile(in);
        trjs.merge(update);
      }
      catch (const Exception & er) {
        Terminal::out << "Could not load trajectory file " << file
                      << ": " << er.message() << ", ignoring" << endl;
      }
    }
  }

  QFile f(file);
  Utils::open(&f, QIODevice::WriteOnly);

  Terminal::out << "Saving fit trajectories data to '"
                << file << "'" << endl;
  
  QTextStream o(&f);
  trjs.exportToFile(o);
}


ArgumentList sTA(QList<Argument*>() 
                 << new FileArgument("file", 
                                     "Trajectory file",
                                     "name of the file for saving the trajectories")
                );


ArgumentList sTOpts(QList<Argument*>()
                    << new ChoiceArgument([]() -> QStringList {
                        FitWorkspace * ws =
                          FitWorkspace::currentWorkspace();
                        QStringList a = ws->trajectories.allFlags().toList();
                        return a;
                      }
                      ,
                      "flag", "Flag"
                      "only show trajectories with the given flag")
                    << new ChoiceArgument(QStringList() << "overwrite"
                                          << "update" << "fail"
                                          , "mode", "Mode"
                                          "what to do if the target file already exists")
                    );

static Command 
sat("save-trajectories", // command name
    effector(saveTrajectoriesCommand), // action
    "fit",  // group name
    &sTA, // arguments
    &sTOpts, // options
    "Save trajectories",
    "Save the trajectories to a file",
    "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void loadTrajectoriesCommand(const QString & /*name*/,
                                    QString file,
                                    const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();

  QString mode = "update";
  updateFromOptions(opts, "mode", mode);
  QFile fl(file);
  try {
    Utils::open(&fl, QIODevice::ReadOnly);
  }
  catch(RuntimeError & e) {
    if(mode == "ignore") {
      Terminal::out << "Error opening file '" << file
                    << "', : " << e.message()
                    << ", but ignoring as requested" << endl;
      return;
    }
    throw;
  }
    
  int nb;
  FitTrajectories update(ws);
  QTextStream in(&fl);
  nb = update.importFromFile(in);
  if(mode == "drop")
    ws->trajectories.clear();
  ws->trajectories.merge(update);
}


ArgumentList ldTA(QList<Argument*>() 
                 << new FileArgument("file", 
                                     "Trajectory file",
                                     "name of the file for saving the trajectories")
                );


ArgumentList ldTOpts(QList<Argument*>()
                     << new ChoiceArgument(QStringList() << "drop"
                                           << "update"
                                           << "ignore"
                                           , "mode", "Mode"
                                           "what to do with current trajectories")
                    );

static Command 
ldt("load-trajectories", // command name
    effector(loadTrajectoriesCommand), // action
    "fit",  // group name
    &ldTA, // arguments
    &ldTOpts, // options
    "Load trajectories",
    "Load trajectories from a file",
    "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void trimTrajectoriesCommand(const QString & /*name*/,
                                    double factor,
                                    const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  int nb = ws->trajectories.trim(factor);
  Terminal::out << "Trimmed " << nb << " trajectories" << endl;
}


ArgumentList trimTA(QList<Argument*>() 
                 << new NumberArgument("threshold", 
                                       "Threshold",
                                       "threshold for trimming")
                );


static Command 
trim("trim-trajectories", // command name
     effector(trimTrajectoriesCommand), // action
     "fit",  // group name
     &trimTA, // arguments
     NULL, // options
     "Trim trajectories",
     "Trim trajectories whose residuals are too high",
     "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

#include <fittrajectorydisplay.hh>

static void browseTrajectoriesCommand(const QString & /*name*/,
                                      const CommandOptions & opts)
{
  FitTrajectoryDisplay d(FitWorkspace::currentWorkspace());
  d.exec();
}


static Command 
brse("browse-trajectories", // command name
     effector(browseTrajectoriesCommand), // action
     "fit",  // group name
     NULL, // arguments
     NULL, // options
     "Browse trajectories",
     "Opens a dialog box to browse trajectories",
     "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

#include <parametersviewer.hh>

static void showParametersCommand(const QString & /*name*/,
                                  const CommandOptions & opts)
{
  ParametersViewer dlg(FitWorkspace::currentWorkspace());
  dlg.exec();
}

static Command 
spar("show-parameters", // command name
     effector(showParametersCommand), // action
     "fit",  // group name
     NULL, // arguments
     NULL, // options
     "Show parameters",
     "Opens a dialog box to show the current parameters",
     "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

#include <parametersspreadsheet.hh>

static void parametersSpreadsheetCommand(const QString & /*name*/,
                                         const CommandOptions & opts)
{
  ParametersSpreadsheet dlg(FitWorkspace::currentWorkspace());
  dlg.exec();
}

static Command 
spsh("parameters-spreadsheet", // command name
     effector(parametersSpreadsheetCommand), // action
     "fit",  // group name
     NULL, // arguments
     NULL, // options
     "Parameters spreadsheet",
     "Opens a spreadsheet to edit parameters",
     "", CommandContext::fitContext());
