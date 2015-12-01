/**
   \file ruby-commands.cc Ruby-related commands
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
#include <multiintegrator.hh>

#include <idioms.hh>

//////////////////////////////////////////////////////////////////////

static QStringList prepareArgs(const DataSet * ds, int extra = 0,
                               QStringList * cn = NULL)
{
  QStringList vars;
  vars << "i" << "seg";

  QStringList colNames;
  colNames << "x" << "y";
  for(int i = 2; i < ds->nbColumns() + extra; i++)
    colNames << QString("y%1").arg(i);

  vars += colNames;
  if(cn)
    *cn = colNames;

  return vars;
}

static void loopOverDataset(const DataSet * ds, 
                            std::function<void (double * args, 
                                                double * cols)> loop, 
                            int extra = 0)
{
  int size = ds->x().size();
  int seg = 0;
  int origCols = ds->nbColumns();
  int columns = origCols + extra;
  QVarLengthArray<double, 50> args(columns + 2);
  for(int i = 0; i < size; i++) {
    while(seg < ds->segments.size() && i >= ds->segments[seg])
      seg++;
    args[0] = i;                // the index !
    args[1] = seg;

    for(int j = 0; j < columns; j++) {
      if(j >= origCols)
        args[j+2] = 0;
      else
        args[j+2] = ds->column(j)[i];
    }
    loop(args.data(), args.data() + 2);
  }
}


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
  bool useMeta = true;
  updateFromOptions(opts, "use-meta", useMeta);

  /// @question I'm using a global variable here since it is
  /// impossible for the time being to pass a plain Ruby object
  /// through Expression.

  SaveGlobal a("$stats");
  if(useStats) {
    Statistics st(ds);
    rb_gv_set("$stats", st.toRuby());
  }

  SaveGlobal b("$meta");
  if(useMeta)
    rb_gv_set("$meta", ds->getMetaData().toRuby());


  Terminal::out << QObject::tr("Applying formula '%1' to buffer %2").
    arg(formula).arg(ds->name) << endl;

  QStringList colNames;
  QStringList vars = prepareArgs(ds, extra, &colNames);

  formula = QString("%1\n%3\n[%2]").
    arg(vars.join("\n")).
    arg(colNames.join(",")).
    arg(formula);


  Expression exp(formula, vars);

  QList<Vector> newCols;
  for(int i = 0; i < ds->nbColumns() + extra; i++)
    newCols << Vector();

  QVarLengthArray<double, 50> ret(newCols.size());
  loopOverDataset(ds, [&newCols, &exp, &ret](double *args, double *) {
      exp.evaluateIntoArray(args, ret.data(), ret.size());
      for(int j = 0; j < ret.size(); j++)
        newCols[j].append(ret[j]);
    }, extra);

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
   << new BoolArgument("use-meta", 
                       "Use meta-data",
                       "If on (by default), a meta hash is available that contains "
                       "the dataset meta-data")
   );


static Command 
load("apply-formula", // command name
     effector(applyFormulaCommand), // action
     "math",  // group name
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

/// This function is called from other places, but this is just a
/// workaround before a decent dataset-formula-looping class is
/// written.
DataSet * stripDataset(const DataSet * ds, const QString & formula, 
                       int * nbDropped)
{
  QStringList vars = prepareArgs(ds);

  Expression exp(formula, vars);

  QList<Vector> newcols;
  for(int i = 0; i < ds->nbColumns(); i++)
    newcols << Vector();
  int dropped = 0;

  /// @todo Fix segments !
  QList<int> segs;
  int lastSeg = 0;
  loopOverDataset(ds, [&, ds] (double * args, 
                               double * data){
      if( (int) args[1] > lastSeg) {
        segs << ds->segments[lastSeg] - dropped;
        lastSeg = args[1];
      }
      if(! exp.evaluateAsBoolean(args)) {
        for(int j = 0; j < ds->nbColumns(); j++)
          newcols[j] << data[j];
      }
      else
        ++dropped;
    });
  DataSet * nds = ds->derivedDataSet(newcols, "_trimmed.dat");
  nds->segments = segs;
  if(nbDropped)
    *nbDropped = dropped;
  return nds;
}

/// @todo Large number of columns must be handled !
static void stripIfCommand(const QString &, QString formula)
{
  const DataSet * ds = soas().currentDataSet();
  Terminal::out << QObject::tr("Stripping buffer %2 where the data points match '%1' ").
    arg(formula).arg(ds->name) << endl;

  int dropped = 0;
  DataSet * nds = stripDataset(ds, formula, &dropped);

  Terminal::out << "Removed " << dropped << " points" << endl;
  soas().pushDataSet(nds);
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
  QScopedPointer<Integrator> in(Integrator::fromOptions(opts));

  Expression ex(formula);
  std::function<double (double)> fn = [&ex](double x) -> double {
    return ex.evaluate(&x);
  };
  double err = 0;
  double val = in->integrateSegment(fn, a, b, &err);
  
  Terminal::out << "Integral value: " << QString::number(val, 'g', 10)
                << "\testimated error: " << err << "\t in " 
                << in->functionCalls() << " evaluations over " 
                << in->intervals() << " intervals " << endl;
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


static ArgumentList 
iO(QList<Argument *>()
   << Integrator::integratorOptions()
   );



static Command 
in("integrate-formula", // command name
   effector(integrate), // action
   "file",  // group name
   &iA, // arguments
   &iO, // options
   "Integrate expression",
   "Integrate the given expression");

//////////////////////////////////////////////////////////////////////

static void mintegrate(const QString &, QString formula,
                       double a, double b,
                       const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  QStringList nargs;
  nargs << "x" << "z";

  Expression ex(formula, nargs);
  const Vector &xvals = ds->x();
  
  std::function<void (double, gsl_vector *)> fn = [&ex, xvals](double x, gsl_vector * tg) -> void {
    double args[2];
    args[1] = x;
    for(int i = 0; i < xvals.size(); i++) {
      args[0] = xvals[i];
      gsl_vector_set(tg, i, ex.evaluate(args));
    }
  };
  
  QScopedPointer<MultiIntegrator> in(MultiIntegrator::fromOptions(opts, fn, xvals.size()));
  Vector tg = xvals;
  gsl_vector * gl = tg.toGSLVector();
  double err = in->integrate(gl, a, b);
  DataSet *nv = new DataSet(xvals, tg);
  nv->name = "integrated.dat";
  soas().pushDataSet(nv);

  Terminal::out << "Integration used " << in->evaluationNumbers()
                << " function calls.\n The error estimation is " << err << endl;
}

static ArgumentList 
miA(QList<Argument *>() 
   << new StringArgument("formula", 
                         "Formula",
                         "An expression of x and z")
   << new NumberArgument("a", "a", 
                         "Lower Z value")
   << new NumberArgument("b", "b", 
                         "Upper Z value")
   );


static ArgumentList 
miO(QList<Argument *>()
   << MultiIntegrator::integratorOptions()
   );



static Command 
min("mintegrate-formula", // command name
   effector(mintegrate), // action
   "file",  // group name
   &miA, // arguments
   &miO, // options
   "Integrate expression",
   "Integrate the given expression");

//////////////////////////////////////////////////////////////////////

/// @todo There should also be a way to store the result somehow
/// (although using global variables is always possible !)
static void eval(const QString &, QString code, const CommandOptions & opts)
{
  bool useDs = true;
  updateFromOptions(opts, "use-dataset", useDs);
  
  DataSet * ds = (useDs ? soas().currentDataSet(true) : NULL);
  VALUE value;
  if(ds)
    value = ds->evaluateWithMeta(code, true);
  else {
    QByteArray bt = code.toLocal8Bit();
    value = Ruby::run(Ruby::eval, bt);
  }

  Terminal::out << " => " << Ruby::inspect(value) << endl;
}

static ArgumentList 
eA(QList<Argument *>() 
   << new StringArgument("code", 
                         "Code",
                         "Any ruby code"));

static ArgumentList 
eO(QList<Argument *>() 
   << new BoolArgument("use-dataset", 
                       "Use current dataset",
                       "If on (the default) and if there is a current dataset, the $meta and $stats hashes are available")
);


static Command 
ev("eval", // command name
   effector(eval), // action
   "file",  // group name
   &eA, // arguments
   &eO,
   "Ruby eval",
   "Evaluates a Ruby expression and prints the result", "");
