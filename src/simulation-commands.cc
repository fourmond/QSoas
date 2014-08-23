/*
  simulation-commands.cc: commands to simulate stuff
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014 by CNRS/AMU

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

#include <terminal.hh>

#include <utils.hh>

#include <soas.hh>
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <vector.hh>
#include <dataset.hh>
#include <datastack.hh>
#include <curveview.hh>

#include <rubyodesolver.hh>


static Group simulations("simulations", 10,
                         "Simulations",
                         "Commands producing simulated data");



/// @todo Bring back the old "everything on command-line" approach ?
static void odeComputationCommand(const QString &, QString file,
                                  const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  RubyODESolver solver;

  QFile f(file);
  Utils::open(&f, QIODevice::ReadOnly);

  solver.parseFromFile(&f);

  ODEStepperOptions op = solver.getStepperOptions();
  op.parseOptions(opts);
  solver.setStepperOptions(op);

  QString extra;
  updateFromOptions(opts, "parameters", extra);
  solver.setParameterValues(extra);

  bool annotate = false;
  updateFromOptions(opts, "annotate", annotate);

  Terminal::out << "Solving the ODE for variables " 
                << solver.variables().join(", ") << endl;

  op = solver.getStepperOptions();
  Terminal::out << "Using integrator parameters: " 
                << op.description() << endl;

  QStringList params = solver.extraParameters();
  if(params.size() > 0) {
    QStringList desc;
    const Vector &v = solver.parameterValues();
    for(int i = 0; i < params.size(); i++)
      desc << QString("%1 = %2").
        arg(params[i]).arg(v[i]);
    Terminal::out << "With parameters " << desc.join(", ") << endl;
  }
  
  QList<Vector> cols;

  const Vector & xs = ds->x();
  solver.initialize(xs[0]);

  cols << xs;

  cols << solver.steps(xs, annotate);
  
  DataSet * nds = new DataSet(cols);
  nds->name = "ode.dat";
  Terminal::out << "Total number of function evaluations: " 
                << solver.evaluations << endl;
  soas().stack().pushDataSet(nds);
}

static ArgumentList 
odeArgs(QList<Argument *>() 
        << new FileArgument("file", 
                            "File",
                            "File containing the system")
        );

static ArgumentList 
odeOpts(QList<Argument *>() 
        << new StringArgument("parameters", 
                              "Parameter values",
                              "Values of the extra parameters",
                              true)
        << new BoolArgument("annotate", 
                            "Annotate",
                            "If on, a last column will contain the number of function evaluation for each step")
        << ODEStepperOptions::commandOptions()
        );

static Command 
ode("ode", // command name
    effector(odeComputationCommand), // action
    "simulations",  // group name
    &odeArgs, // arguments
    &odeOpts, // options
    "ODE solver",
    "Solves the given set of ODE",
    "...");
