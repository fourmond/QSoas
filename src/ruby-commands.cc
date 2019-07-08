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
#include <commandcontext.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <terminal.hh>
#include <soas.hh>

#include <datastack.hh>
#include <datastackhelper.hh>

#include <dataset.hh>
#include <vector.hh>

#include <debug.hh>
#include <mruby.hh>

#include <expression.hh>
#include <statistics.hh>
#include <rubysolver.hh>
#include <integrator.hh>
#include <multiintegrator.hh>

#include <commandwidget.hh>

#include <datasetexpression.hh>

#include <fitworkspace.hh>
#include <fwexpression.hh>

#include <idioms.hh>

//////////////////////////////////////////////////////////////////////
// Paradoxally, this command isn't really anymore related to Ruby --
// or much less than before

static void applyFormulaCommand(const QString &, QString formula,
                                const CommandOptions & opts)
{


  int extra = 0;
  updateFromOptions(opts, "extra-columns", extra);

  bool keepOnError = false;
  updateFromOptions(opts, "keep-on-error", keepOnError);


  QList<const DataSet *> buffers;
  if(opts.contains("buffers"))
    updateFromOptions(opts, "buffers", buffers);
  else
    buffers << soas().currentDataSet();

  DataStackHelper pusher(opts);
  for(const DataSet * ds : buffers) {
    DataSetExpression ex(ds);
    QStringList colNames;
    int argSize = ex.dataSetParameters(extra, &colNames).size();
    ex.useStats = false;
    updateFromOptions(opts, "use-stats", ex.useStats);
    ex.useMeta = true;
    updateFromOptions(opts, "use-meta", ex.useMeta);

    Terminal::out << QObject::tr("Applying formula '%1' to buffer %2").
      arg(formula).arg(ds->name) << endl;

    QString finalFormula = QString("%2\n[%1]").
      arg(colNames.join(",")).
      arg(formula);

  

    ex.prepareExpression(finalFormula, extra);



    QList<Vector> newCols;
    for(int i = 0; i < ds->nbColumns() + extra; i++)
      newCols << Vector();

    {
      // QMutexLocker m(&Ruby::rubyGlobalLock);
      QVarLengthArray<double, 100> ret(newCols.size());
      QVarLengthArray<double, 100> args(argSize);
      int idx = 0;
      while(ex.nextValues(args.data(), &idx)) {
        bool error = false;
        try {
          ex.expression().
            evaluateIntoArrayNoLock(args.data(), ret.data(), ret.size());
        }
        catch (const RuntimeError & er) {
          Terminal::out << "Error at X = " << ds->x()[idx]
                        << " (#" << idx << "): " << er.message()
                        << " => "
                        << (keepOnError ? "keeping" : "dropping")
                        << endl;
          for(int i = 0; i < ret.size(); i++)
            ret[i] = 0.0/0.0;
          error = true;
        }
        if(! error || (error && keepOnError))
          for(int j = 0; j < ret.size(); j++)
            newCols[j].append(ret[j]);
      }
    }
  
    DataSet * newDs = ds->derivedDataSet(newCols, "_mod.dat");
    pusher << newDs;
  }
}

static ArgumentList 
fA(QList<Argument *>() 
   << new StringArgument("formula", 
                         "Formula",
                         "formula"));

static ArgumentList 
fO(QList<Argument *>() 
   << DataStackHelper::helperOptions()
   << new IntegerArgument("extra-columns", 
                          "Extra columns",
                          "number of extra columns to create")
   << new BoolArgument("use-stats", 
                       "Use statistics",
                       "if on, you can use `$stats` to refer to statistics (off by default)")
   << new BoolArgument("use-meta", 
                       "Use meta-data",
                       "if on (by default), you can use `$meta` to refer to "
                       "the dataset meta-data")
   << new BoolArgument("keep-on-error", 
                       "Keep on error",
                       "if on, the points where the Ruby expression returns a  error are kept, as invalid numbers")
   << new SeveralDataSetArgument("buffers", 
                                 "Buffers",
                                 "Buffers to work on", true, true)
   );


static Command 
load("apply-formula", // command name
     effector(applyFormulaCommand), // action
     "math",  // group name
     &fA, // arguments
     &fO, // options
     "Apply formula",
     "Applies a formula to the current dataset",
     "F");

//////////////////////////////////////////////////////////////////////

