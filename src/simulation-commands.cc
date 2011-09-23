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

#include <soas.hh>
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <vector.hh>
#include <dataset.hh>
#include <datastack.hh>
#include <curveview.hh>



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

static void kineticSystemCommand(const QString &, int specs,
                                 QList<double> consts)
{
  LinearKineticSystem system(specs);
  QVarLengthArray<double, 30> init(specs);
  QVarLengthArray<double, 900> constants(specs*specs);

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
                << system.kineticMatricText() << endl;
  system.setInitialConcentrations(&v.vector);

  // Now, we have all we need;
  
  Vector xvals;
  QVarLengthArray<Vector, 30> values(specs);
  for(int i = 0; i < 100; i++) {
    double t = i * 0.1;         // ??
    xvals << t;
    system.getConcentrations(t, &v.vector);
    for(int j = 0; j < specs; j++)
      values[j] << init[j];
  }
  

  {
    for(int i = 0; i < specs; i++) {
      QList<Vector> v;
      v << xvals;
      v << values[i];
      DataSet * ds = new DataSet(v);
      ds->name = QString("simulations_species_%1.dat").arg(i);
      soas().stack().pushDataSet(ds, true); 
      if(i > 0)
        soas().view().addDataSet(ds);
      else
        soas().view().showDataSet(ds);
    }
  }
}

/// @todo this should join a proper "simulations" group
static Command 
p("kinetic-system", // command name
  optionLessEffector(kineticSystemCommand), // action
  "simulations",  // group name
  &kSArgs, // arguments
  NULL, // options
  "Linear kinetic system",
  "Computes the concentration of species of a linear kinetic system",
  "...");
