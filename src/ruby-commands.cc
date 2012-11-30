/**
   \file ruby-commands.cc Ruby-related commands
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
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <terminal.hh>
#include <soas.hh>

#include <datastack.hh>

#include <dataset.hh>
#include <vector.hh>

#include <expression.hh>


//////////////////////////////////////////////////////////////////////


// Paradoxally, this command isn't really anymore related to Ruby --
// or much less than before

static ArgumentList 
fA(QList<Argument *>() 
   << new StringArgument("formula", 
                         "Formula",
                         "Formula (valid Ruby code)"));

static void applyFormulaCommand(const QString &, QString formula)
{
  const DataSet * ds = soas().currentDataSet();
  Terminal::out << QObject::tr("Applying formula '%1' to buffer %2").
    arg(formula).arg(ds->name) << endl;
  QStringList vars;
  vars << "x" << "y";              /// @todo handle the case of a
                                   /// larger number of columns

  formula = QString("%1\n%3\n[%2]").
    arg(vars.join("\n")).       /// @todo This is hackish, but, well
                                /// it should work !
    arg(vars.join(",")).
    arg(formula);

  Expression exp(formula, vars);
  
  Vector newX, newY;
  {
    const Vector & xc = ds->x();
    const Vector & yc = ds->y();
    double vars[2];
    double values[2];
    for(int i = 0; i < xc.size(); i++) {
      vars[0] = xc[i];
      vars[1] = yc[i];
      exp.evaluateIntoArray(vars, values, 2);
      newX << values[0];
      newY << values[1];
    }
  }

  DataSet * newDs = new DataSet(QList<Vector>() << newX << newY);
  newDs->name = ds->cleanedName() + "_mod.dat";
  soas().pushDataSet(newDs);
}


static Command 
load("apply-formula", // command name
     optionLessEffector(applyFormulaCommand), // action
     "stack",  // group name
     &fA, // arguments
     NULL, // options
     "Apply formula",
     "Applies a formula to the current dataset",
     QT_TR_NOOP("Applies a formula to the current dataset. "
                "The formula must be valid Ruby code."),
     "F");

//////////////////////////////////////////////////////////////////////

static ArgumentList 
tA(QList<Argument *>() 
   << new StringArgument("formula", 
                         "Formula",
                         "Ruby boolean expression"));

/// @todo Large number of columns must be handled !
static void stripIfCommand(const QString &, QString formula)
{
  const DataSet * ds = soas().currentDataSet();
  Terminal::out << QObject::tr("Applying formula '%1' to buffer %2").
    arg(formula).arg(ds->name) << endl;
  QStringList vars;
  vars << "x" << "y";              // @todo handle the case of a
                                   // larger number of columns

  Expression exp(formula, vars);

  QList<Vector> newcols;
  for(int i = 0; i < ds->nbColumns(); i++)
    newcols << Vector();
  int dropped = 0;
  {
    const Vector & xc = ds->x();
    QVarLengthArray<double, 1000> vars(ds->nbColumns());
    for(int i = 0; i < xc.size(); i++) {
      for(int j = 0; j < ds->nbColumns(); j++)
        vars[j] = ds->column(j)[i];
      if(! exp.evaluateAsBoolean(vars.data())) {
        for(int j = 0; j < ds->nbColumns(); j++)
          newcols[j] << vars[j];
      }
      else
        ++dropped;
    }
  }

  Terminal::out << "Removed " << dropped << " points" << endl;
  DataSet * newDs = new DataSet(newcols);
  newDs->name = ds->cleanedName() + "_trimmed.dat";
  soas().pushDataSet(newDs);
}


static Command 
stripIf("strip-if", // command name
       optionLessEffector(stripIfCommand), // action
       "stack",  // group name
       &tA, // arguments
       NULL, // options
       "Strip points",
       "Remove points for which the formula is true", "");
