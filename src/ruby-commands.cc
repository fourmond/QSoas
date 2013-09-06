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
#include <statistics.hh>
#include <rubysolver.hh>
#include <integrator.hh>


//////////////////////////////////////////////////////////////////////


// Paradoxally, this command isn't really anymore related to Ruby --
// or much less than before

static void applyFormulaCommand(const QString &, QString formula,
                                const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  int extra = 0;
  updateFromOptions(opts, "extra-columns", extra);
  bool useStats = false;
  updateFromOptions(opts, "use-stats", useStats);

  /// @question I'm using a global variable here since it is
  /// impossible for the time being to pass a plain Ruby object
  /// through Expression.

  VALUE oldStats = Qnil;
  if(useStats) {
    oldStats = rb_gv_get("$stats");
    Statistics st(ds);
    ValueHash s;
    QList<ValueHash> sstats = st.statsBySegments(&s);
    VALUE hsh = s.toRuby();
    for(int i = 0; i < sstats.size(); i++) {
      VALUE v = sstats[i].toRuby();
      rb_hash_aset(hsh, INT2FIX(i), v);
      rb_hash_aset(hsh, rb_float_new(i), v);
    }
    rb_gv_set("$stats", hsh);
  }

  Terminal::out << QObject::tr("Applying formula '%1' to buffer %2").
    arg(formula).arg(ds->name) << endl;
  QStringList vars;
  vars << "i" << "seg";

  QStringList colNames;
  colNames << "x" << "y";
  for(int i = 2; i < ds->nbColumns() + extra; i++)
    colNames << QString("y%1").arg(i);

  vars += colNames;

  formula = QString("%1\n%3\n[%2]").
    arg(vars.join("\n")).
    arg(colNames.join(",")).
    arg(formula);


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
      if(j >= ds->nbColumns())
        args[j+2] = 0;
      else
        args[j+2] = ds->column(j)[i];
    }
    exp.evaluateIntoArray(args.data(), ret.data(), ret.size());
    for(int j = 0; j < ret.size(); j++)
      newCols[j].append(ret[j]);
  }
  if(useStats)
    rb_gv_set("$stats", oldStats);

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
                          "Number of extra columns to create")
   << new BoolArgument("use-stats", 
                       "Use statistics",
                       "If on, a $stats hash is available that contains "
                       "statistics")
   );


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
static void eval(const QString &, QString code)
{
  QByteArray bt = code.toLocal8Bit();
  VALUE value = Ruby::run(Ruby::eval, bt);
  Terminal::out << " => " << Ruby::toQString(value) << endl;
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

//////////////////////////////////////////////////////////////////////

static void solve(const QString &, QString formula, double seed, 
                  const CommandOptions & opts)
{
  RubySolver slv(formula);
  
  double rt = 1.0/0.0;
  if(opts.contains("max")) {
    double max = seed;
    updateFromOptions(opts, "max", max);
    rt = slv.solve(seed, max);
  }
  else 
    rt = slv.solve(seed);
  Terminal::out << "Found root at: " << rt << endl;
}

static ArgumentList 
sA(QList<Argument *>() 
   << new StringArgument("formula", 
                         "Formula",
                         "An expression of 1 variable (not an equation !)")
   << new NumberArgument("seed", "Seed", 
                         "Initial X value from which to search")
   );


static ArgumentList 
sO(QList<Argument *>() 
   << new NumberArgument("max", "Max", 
                         "If present, uses dichotomy between seed and max",
                         true)
   );


static Command 
sv("find-root", // command name
   effector(solve), // action
   "file",  // group name
   &sA, // arguments
   &sO, // options
   "Finds a root",
   "Finds a root for the given expression");

//////////////////////////////////////////////////////////////////////

static void integrate(const QString &, QString formula, 
                      double a, double b,
                      const CommandOptions & opts)
{
  Integrator in;

  Expression ex(formula);
  std::function<double (double)> fn = [&ex](double x) -> double {
    return ex.evaluate(&x);
  };
  double err = 0;
  double val = in.integrateSegment(fn, a, b, &err);
  
  Terminal::out << "Integral value: " << val 
                << "\testimated error: " << err << "\t in " 
                << in.functionCalls() << " evaluations" << endl;
}

static ArgumentList 
iA(QList<Argument *>() 
   << new StringArgument("formula", 
                         "Formula",
                         "An expression of 1 variable (not an equation !)")
   << new NumberArgument("a", "a", 
                         "Left bound of the segment")
   << new NumberArgument("b", "b", 
                         "Right bound of the segment")
   );


static ArgumentList iO;


static Command 
in("integrate", // command name
   effector(integrate), // action
   "file",  // group name
   &iA, // arguments
   &iO, // options
   "Integrate expression",
   "Integrate the given expression");
