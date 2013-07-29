/*
  linearkineticsystem.cc: implementation of LinearKineticSystem
  Copyright 2011 by Vincent Fourmond

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
#include <linearkineticsystem.hh>

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

// This command may just be temporary. In any case, it's main purpose
// is to test the kinetic simulations.

static ArgumentList 
kSArgs(QList<Argument *>() 
       << new IntegerArgument("species", 
                              "Number of species")
       << new SeveralNumbersArgument("constants", 
                                     "Kinetic constants")
       );

static ArgumentList 
kSOpts(QList<Argument *>() 
       << new IntegerArgument("samples", 
                              "Number of data points")
       << new StringArgument("base-name", 
                             "Base name for dataset")
       << new NumberArgument("duration", 
                             "Duration of the simulation")
       );

static void kineticSystemCommand(const QString &, int specs,
                                 QList<double> consts,
                                 const CommandOptions & opts)
{
  LinearKineticSystem system(specs);
  QVarLengthArray<double, 30> init(specs);
  QVarLengthArray<double, 900> constants(specs*specs);

  int nbSamples = 1000;
  updateFromOptions(opts, "samples", nbSamples);

  double tend = 10;
  updateFromOptions(opts, "duration", tend);

  QString base = "simulation";
  updateFromOptions(opts, "base-name", base);

  for(int i = 0; i < specs; i++) {
    if(i)
      init[i] = 0;
    else
      init[i] = 1;
  }

  for(int i = 0; i < constants.size(); i++)
    constants[i] = consts.value(i, 0);

  gsl_vector_view v = gsl_vector_view_array(init.data(), specs);
  system.setConstants(constants.data());
  Terminal::out << "Kinetic matrix is: \n"
                << Utils::matrixString(system.kineticMatrixValue()) << endl;
  system.setInitialConcentrations(&v.vector);

  Terminal::out << "Eigenvectors are: \n"
                << Utils::matrixString(system.eigenVectorsValue()) << endl;

  Terminal::out << "Eigenvalues are: \n"
                << Utils::vectorString(system.eigenValuesVector()) << endl;

  // Now, we have all we need;
  
  Vector xvals;
  Vector sum;
  QVarLengthArray<Vector, 30> values(specs);

  for(int i = 0; i < nbSamples; i++) {
    double t = i * tend/(nbSamples - 1);
    xvals << t;
    system.getConcentrations(t, &v.vector);
    for(int j = 0; j < specs; j++)
      values[j] << init[j];
  }


  {
    for(int i = 0; i < specs; i++) {
      QList<Vector> v;
      if(! i)
        sum = values[i];
      else
        sum += values[i];
      v << xvals << values[i];
      DataSet * ds = new DataSet(v);
      ds->name = QString("%1_species_%2.dat").arg(base).arg(i);
      soas().stack().pushDataSet(ds, true); 
      if(i > 0)
        soas().view().addDataSet(ds);
      else
        soas().view().showDataSet(ds);
    }
    QList<Vector> v;
    v << xvals << sum;
    DataSet * ds = new DataSet(v);
    ds->name = QString("%1_species_sum.dat").arg(base);
    soas().stack().pushDataSet(ds, true); 
    soas().view().addDataSet(ds);
  }
}

static Command 
p("linear-kinetic-system", // command name
  effector(kineticSystemCommand), // action
  "simulations",  // group name
  &kSArgs, // arguments
  &kSOpts, // options
  "Linear kinetic system",
  "Computes the concentration of species of a linear kinetic system",
  "...");

//////////////////////////////////////////////////////////////////////



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

  Terminal::out << "Solving the ODE for variables " 
                << solver.variables().join(", ") << endl;
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
  for(int i = -1; i < solver.dimension(); i++)
    cols << Vector();

  const Vector & xs = ds->x();
  solver.initialize(xs[0]);

  for(int i = 0; i < xs.size(); i++) {
    double t = xs[i];
    const double * cvs = solver.currentValues();
    solver.stepTo(t);
    cols[0] << solver.currentTime();
    for(int j = 0; j < solver.dimension(); j++)
      cols[j+1] << cvs[j];
  }
  DataSet * nds = new DataSet(cols);
  nds->name = "ode.dat";
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
