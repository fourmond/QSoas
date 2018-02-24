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
 