static ArgumentList 
tA(QList<Argument *>() 
   << new StringArgument("formula", 
                         "Formula",
                         "Ruby boolean expression"));

/// @todo Large number of columns must be handled !
static void stripIfCommand(const QString &, QString formula,
                           const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  Terminal::out << QString("Stripping buffer %2 where the data points match '%1' ").
    arg(formula).arg(ds->name) << endl;

  DataSetExpression ex(ds);
  ex.useStats = false;
  updateFromOptions(opts, "use-stats", ex.useStats);
  ex.useMeta = true;
  updateFromOptions(opts, "use-meta", ex.useMeta);
  int threshold = 0;
  updateFromOptions(opts, "threshold", threshold);

  int argSize = ex.dataSetParameters().size();

  ex.prepareExpression(formula);

  QList<Vector> newcols;
  for(int i = 0; i < ds->nbColumns(); i++)
    newcols << Vector();
  int dropped = 0;

  QVarLengthArray<double, 100> args(argSize);
  int idx = 0;
  OrderedList segs = ds->segments;
  
  while(ex.nextValues(args.data(), &idx)) {
    if(! ex.expression().evaluateAsBoolean(args.data())) {
      for(int j = 0; j < ds->nbColumns(); j++)
        newcols[j] << ds->column(j)[idx];
    }
    else {
      segs.shiftAbove(newcols[0].size());
      ++dropped;
    }
  }

  DataSet * nds = ds->derivedDataSet(newcols, "_trimmed.dat");
  nds->segments = segs;

  Terminal::out << "Removed " << dropped << " points" << endl;
  if(nds->nbRows() < threshold) {
    Terminal::out << "-> only " << nds->nbRows() << " points left, not creating a new dataset" << endl;
    delete nds;
  }
  else
    soas().pushDataSet(nds);
}

static ArgumentList 
siO(QList<Argument *>() 
    << new BoolArgument("use-stats", 
                        "Use statistics",
                        "if on, you can use `$stats` to refer to statistics (off by default)")
    << new BoolArgument("use-meta", 
                        "Use meta-data",
                        "if on (by default), you can use `$meta` to refer to "
                        "the dataset meta-data")
    << new IntegerArgument("threshold",
                           "Threshold for creating new datasets",
                           "If the stripping operation leaves less than that many points, do not create a new dataset")
                         
    );


static Command 
stripIf("strip-if", // command name
       effector(stripIfCommand), // action
       "buffer",  // group name
       &tA, // arguments
       &siO, // options
       "Strip points",
       "Remove points for which the formula is true", "");

//////////////////////////////////////////////////////////////////////

// This command is actually a real Ruby command!

