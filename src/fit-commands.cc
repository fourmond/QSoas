/*
  fit-commands.cc: implementation of many fit commands
  Copyright 2018, 2019, 2010 by CNRS/AMU

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

#include <fitparametersfile.hh>
#include <fitdata.hh>
#include <dataset.hh>

#include <fwexpression.hh>
#include <mruby.hh>

#include <file.hh>
#include <filelock.hh>

#include <argument-templates.hh>

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
     "fits",  // group name
     NULL, // arguments
     NULL, // options
     "Quit",
     "Closes the fit window",
     "", CommandContext::fitContext());


//////////////////////////////////////////////////////////////////////

static void saveCommand(const QString & /*name*/, QString file, const CommandOptions & opts)
{
  // int rotation = 0;
  // updateFromOptions(opts, "rotate", rotation);
  // if(rotation != 0)
  //   Utils::rotateFile(file, rotation);
  FitWorkspace::currentWorkspace()->saveParameters(file, opts);
}

ArgumentList sA(QList<Argument*>() 
                << new FileArgument("file", 
                                    "Parameter file",
                                    "name of the file for saving the parameters")
                );

ArgumentList sO(QList<Argument*>() 
                << File::fileOptions(File::OverwriteOption|
                                     File::RotationOption|File::MkPathOption)

                );

static Command 
save("save", // command name
     effector(saveCommand), // action
     "fits",  // group name
     &sA, // arguments
     &sO, // options
     "Save",
     "Save the current parameters to file",
     "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void loadCommand(const QString & /*name*/, QString file,
                        const CommandOptions & opts)
{
  File f(file, File::TextRead);
  QTextStream in(f);
  FitParametersFile params;

  params.readFromStream(in);

  QString mode = "normal";
  updateFromOptions(opts, "mode", mode);

  FitWorkspace * ws = FitWorkspace::currentWorkspace();

  if(mode == "normal") {
    ws->loadParameters(params);
  }
  else {
    QHash<int, int> splice;
    const QList<const DataSet * > & datasets = ws->data()->datasets;
    if(mode == "buffer-name") {
      Terminal::out << "Loading '" << file
                    << "' according to buffer names"
                    << endl;
                       
      for(int i = 0; i < datasets.size(); i++) {
        const QString & n = datasets[i]->name;
        int idx = params.bufferByName.value(n, -1);
        Terminal::out << " * " << n;
        if(idx >= 0) {
          Terminal::out << " -> " <<  idx;
          splice[i] = idx;
        }
        else
          Terminal::out << " not found";
        Terminal::out << endl;
      }
    }
    if(mode == "closest-Z") {
      if(! ws->hasPerpendicularCoordinates())
        throw RuntimeError("The buffers have no perpendicular coordinates");
        
      QList<QPair <double, int> > buffersByZ;
      for(int i : params.bufferZValues.keys())
        buffersByZ << QPair<double, int>(params.bufferZValues[i], i);
      std::sort(buffersByZ.begin(), buffersByZ.end(),
                [](const QPair<double, int> & a,
                   const QPair<double, int> & b) -> bool {
                  return a.first < b.first;
                });
      if(buffersByZ.size() == 0)
        throw RuntimeError("No parameter Z values found in '%1'").
          arg(file);
      Terminal::out << "Matching buffers based on Z values" << endl;
      for(int j = 0; j < datasets.size(); j++) {
        double v = ws->perpendicularCoordinates[j];
        int i = 0;
        for(; i < buffersByZ.size(); i++) {
          if(v < buffersByZ[i].first) {
            if(i > 0) {
              if((buffersByZ[i].first - v) > (v - buffersByZ[i-1].first))
                --i;
            }
            break;
          }
        }
        if(i >= buffersByZ.size())
          i = buffersByZ.size() - 1;
        i = buffersByZ[i].second; // Now i is the dataset number
        Terminal::out << " * #" << j << " " << datasets[j]->name
                      << " Z = " << v << " <- #"
                      << i << ", " << params.bufferNames[i]
                      << " Z = " << params.bufferZValues[i]
                      << endl;
        splice[j] = i;
      }
    }
    ws->loadParameters(params, splice);
  }
}

ArgumentList lA(QList<Argument*>() 
                << new FileArgument("file", 
                                    "Parameter file",
                                    "name of the file to load the parameters from")
                );

