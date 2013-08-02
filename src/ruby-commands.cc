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
#include <file-arguments.hh>
#include <terminal.hh>
#include <soas.hh>

#include <datastack.hh>

#include <dataset.hh>
#include <vector.hh>

#include <ruby.hh>
#include <ruby-templates.hh>

#include <expression.hh>


//////////////////////////////////////////////////////////////////////


// Paradoxally, this command isn't really anymore related to Ruby --
// or much less than before

static void applyFormulaCommand(const QString &, QString formula,
                                const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  int extra = 0;
  updateFromOptions(opts, "extra-columns", extra);

  Terminal::out << QObject::tr("Applying formula '%1' to buffer %2").
    arg(formula).arg(ds->name) << endl;
  QStringList vars;
  vars << "i" << "seg";

  QStringList colNames;
  colNames << "x" << "y";
  for(int i = 2; i < ds->nbColumns() + extra; i++)
    colNames << QString("y%1").arg(i);

  formula = QString("%3\n[%2]").
    arg(colNames.join(",")).
    arg(formula);

  vars += colNames;

  Expression exp(formula, vars);

  QList<Vector> newCols;
  for(int i = 0; i < ds->nbColumns() + extra; i++)
    newCols << Vector();
  
  int size = ds->x().size();
  int seg = 0;
  for(int i = 0; i < size; i++) {
    QVarLengthArray<double, 50> args(newCols.size() + 2);
    QVarLengthArray<double, 50> ret(newCols.size());

    while(seg < ds->segments.size() && i >= ds->segments[seg])
      seg++;
    
    args[0] = i;                // the index !
    args[1] = seg;

    for(int j = 0; j < newCols.size(); j++) {
      if(j > ds->nbColumns())
        args[j+2] = 0;
      else
        args[j+2] = ds->column(j)[i];
    }
    exp.evaluateIntoArray(args.data(), ret.data(), ret.size());
    for(int j = 0; j < ret.size(); j++)
      newCols[j].append(ret[j]);
  }

  DataSet * newDs = ds->derivedDataSet(newCols, "_mod.dat");
  soas().pushDataSet(newDs);
}

static ArgumentList 
fA(QList<Argument *>() 
   << new StringArgument("formula", 
                         "Formula",
                         "Formula (valid Ruby code)"));

static ArgumentList 
fO(QList<Argument *>() 
   << new IntegerArgument("extra-columns", 
                          "Extra columns",
                          "Number of extra columns to create"));


static Command 
load("apply-formula", // command name
     effector(applyFormulaCommand), // action
     "buffer",  // group name
     &fA, // arguments
     &fO, // options
     "Apply formula",
     "Applies a formula to the current dataset",
     "...",
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
  soas().pushDataSet(ds->derivedDataSet(newcols, "_trimmed.dat"));
}


static Command 
stripIf("strip-if", // command name
       optionLessEffector(stripIfCommand), // action
       "buffer",  // group name
       &tA, // arguments
       NULL, // options
       "Strip points",
       "Remove points for which the formula is true", "");

//////////////////////////////////////////////////////////////////////

// This command is actually a real Ruby command!

void rubyRunFile(const QString &, QString file)
{
  QFile f(file);
  Utils::open(&f, QIODevice::ReadOnly | QIODevice::Text);
  QByteArray bt = f.readAll();
  Ruby::run(Ruby::eval, bt);
}

static ArgumentList 
rA(QList<Argument *>() 
   << new FileArgument("file", 
                       "File",
                       "Ruby file to load"));


static Command 
lf("ruby-run", // command name
   optionLessEffector(rubyRunFile), // action
   "file",  // group name
   &rA, // arguments
   NULL, // options
   "Ruby load",
   "Loads and runs a file containing ruby code", "");

//////////////////////////////////////////////////////////////////////

/// @todo This function could make many information available (stats
/// about the current buffer and so on).
///
/// @todo There should also be a way to store the result somehow
/// (although using global variables is always possible !)
void eval(const QString &, QString code)
{
  QByteArray bt = code.toLocal8Bit();
  VALUE value = Ruby::run(Ruby::eval, bt);
  Terminal::out << " => " << Ruby::toString(value) << endl;
}

static ArgumentList 
eA(QList<Argument *>() 
   << new StringArgument("code", 
                         "Code",
                         "Any ruby code"));


static Command 
ev("eval", // command name
   optionLessEffector(eval), // action
   "file",  // group name
   &eA, // arguments
   NULL, // options
   "Ruby eval",
   "Evaluates a Ruby expression and prints the result", "");