void rubyRunFile(const QString &, QString file)
{
  QFile f(file);
  Utils::open(&f, QIODevice::ReadOnly | QIODevice::Text);
  MRuby * mr = MRuby::ruby();
  mr->eval(&f);
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

static Command 
lfF("ruby-run", // command name
    optionLessEffector(rubyRunFile), // action
    "file",  // group name
    &rA, // arguments
    NULL, // options
    "Ruby load",
    "Loads and runs a file containing ruby code", "",
    CommandContext::fitContext());

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

/// Solves an equation for the whole buffer !
static void solveDs(const QString &, QString formula, 
                    const CommandOptions & opts)
{

  // We use the current value of y as the seed !
  const DataSet * ds = soas().currentDataSet();

  DataSetExpression ex(ds);

  DataSetExpression * minEx = NULL;
  DataSetExpression * maxEx = NULL;

  QString minF;
  QString maxF;
  updateFromOptions(opts,"min", minF);
  updateFromOptions(opts,"max", maxF);

  if( (! minF.isEmpty()) || (! maxF.isEmpty())) {
    if(minF.isEmpty() || maxF.isEmpty())
      throw RuntimeError("One of min or max specified, but not the other: "
                         "cannot use bisection");
    
    minEx = new DataSetExpression(ds);
    minEx->prepareExpression(minF);
    maxEx = new DataSetExpression(ds);
    maxEx->prepareExpression(maxF);
    
  }
  

  QStringList an = ex.dataSetParameters();
  int argSize = an.size();

  // The index of the value of the y point.
  int x_idx = an.indexOf("x");
  int y_idx = an.indexOf("y");

  Terminal::out << QString("Solving formula '%1' for buffer %2").
    arg(formula).arg(ds->name) << endl;

  ex.prepareExpression(formula);

  Vector newY;

  {
    QVarLengthArray<double, 100> argsb(argSize);
    double * args = argsb.data();
    int idx = 0;
    LambdaSolver slv([=, &ex](double nv) -> double {
        args[y_idx] = nv;
        double v = ex.expression().evaluate(args);
        return v;
      });
    slv.parseOptions(opts);
    while(ex.nextValues(args, &idx)) {
      double nv;
      try {
        if(minEx)
          nv = slv.solve(minEx->expression().evaluate(args),
                         maxEx->expression().evaluate(args));
        else
          nv = slv.solve(args[y_idx]);   // use the current value of Y
                                        // as seed
      }
      catch(const RuntimeError & er) {
        nv = 0.0/0.0;
        Terminal::out << "Could not find a solution for X = "
                      << args[x_idx] << ": " << er.message() << endl;
      }
      newY << nv;
    }
  }

  if(minEx) {
    delete minEx;
    delete maxEx;
  }
            
  
  DataSet * newDs = ds->derivedDataSet(newY, "_slv.dat");
  soas().pushDataSet(newDs);
}

static ArgumentList 
sdA(QList<Argument *>() 
   << new StringArgument("formula", 
                         "Formula",
                         "An expression of the y variable")
   );


static ArgumentList 
sdO(QList<Argument *>() 
    << Solver::commandOptions()
    << new StringArgument("min", 
                          "Minimum",
                          "An expression giving the lower boundary for "
                          "dichotomy approaches")
    << new StringArgument("max", 
                          "Maximum",
                          "An expression giving the upper boundary for "
                          "dichotomy approaches")
    );



static Command 
sdv("solve", // command name
   effector(solveDs), // action
   "math",  // group name
   &sdA, // arguments
   &sdO, // options
   "Solves an equation",
   "Solves the given equation at each point of the current dataset");

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
  bool modifyMeta = false;
  updateFromOptions(opts, "modify-meta", modifyMeta);
  if(modifyMeta)
    useDs = true;

  QList<const DataSet *> buffers;
  updateFromOptions(opts, "buffers", buffers);
  if(buffers.isEmpty() && useDs) {
    const DataSet * ds = soas().currentDataSet(true);
    if(ds)
      buffers << ds; 
  }

  MRuby * mr = MRuby::ruby();

  auto strValue = [mr](mrb_value value) -> QString {
    QString v = mr->inspect(value);
    if(v[0] == '"')
      return mr->toQString(value);
    return v;
  };
  
  mrb_value value;
  if(buffers.isEmpty()) {
    value = mr->eval(code);
    Terminal::out << " => " << strValue(value) << endl;
  }
  else {
    for(const DataSet * s : buffers) {
      DataSet * ds = const_cast<DataSet *>(s);
      Terminal::out << "Evaluating with buffer: " << ds->name << endl;
      if(modifyMeta)
        value = ds->evaluateWithMeta(code, true, true);
      else 
        value = ds->evaluateWithMeta(code, true);
      Terminal::out << " => " << strValue(value) << endl;
    }
  }
}

static ArgumentList 
eA(QList<Argument *>() 
   << new StringArgument("code", 
                         "Code",
                         "Any ruby code"));

static ArgumentList 
eO(QList<Argument *>() 
   << new SeveralDataSetArgument("buffers", 
                                 "Buffers",
                                 "Buffers to run eval on", true, true)
   << new BoolArgument("use-dataset", 
                       "Use current dataset",
                       "If on (the default) and if there is a current dataset, the $meta and $stats hashes are available")
   << new BoolArgument("modify-meta", 
                       "Modify meta",
                       "Reads backs the modifications made to the $meta hash (implies /use-dataset=true)")
);


static Command 
ev("eval", // command name
   effector(eval), // action
   "file",  // group name
   &eA, // arguments
   &eO,
   "Ruby eval",
   "Evaluates a Ruby expression and prints the result", "");

//////////////////////////////////////////////////////////////////////

/// The current context for assertions
static QString assertContext = "general";

/// The current fine context for assertions
static QString assertFineContext;

/// This class represents the coarse list of results of an assertion.
class AssertionsList {
public:

  /// total number
  int total;

  /// number failed (not true)
  int failed;

  /// number for which there was an exception raised
  int exceptions;