ArgumentList lO(QList<Argument*>() 
                << new ChoiceArgument(QStringList()
                                      << "normal" << "buffer-name"
                                      << "closest-Z",
                                      "mode", "Loading mode"
                                      "mode to chose the correspondance between source and target datasets")
                );

static Command 
load("load", // command name
     effector(loadCommand), // action
     "fits",  // group name
     &lA, // arguments
     &lO, // options
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
    "fits",  // group name
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

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override {
    return Argument::convertRubyString(value);
  };

  virtual QStringList toString(const ArgumentMarshaller * arg) const override {
    QStringList lst;
    NOT_IMPLEMENTED;
    return lst;
  };

  virtual QWidget * createEditor(QWidget * parent = NULL) const override {
    return Argument::createTextEditor(parent);
  }

  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override {
    Argument::setTextEditorValue(editor, value);
  };


};

//////////////////////////////////////////////////////////////////////

static void setCommand(const QString & /*name*/, QList<QPair<int, int> > params,
                       QString value, const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  bool expression = false;
  updateFromOptions(opts, "expression", expression);

  bool fix = false;
  updateFromOptions(opts, "fix", fix);
  bool unfix = false;
  updateFromOptions(opts, "unfix", unfix);

  if(expression) {
    MRuby * mr = MRuby::ruby();
    FWExpression exp(value, ws);
    for(QPair<int, int> ps : params) {
      // We use the first dataset as source for the meta
      mrb_value v = exp.evaluate(ps.second >= 0 ? ps.second : 0);
      /// @todo Here: check for a string return value
      if(fix || unfix)
        ws->setFixed(ps.first, ps.second, fix);
      ws->setValue(ps.first, ps.second, mr->floatValue(v));
    }
  }
  else {
    for(QPair<int, int> ps : params) {
      if(fix || unfix)
        ws->setFixed(ps.first, ps.second, fix);
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
                   << new BoolArgument("fix", 
                                       "Fix",
                                       "if true, also fixes the parameters")
                   << new BoolArgument("unfix", 
                                       "Unfix",
                                       "if true, also unfixes the parameters")
                   );

static Command 
set("set", // command name
    effector(setCommand), // action
    "fits",  // group name
    &sArgs, // arguments
    &sOpts, // options
    "Set parameter",
    "Sets the parameters",
    "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void setFromDatasetCommand(const QString & /*name*/,
                                  QString name,
                                  QList<const DataSet *> dss,
                                  const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  int idx = ws->parameterIndex(name);
  if(idx < 0)
    throw RuntimeError("Unkown parameter: '%1'").arg(name);

  if(! ws->hasPerpendicularCoordinates())
    throw RuntimeError("The buffers have no perpendicular coordinates");

  if(dss.size() != 1)
    throw RuntimeError("Need exactly one buffer !");

  const DataSet * ds = dss[0];


  int nbds = ws->data()->datasets.size();
  for(int i = 0; i < nbds; i++) {
    double z = ws->perpendicularCoordinates[i];
    ws->setValue(idx, i, ds->yValueAt(z, true));
  }
}

ArgumentList sfdArgs(QList<Argument*>() 
                     << new StringArgument("parameter", 
                                           "Parameter",
                                           "the parameters of the fit")
                     << new SeveralDataSetArgument("source", 
                                                   "Source",
                                                   "the source for the data")
                     );

// ArgumentList sOpts(QList<Argument*>()
//                    << new CodeArgument("convert-x", 
//                                        "Convert X",
//                                        "used to convert the X values")
//                    << new BoolArgument("fix", 
//                                        "Fix",
//                                        "if true, also fixes the parameters")
//                    << new BoolArgument("unfix", 
//                                        "Unfix",
//                                        "if true, also unfixes the parameters")
//                    );

static Command 
sfd("set-from-dataset", // command name
    effector(setFromDatasetCommand), // action
    "fits",  // group name
    &sfdArgs, // arguments
    NULL, // options
    "Set parameter from dataset",
    "Sets the parameter from a dataset using interpolation on Z values",
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
    "fits",  // group name
    &fuArgs, // arguments
    NULL, // options
    "Fix parameter",
    "Fixs the parameter",
    "", CommandContext::fitContext());

static Command 
ufix("unfix", // command name
     optionLessEffector(fixUnfix), // action
     "fits",  // group name
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
    "fits",  // group name
    &glArgs, // arguments
    NULL, // options
    "Global parameter",
    "Sets the parameter to be a global one",
    "", CommandContext::fitContext());

static Command 
loc("local", // command name
    optionLessEffector(globalLocal), // action
    "fits",  // group name
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
    File f(file, File::TextWrite, opts);
    Terminal::out << "Exporting parameters to the file '"
                  << file << "'" << endl;
    ws->exportParameters(f, errors);
  }
}

ArgumentList eOpts(QList<Argument*>()
                   << new FileArgument("file", 
                                       "Parameter file",
                                       "name of the file for saving the parameters", false, true)
                   << new BoolArgument("errors", 
                                       "Errors",
                                       "whether the errors are exported too")
                   << File::fileOptions(File::OverwriteOption|File::MkPathOption)
                   );

static Command 
expt("export", // command name
    effector(exportCommand), // action
    "fits",  // group name
    NULL, // arguments
    &eOpts, // options
    "Export parameters",
    "Export the parameters to a file/to the output file",
    "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

#include <datastackhelper.hh>

static void pushCommand(const QString & /*name*/, const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  bool recompute = true;
  updateFromOptions(opts, "recompute", recompute);
  bool subfunctions = false;
  updateFromOptions(opts, "subfunctions", subfunctions);
  bool residuals = false;
  updateFromOptions(opts, "residuals", residuals);
  DataStackHelper pusher(opts);
  if(recompute)
    ws->recompute(false, false); // Second false since we want
                                 // exceptions to show up
  ws->pushComputedData(residuals, subfunctions, &pusher);
}

ArgumentList pOpts(QList<Argument*>()
                   << new BoolArgument("subfunctions", 
                                       "Subfunctions",
                                       "whether the subfunctions are also exported or not")
                   << new BoolArgument("recompute", 
                                       "Recompute",
                                       "whether or not to recompute the fit (on by default)")
                   << new BoolArgument("residuals", 
                                       "Residuals",
                                       "if true, push the residuals rather than the computed values")
                   << DataStackHelper::helperOptions()
                   );

static Command 
psh("push", // command name
    effector(pushCommand), // action
    "fits",  // group name
    NULL, // arguments
    &pOpts, // options
    "Push to stack",
    "Push the computed data to stack",
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
    "fits",  // group name
    &eArgs, // arguments
    NULL, // options
    "Evaluate",
    "Evaluate a Ruby expression in the current context",
    "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

static void verifyCommand(const QString & /*name*/, QString formula,
                        const CommandOptions & /*opts*/)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  MRuby * mr = MRuby::ruby();
  mrb_value value;
  FWExpression exp(formula, ws);
  value = exp.evaluate();
  Terminal::out << " => " << mr->inspect(value) << endl;
}

static Command 
vfy("verify", // command name
    effector(verifyCommand), // action
    "fits",  // group name
    &eArgs, // arguments
    NULL, // options
    "Verify",
    "Evaluate a Ruby expression in the current context, and raises an exception if it is false",
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
    "fits",  // group name
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
    "fits",  // group name
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
                    << FitTrajectory::endingName(t.ending)
                    << "\t\tFlags: "
                    << QStringList(t.flags.toList()).join(",")
                    << endl;
  }
}

ArgumentList lTOpts(QList<Argument*>()
                    /// @todo This should probably be a separated
                    /// type.
                    << new ChoiceArgument([]() -> QStringList {
                        FitWorkspace * ws =
                          FitWorkspace::currentWorkspace();
                        QStringList a;
                        if(ws)
                          a = ws->trajectories.allFlags().toList();
                        return a;
                      }
                      ,
                      "flag", "Flag"
                      "only show trajectories with the given flag")
                    );

static Command 
st("list-trajectories", // command name
   effector(listTrajectoriesCommand), // action
   "fits",  // group name
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
   "fits",  // group name
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
        File fl(file, File::TextRead);

        int nb;
        FitTrajectories update(ws);
        QTextStream in(fl);
        nb = update.importFromFile(in);
        trjs.merge(update);
      }
      catch (const Exception & er) {
        Terminal::out << "Could not load trajectory file " << file
                      << ": " << er.message() << ", ignoring" << endl;
      }
    }
  }

  File f(file, File::TextWrite);

  Terminal::out << "Saving " << trjs.size() << " fit trajectories to '"
                << file << "'" << endl;
  
  QTextStream o(f);
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
                        QStringList a;
                        if(ws)
                          a = ws->trajectories.allFlags().toList();
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
    "fits",  // group name
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
  FitTrajectories update(ws);
  File fl(file, File::TextRead);
  int nb;

  try {
    QTextStream in(fl);
    nb = update.importFromFile(in);
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
    "fits",  // group name
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
  int max = -1;
  updateFromOptions(opts, "at-most", max);
  if(max > 0)
    nb += ws->trajectories.keepBestTrajectories(max);
  Terminal::out << "Trimmed " << nb << " trajectories" << endl;
}


