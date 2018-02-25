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
#include <terminal.hh>
#include <soas.hh>

#include <utils.hh>
#include <debug.hh>

#include <command.hh>
#include <commandcontext.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>


// static Group fit("fit", 0,
//                  "Fit",
//                  "Commands for fitting");


static void quitCommand(const QString & name)
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

static void saveCommand(const QString & name, QString file)
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

static void loadCommand(const QString & name, QString file)
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

static void fitCommand(const QString & name, const CommandOptions & opts)
{
  int iterations = 50;
  updateFromOptions(opts, "iterations", iterations);
  FitWorkspace::currentWorkspace()->runFit(iterations);
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

static void setCommand(const QString & name, QList<QPair<int, int> > params,
                       QString value, const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  for(QPair<int, int> ps : params)
    ws->setValue(ps.first, ps.second, value);
}

ArgumentList sArgs(QList<Argument*>() 
                   << new FitParameterArgument("parameter", 
                                               "Parameter",
                                               "the parameters of the fit")
                   << new StringArgument("value", 
                                         "Value",
                                         "the value")
                   );

static Command 
set("set", // command name
    effector(setCommand), // action
    "fit",  // group name
    &sArgs, // arguments
    NULL, // options
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