  AssertionsList() : total(0), failed(0), exceptions(0) {;};

  void add(const AssertionsList & o) {
    total += o.total;
    failed += o.failed;
    exceptions += o.exceptions;
  };
};

static QHash<QString, AssertionsList> assertResults;

/// This class represents the result of a single assertion
class SingleAssertion {
public:

  enum AssertionResult {
    Pass,
    Failed,
    Exception
  };

  /// Whether or not a tolerance was specified
  bool useTolerance;

  /// The tolerance value
  double tolerance;

  /// The result value
  double value;

  /// The result of the assertion
  AssertionResult result;

  /// The text of the exception, if there was one
  QString exceptionMessage;

  /// The context of the command
  ScriptContext commandContext;

  /// The current assertion context
  QString context;

  /// The current fine context
  QString fineContext;

  SingleAssertion(bool tol) :
    useTolerance(tol), 
    commandContext(soas().prompt().currentContext()),
    context(assertContext), fineContext(assertFineContext)
  {
  };

  /// Returns a YAML-like representation of the assertion. (but it's
  /// not true YAML)
  QString toString() const {
    QString rv;
    QTextStream o(&rv);
    QString res;
    switch(result) {
    case Pass:
      res = "pass";
      break;
    case Exception:
      res = "exception";
      break;
    default:
      res = "failed";
    };
    o << commandContext.scriptFile << ":" << commandContext.lineNumber
      << ": " << res << "\n";
    o << "\tcontext: " << context;
    if(! fineContext.isEmpty())
      o << "/" << fineContext;
    o << "\n";
    if(result == Exception)
      o << "\t" << "message: " << exceptionMessage << "\n";
    else {
      if(useTolerance)
        o << "\tvalue:" << value << " for " << tolerance << "\n";
    }
    return rv;
  };
};

static QList<SingleAssertion> allAssertions;

#ifdef Q_OS_WIN32
#  define QSOAS_PLATFORM_SCALE 3.0
#else
#  define QSOAS_PLATFORM_SCALE 1.0
#endif


static void doAssert(QString code,
                     const CommandOptions & opts,
                     std::function <mrb_value ()> eval)
{
  QString sc ;
  updateFromOptions(opts, "set-context", sc);
  QString dump;
  updateFromOptions(opts, "dump", dump);

  bool useTol = false;
  double tolerance = 0.0;
  bool pf = false;
  if(opts.contains("tolerance")) {
    updateFromOptions(opts, "tolerance", tolerance);
    useTol = true;
    updateFromOptions(opts, "platform-precision", pf);
    int fct = pf ? 1 : 0;
    updateFromOptions(opts, "platform-precision-power", fct);
    while(fct-- > 0)
      tolerance *= QSOAS_PLATFORM_SCALE;
  }


  if(sc == "global") {
    assertContext = code;
    assertFineContext = "";
    Debug::debug() << "\n" << code << ": "; 
    return;
  }
  if(sc == "fine") {
    assertFineContext = code;
    return;
  }

  MRuby * mr = MRuby::ruby();

  if(!dump.isEmpty()) {

    if(dump == "summary") {
      QStringList keys;
      AssertionsList totl;
      if(code == "all") {
        keys = assertResults.keys();
        qSort(keys);
      }
      else
        keys = code.split(QRegExp("\\s+"));

      Debug::debug() << "\nTest suite summary:\n";
      for(int i = 0; i < keys.size(); i++) {
        const QString & key = keys[i];
        const AssertionsList & as = assertResults[key];
        Terminal::out << key << ": " << as.total << " total, "
                      << as.failed << " failed, " 
                      << as.exceptions << " exceptions." << endl;
        Debug::debug() << key << ": " << as.total << " total, "
                       << as.failed << " failed, " 
                       << as.exceptions << " exceptions." << endl;
        totl.add(as);
      }
      if(keys.size() > 0) {
        Terminal::out << "Overall: " << totl.total << " total, "
                      << totl.failed << " failed, " 
                      << totl.exceptions << " exceptions." << endl;
        Debug::debug() << "Overall: " << totl.total << " total, "
                       << totl.failed << " failed, " 
                       << totl.exceptions << " exceptions." << endl;
      }

      // Dump memory use
      
      Terminal::out << "Memory used:" << endl
                    << " -> system: " << Utils::memoryUsed() << " kB"
                    << endl
                    << " -> ruby: " << mr->memoryUse() << endl;
      Debug::debug() << "Memory used:" << endl
                     << " -> system: " << Utils::memoryUsed() << " kB"
                     << endl
                     << " -> ruby: " << mr->memoryUse() << endl;
    }
    else {                      // fine details
      QFile f(code);
      Utils::open(&f, QIODevice::WriteOnly);
      QTextStream o(&f);
      for(int i = 0; i < allAssertions.size(); i++)
        o << allAssertions[i].toString();
    }
    return;
  }


  mrb_value value;
  AssertionsList * cur = &assertResults[assertContext];
  cur->total += 1;
  QString context = "";
  if(! assertFineContext.isEmpty())
    context = QString(" (%1)").arg(assertFineContext);
  SingleAssertion as(useTol);
  try {
    value = eval();

    bool check;
    if(useTol) {
      double v = mr->floatValue(value);
      check = fabs(v) <= tolerance;
      as.value = v;
      as.tolerance = tolerance;
    }
    else
      check = mrb_test(value);
    
    if(check) {
      Terminal::out << "assertion success" << endl;
      Debug::debug() << "." << flush;
      as.result = SingleAssertion::Pass;
    }
    else {
      cur->failed++;
      as.result = SingleAssertion::Failed;
      if(useTol) {
        Terminal::out << "assertion failed: " << code
                      << " (should be below: " << tolerance
                      << " but is: " << mr->floatValue(value) << ")" << endl;
        Debug::debug() << "F: " << code
                       << " (should be below: " << tolerance
                       << " but is: " << mr->floatValue(value) << ")"
                       << context << endl;
      }
      else {
        Terminal::out << "assertion failed: " << code  << endl;
        Debug::debug() << "F: " << code  << context << endl;
      }
    }
  }
  catch(const Exception & e) {
    cur->exceptions++;
    as.result = SingleAssertion::Failed;
    as.exceptionMessage = e.message();
    Terminal::out << "assertion failed with exception: " << code  << ":\n";
    Terminal::out << e.message() << endl;
    Debug::debug() << "E: " << code  << " -- "
                   << e.message() << context << endl;
  }
  allAssertions << as;
}