ArgumentList trimTA(QList<Argument*>() 
                 << new NumberArgument("threshold", 
                                       "Threshold",
                                       "threshold for trimming")
                );

ArgumentList trimTO(QList<Argument*>() 
                 << new IntegerArgument("at-most", 
                                        "Keep at most",
                                        "keep at most that many trajectories")
                );


static Command 
trim("trim-trajectories", // command name
     effector(trimTrajectoriesCommand), // action
     "fits",  // group name
     &trimTA, // arguments
     &trimTO, // options
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
     "fits",  // group name
     NULL, // arguments
     NULL, // options
     "Browse trajectories",
     "Opens a dialog box to browse trajectories",
     "", CommandContext::fitContext());


//////////////////////////////////////////////////////////////////////


static void memCommand(const QString &,
                       const CommandOptions & opts)
{
  MRuby * mr = MRuby::ruby();
  int kb = Utils::memoryUsed();
  Terminal::out << "Memory used: " << kb << " kB" << endl
                << "Ruby memory used: " << mr->memoryUse() << endl;
  long ut, kt;
  Utils::processorUsed(&ut, &kt);
  Terminal::out << "Total time used: " << (ut+kt)*0.001 << endl;
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  int sz = 0;
  if(ws->trajectories.size() > 0) {
    sz = 3 * sizeof(double) *
      ws->trajectories.last().initialParameters.size();
  }
  Terminal::out << "Fit trajectories: " << ws->trajectories.size()
                << " (~= " << (sz * ws->trajectories.size() >> 10) << " kB)"
                << endl;
}

static Command 
m("mem", // command name
  effector(memCommand), // action
  "fits",  // group name
  NULL, // arguments
  NULL, // options
  "Memory",
  "Informations about QSoas memory usage",
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
     "fits",  // group name
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
     "fits",  // group name
     NULL, // arguments
     NULL, // options
     "Parameters spreadsheet",
     "Opens a spreadsheet to edit parameters",
     "", CommandContext::fitContext());


//////////////////////////////////////////////////////////////////////


/// An argument that represents a list of trajectories
class TrajectoriesArgument : public Argument {
public:

  TrajectoriesArgument(const char * cn, const char * pn,
                       const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  }; 
  
  /// Returns a wrapped FitTrajectories
  virtual ArgumentMarshaller * fromString(const QString & str) const override {
    QStringList spl = str.split(":");
    QString what = spl.takeFirst();
    QString rest = spl.join(":");
    FitWorkspace * ws = FitWorkspace::currentWorkspace();

    if(what == "flagged") {
      return new ArgumentMarshallerChild<FitTrajectories>
        (ws->trajectories.flaggedTrajectories(rest));
    }
    if(what == "all")
      return new ArgumentMarshallerChild<FitTrajectories>
        (ws->trajectories);
      
    throw RuntimeError("Invalid trajectories specification: '%1'").
      arg(str);
    return NULL;
  }

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const override {
    for(const FitTrajectory & t : b->value<FitTrajectories>())
      a->value<FitTrajectories>() << t;
  }
    
  virtual QStringList proposeCompletion(const QString & starter) const override {
    FitWorkspace * ws = FitWorkspace::currentWorkspace();
    QStringList names;
    for(const QString & s : ws->trajectories.allFlags())
      names << "flagged:" + s;
    names << "all";
    return Utils::stringsStartingWith(names, starter);
  }


  virtual QString typeName() const override {
    return "trajectories";
  };

  virtual QString typeDescription() const override {
    return "Fit Trajectories";
  };

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override {
    return Argument::convertRubyString(value);
  };

  virtual QStringList toString(const ArgumentMarshaller * arg) const override {
    QStringList lst;
    NOT_IMPLEMENTED;
    return lst;
  };