static void assertCmd(const QString &, QString code,
                      const CommandOptions & opts)
{
  doAssert(code, opts, [code]() -> mrb_value {
      DataSet * ds = soas().currentDataSet(true);
      MRuby * mr = MRuby::ruby();
      if(ds)
        return ds->evaluateWithMeta(code, true);
      else
        return mr->eval(code);
    });
}

static ArgumentList 
aA(QList<Argument *>() 
   << new StringArgument("code", 
                         "Code",
                         "Any ruby code"));

static ArgumentList 
aO(QList<Argument *>() 
   << new ChoiceArgument(QStringList() << "no"
                         << "global"
                         << "fine",
                         "set-context", 
                         "Set assertion context",
                         "If either global or fine, instead of running an assertion, sets the assertion global or fine context")
   << new ChoiceArgument(QStringList() << "no"
                         << "summary"
                         << "details",
                         "dump", 
                         "Print results",
                         "If summary, prints the summary on the terminal. If details, write details to a file")
   << new NumberArgument("tolerance", 
                         "Tolerance",
                         "If given, does not check that the value is true or false, just that its magnitude is smaller than the tolerance", true)
   << new BoolArgument("platform-precision", 
                       "Use 'platform precision'",
                       "Use a platform-specific scaling factor for the tolerance")
   << new IntegerArgument("platform-precision-power", 
                          "Use 'platform precision' to a given power",
                          "Use a platform-specific scaling factor for the tolerance, scaled to the given power")
);



static Command 
as("assert", // command name
   effector(assertCmd), // action
   "file",  // group name
   &aA, // arguments
   &aO,
   "Assert",
   "Runs an assertion in a test suite");

static void fitAssertCmd(const QString &, QString code,
                         const CommandOptions & opts)
{
  doAssert(code, opts, [code]() -> mrb_value {
        FitWorkspace * ws = FitWorkspace::currentWorkspace();
        FWExpression exp(code, ws);
        return exp.evaluate();
    });
}

static Command 
asf("assert", // command name
    effector(fitAssertCmd), // action
    "file",  // group name
    &aA, // arguments
    &aO,
    "Assert",
    "Runs an assertion in a test suite", "", CommandContext::fitContext());