  virtual QWidget * createEditor(QWidget * parent = NULL) const override {
    return Argument::createTextEditor(parent);
  }

  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override {
    Argument::setTextEditorValue(editor, value);
  };


};

//////////////////////////////////////////////////////////////////////

static void dropTrajectoriesCommand(const QString & /*name*/,
                                    FitTrajectories trajs)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  int nb = 0;
  for(const FitTrajectory & t : trajs) {
    for(int i = 0; i < ws->trajectories.size(); i++) {
      if(t == ws->trajectories[i]) {
        ws->trajectories.remove(i);
        ++nb;
        break;
      }
    }
  }
  Terminal::out << "Removed " << nb << " trajectories" << endl;
}


ArgumentList dropT(QList<Argument*>() 
                 << new TrajectoriesArgument("trajectories", 
                                             "Trajectories",
                                             "trajectories to remove", true)
                );

static Command 
drop("drop-trajectories", // command name
     optionLessEffector(dropTrajectoriesCommand), // action
     "fits",  // group name
     &dropT, // arguments
     NULL,
     "Drop trajectories",
     "Drop trajectories",
     "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////

#include <commandwidget.hh>

static void runForTrajectoriesCommand(const QString &, QString cmdfile,
                                      FitTrajectories trajs,
                                      const CommandOptions &opts)
{
  bool addToHistory = false;
  updateFromOptions(opts, "add-to-history", addToHistory);
  bool silent = false;
  updateFromOptions(opts, "silent", silent);
  bool cd = false;
  updateFromOptions(opts, "cd-to-script", cd);

  CommandWidget::ScriptErrorMode mode = CommandWidget::Abort;
  updateFromOptions(opts, "error", mode);

  // WDisableUpdates eff(& soas().view(), silent);

  bool final = true;
  QString t;
  updateFromOptions(opts, "parameters", t);
  if(t == "initial")
    final = false;

  updateFromOptions(opts, "sort", t);
  if(t == "date") {
    trajs.sort();
  }
  if(t == "residuals") {
    trajs.sortByResiduals();
  }

  QString nd;
  if(cd) {
    QFileInfo info(cmdfile);
    nd = info.path();
    cmdfile = info.fileName();
  }
  // TemporarilyChangeDirectory c(nd);
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  QStringList args;

  for(const FitTrajectory & t : trajs) {
    Terminal::out << "Using " << QString(final ? "final" : "initial")
                  << " parameters from trajectory started: "
                  << t.startTime.toString() << " (" 
                  << t.engine << ") -> "
                  << FitTrajectory::endingName(t.ending)
                  << endl;
    ws->restoreParameterValues(final ? t.finalParameters :
                               t.initialParameters);
    soas().prompt().runCommandFile(cmdfile, args, addToHistory, mode);
  }
}

static ArgumentList
rftOpts(QList<Argument *>() 
        << new BoolArgument("silent", 
                            "Silent processing",
                            "whether or not to switch off display updates "
                            "during the script (off by default)")
        << new BoolArgument("add-to-history", 
                            "Add commands to history",
                            "whether the commands run are added to the "
                            "history (defaults to false)")
        << new BoolArgument("cd-to-script", 
                            "cd to script",
                            "If on, automatically change the directory to that oof the script")
        << new ChoiceArgument(QStringList()
                              << "final" << "initial",
                              "parameters", 
                              "Parameters",
                              "which parameters to use")
        << new ChoiceArgument(QStringList()
                              << "date" << "residuals",
                              "sort", 
                              "Sort",
                              "whether to sort the trajectories (by resdiuals, by date)")
        << new TemplateChoiceArgument<CommandWidget::ScriptErrorMode>(CommandWidget::errorModeNames(),
                                                                      "error", 
                                                                      "on error",
                                                                      "Behaviour to adopt on error")
        );

static ArgumentList 
rftArgs(QList<Argument *>() 
        << new FileArgument("file", 
                            "Script", "the script to run")
        << new TrajectoriesArgument("trajectories", 
                                    "Trajectories",
                                    "trajectories to run", true)

        );


// The same, but for the fit context
static Command 
rtf("run-for-trajectories", // command name
     effector(runForTrajectoriesCommand), // action
     "fits",  // group name
     &rftArgs, // arguments
     &rftOpts, 
     "Run commands",
     "Run commands from a file",
     NULL, CommandContext::fitContext());
