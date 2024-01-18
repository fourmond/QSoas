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

#include <file.hh>

#include <idioms.hh>
#include <datasetlist.hh>

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

  QString operation = "xyy2";
  updateFromOptions(opts, "mode", operation);


  DataSetList buffers(opts);

  DataStackHelper pusher(opts);
  for(const DataSet * ds : buffers) {
    DataSetExpression ex(ds);
    QStringList colNames;

    ex.useRealColNames = false;
    updateFromOptions(opts, "use-names", ex.useRealColNames);

    int argSize = ex.dataSetParameters(extra, &colNames).size();
    
    ex.useStats = true;
    updateFromOptions(opts, "use-stats", ex.useStats);
    ex.useMeta = true;
    updateFromOptions(opts, "use-meta", ex.useMeta);
    ex.useNames = true;         // But that may be a severe
                                // performance hit ?

    if(operation == "xyy2") {
      // standard mode

      Terminal::out << "Applying formula '" << formula
                    << "' to buffer " << ds->name << endl;

      /// @todo Move this functionality in DataSetExpression  ?
      // We now only lok at

      QStringList locals;
      MRuby * mr = MRuby::ruby();
      mr->detectParameters(formula.toLocal8Bit(), &locals);
      QSet<QString> l = locals.toSet();
      QStringList modified;        // The names of columns modified in
      // order
    
      int indexInEval[colNames.size()];
      for(int i = 0; i < colNames.size(); i++) {
        if(l.contains(colNames[i])) {
          indexInEval[i] = modified.size();
          modified << colNames[i];
        }
        else
          indexInEval[i] = -1;
      }

      int nbm = modified.size();

    
    
      QString finalFormula = QString(nbm != 1 ? "%2\n[%1]" : "%2\n%1").
        arg(modified.join(",")).
        arg(formula);

      // QTextStream o(stdout);
      // o << "Found targets: " << modified.join(", ") << endl;
  

      ex.prepareExpression(finalFormula, extra);



      QList<Vector> newCols;
      for(int i = 0; i < ds->nbColumns() + extra; i++)
        newCols << Vector();
      QStringList newRowNames;

      {
        // QMutexLocker m(&Ruby::rubyGlobalLock);
        QVarLengthArray<double, 100> ret(modified.size());
        QVarLengthArray<double, 100> args(argSize);
        int idx = 0;
        while(ex.nextValues(args.data(), &idx)) {
          bool error = false;
          try {
            if(nbm == 1) 
              ret[0] = ex.expression().
                evaluateNoLock(args.data());
            else
              ex.expression().
                evaluateIntoArrayNoLock(args.data(), ret.data(), ret.size());
            
          }
          catch (const RuntimeError & er) {
            if(er.message().contains("LocalJumpError") &&
               er.message().contains("break")) {
              Terminal::out << "Found break at X = " << ds->x()[idx]
                            << " (#" << idx << "): " << endl;
              break;
            }
            Terminal::out << "Error at X = " << ds->x()[idx]
                          << " (#" << idx << "): " << er.message()
                          << " => "
                          << (keepOnError ? "keeping" : "dropping")
                          << endl;
            for(int i = 0; i < ret.size(); i++)
              ret[i] = 0.0/0.0;
            error = true;
          }
          if(! error || (error && keepOnError)) {
            for(int j = 0; j < newCols.size(); j++) {
              double v = indexInEval[j] >= 0 ? ret[indexInEval[j]] :
                ( j < ds->nbColumns() ? ds->column(j)[idx] : 0);
              /// @todo Make that more efficient
              newCols[j].append(v);
            }
            mrb_value rn = mr->getGlobal("$row_name");
            if(newRowNames.size() > 0 || (!mrb_nil_p(rn))) {
              if(mrb_nil_p(rn))
                newRowNames << "";
              else
                newRowNames << mr->toQString(rn);
            }
          }
        }
      }
  
      DataSet * newDs = ds->derivedDataSet(newCols, "_mod.dat");
      if(newRowNames.size() > 0) {
        if(newDs->rowNames.size() == 0)
          newDs->rowNames << newRowNames;
        else
          newDs->rowNames[0] = newRowNames;
      }
      pusher << newDs;
    }
    else if(operation == "add-column") {

      QString newName;
      updateFromOptions(opts, "name", newName);

      QString name = DataSet::standardNameForColumn(ds->nbColumns());
      if(! newName.isEmpty())
        name += " -> " + newName;
      
      Terminal::out << "Creating new column ("
                    << name
                    << ") using formula '" << formula
                    << "' on buffer " << ds->name << endl;
      ex.prepareExpression(formula);
      Vector newCol;
      {
        QVarLengthArray<double, 100> args(argSize);
        int idx = 0;
        while(ex.nextValues(args.data(), &idx)) {
          double val = std::nan("");
          try {
            val = ex.expression().
              evaluateNoLock(args.data());
          }
          catch (const RuntimeError & er) {
            if(er.message().contains("LocalJumpError") &&
               er.message().contains("break")) {
              Terminal::out << "Found break at X = " << ds->x()[idx]
                            << " (#" << idx << "): " << endl;
              break;
            }
            Terminal::out << "Error at X = " << ds->x()[idx]
                          << " (#" << idx << "): " << er.message()
                          << endl;
          }
          newCol << val;
        }
      }
      QList<Vector> ncls = ds->allColumns();
      for(Vector & col : ncls) {
        if(col.size() > newCol.size())
          col.resize(newCol.size());
      }
      ncls << newCol;
      DataSet * newDs = ds->derivedDataSet(ncls, "_mod.dat");
      if(! newName.isEmpty())
        newDs->setColumnName(newDs->nbColumns()-1, newName);
      pusher << newDs;
    }
    else if(operation == "xyz") {
      Terminal::out << "Applying y=f(x,z) formula '" << formula
                    << "' to buffer " << ds->name << endl;
      ex.xyzMap = true;
      if(extra > 0)
        throw RuntimeError("It is not possible to use /extra-columns with /operation=xyz");
      argSize = ex.dataSetParameters(0).size();
      ex.prepareExpression(formula);
      QVarLengthArray<double, 100> args(argSize);

      int idx, idCol;
      DataSet * newDs = ds->derivedDataSet("_mod.dat");
      Vector perp = ds->perpendicularCoordinates();
      while(perp.size() < ds->nbColumns() - 1)
        perp << perp.size();
    
      while(ex.nextValues(args.data(), &idx, &idCol)) {
        double val = std::nan("0");
        try {
          val = ex.expression().evaluateNoLock(args.data());
        }
        catch (const RuntimeError & er) {
          Terminal::out << "Error at X = " << ds->x()[idx]
                        << ", Z = "
                        << perp[idCol]
                        << ": " << er.message()
                        << endl;
        }
        newDs->column(idCol+1)[idx] = val;
      }
      newDs->setPerpendicularCoordinates(perp);
      pusher << newDs;
    }
    else
      throw RuntimeError("Unknown operation: %1").arg(operation);
  }
}

static ArgumentList 
fA(QList<Argument *>() 
   << new CodeArgument("formula", 
                       "Formula",
                       "formula"));

static ArgumentList 
fO(QList<Argument *>() 
   << DataStackHelper::helperOptions()
   << new ChoiceArgument(QStringList()
                         << "xyy2" << "add-column" << "xyz",
                         "mode", "Mode ",
                         "operating mode used by apply-formula")
   << new StringArgument("name",
                         "New column name",
                         "name of the new column (only in 'add-column' mode)")
   << new IntegerArgument("extra-columns", 
                          "Extra columns",
                          "number of extra columns to create")
   << new BoolArgument("use-stats", 
                       "Use statistics",
                       "if on (by default), you can use `$stats` to refer to statistics (off by default)")
   << new BoolArgument("use-meta", 
                       "Use meta-data",
                       "if on (by default), you can use `$meta` to refer to "
                       "the dataset meta-data")
   << new BoolArgument("use-names", 
                       "Use column names",
                       "if on the columns will not be called x,y, and so on, but will take their name based on the column names")
   << new BoolArgument("keep-on-error", 
                       "Keep on error",
                       "if on, the points where the Ruby expression returns a  error are kept, as invalid numbers")
   << DataSetList::listOptions("Datasets to work on")
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
   << new CodeArgument("formula", 
                       "Formula",
                       "Ruby boolean expression"));

/// @todo Large number of columns must be handled !
static void stripIfCommand(const QString &, QString formula,
                           const CommandOptions & opts)
{

  DataSetList buffers(opts);
  DataStackHelper pusher(opts);
  for(const DataSet * ds : buffers) {
    DataSetExpression ex(ds);
    Terminal::out << QString("Stripping buffer %2 where the data points match '%1' ").
      arg(formula).arg(ds->name) << endl;
    
    ex.useStats = false;
    updateFromOptions(opts, "use-stats", ex.useStats);
    ex.useMeta = true;
    updateFromOptions(opts, "use-meta", ex.useMeta);
    ex.useNames = true;         // But that may be a severe
    // performance hit ?
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
    QList<int> remove;
  
    while(ex.nextValues(args.data(), &idx)) {
      if(ex.expression().evaluateAsBoolean(args.data()))
        remove << idx;
    }

    DataSet * nds = ds->derivedDataSet("_trimmed.dat");
    for(int j = remove.size()-1; j >= 0; j--)
      nds->removeRow(remove[j]);

    Terminal::out << "Removed " << remove.size() << " points" << endl;
    if(nds->nbRows() < threshold) {
      Terminal::out << "-> only " << nds->nbRows() << " points left, not creating a new dataset" << endl;
      delete nds;
    }
    else
      pusher << nds;
  }
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
                         
    << DataStackHelper::helperOptions()
    << DataSetList::listOptions("Datasets to work on")
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
  File f(file, File::TextRead);
  MRuby * mr = MRuby::ruby();
  mr->eval(f);
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
static void eval(const QString &, QStringList codes,
                 const CommandOptions & opts)
{
  bool useDs = true;
  updateFromOptions(opts, "use-dataset", useDs);
  bool modifyMeta = false;
  updateFromOptions(opts, "modify-meta", modifyMeta);
  if(modifyMeta)
    useDs = true;

  DataSetList buffers(opts);

  MRuby * mr = MRuby::ruby();

  auto strValue = [mr](mrb_value value) -> QString {
    QString v = mr->inspect(value);
    if(v[0] == '"')
      return mr->toQString(value);
    return v;
  };
  
  mrb_value value;

  QStringList names;

  QRegExp cnRE("^([\\w-]+):[^:]");

  bool classic = true;
  for(QString & n : codes) {
    if(cnRE.indexIn(n) == 0) {
      names << cnRE.cap(1);
      n = n.mid(cnRE.cap(1).size()+1);
      classic = false;
    }
    else
      names << QString("val_%1").arg(names.size() + 1);
  }
  
  if(codes.size() > 1)
    classic = false;
  if(ValueHash::hasOutputOptions(opts))
    classic = false;
    
  
  
  if(! buffers.dataSetsSpecified()) {
    if(ValueHash::hasOutputOptions(opts))
      Terminal::out << "Warning: cannot use output options without a dataset"
                    << endl;
    for(const QString & code : codes) {
      value = mr->eval(code);
      Terminal::out << " => " << strValue(value) << endl;
    }
  }
  else {
    if(!classic)
      Terminal::out << "buffer\t" << names.join("\t") << endl;
    for(const DataSet * s : buffers) {
      DataSet * ds = const_cast<DataSet *>(s);
      if(classic) {
        Terminal::out << "Evaluating with dataset: " << ds->name << endl;
        value = ds->evaluateWithMeta(codes[0], true, modifyMeta);
        Terminal::out << " => " << strValue(value) << endl;
      }
      else {
        ValueHash val;
        for(int i = 0; i < codes.size(); i++) {
          value = ds->evaluateWithMeta(codes[i], true, modifyMeta);
          val << names[i] << value;
        }
        Terminal::out << ds->name << "\t"
                      << val.toString(QString("\t")) << endl;
        val.handleOutput(ds, opts);
      }
    }
  }
}

static ArgumentList 
eA(QList<Argument *>() 
   << new SeveralCodesArgument("codes",
                               "Code",
                               "Any ruby code"));

static ArgumentList 
eO(QList<Argument *>() 
   << DataSetList::listOptions("Datasets to run eval on")
   << ValueHash::outputOptions()
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
   "Evaluates a Ruby expression and prints the result", "eval-cmd");

//////////////////////////////////////////////////////////////////////

static void verify(const QString &, QString code, const CommandOptions & opts)
{
  bool useDs = true;
  updateFromOptions(opts, "use-dataset", useDs);

  DataSetList buffers(opts);

  MRuby * mr = MRuby::ruby();

  mrb_value value;
  if(! buffers.dataSetsSpecified()) {
    value = mr->eval(code);
    if(! mrb_test(value))
      throw RuntimeError("'%1' is not verified").
        arg(code);

  }
  else {
    for(const DataSet * s : buffers) {
      DataSet * ds = const_cast<DataSet *>(s);
      value = ds->evaluateWithMeta(code, true);
      if(! mrb_test(value))
        throw RuntimeError("'%1' is not verified for dataset %2").
          arg(code).arg(ds->name);
    }
  }
}

static ArgumentList 
vO(QList<Argument *>() 
   << DataSetList::listOptions("Datasets to run verify on")
   << new BoolArgument("use-dataset", 
                       "Use current dataset",
                       "If on (the default) and if there is a current dataset, the $meta and $stats hashes are available")
);


static Command 
ve("verify", // command name
   effector(verify), // action
   "file",  // group name
   &fA, // arguments
   &vO,
   "Verify",
   "Evaluates a Ruby expression and raises an error if it is false");

//////////////////////////////////////////////////////////////////////


#include <linearfunctions.hh>

#include <curveview.hh>
#include <curveitems.hh>
#include <graphicssettings.hh>

#include <gsl-types.hh>

static void linearLeastSquaresCommand(const QString &, QString formula,
                                      const CommandOptions & opts)
{


  DataSetList buffers(opts);
  QString mode = "compute-only";
  updateFromOptions(opts, "mode", mode);
  

  DataStackHelper pusher(opts, false, true,
                         DataStackHelper::HelperFeatures(DataStackHelper::All^
                                                         DataStackHelper::SetMeta));
  ColumnSpecification col("y");
  updateFromOptions(opts, "column", col);
  
  for(const DataSet * ds : buffers) {
    DataSetExpression ex(ds);

    ex.useStats = true;
    updateFromOptions(opts, "use-stats", ex.useStats);
    ex.useMeta = true;
    updateFromOptions(opts, "use-meta", ex.useMeta);

    DataSetExpressionFunction fcn(&ex, formula, ds);
    Terminal::out << "Fitting '" << formula << "' to dataset "
                  << ds->name << "\n"
                  << " ->  found parameters: "
                  << fcn.parameterNames().join(", ") << endl;
    if(fcn.parameters() == 0)
      continue;                 // Nothing to do

    GSLVector res(fcn.parameters());
    GSLVector errs(fcn.parameters());
    /// @todo Hmmm... Is really solve the best name for this ?
    Vector v = col.getColumn(ds);
    double chisq = fcn.solve(v, res, errs);
    ValueHash results;
    for(int i = 0; i < fcn.parameters(); i++)
        results << fcn.parameterNames()[i] << res[i]
                << fcn.parameterNames()[i] + "_err"
                << errs[i];

    results << "residuals" << sqrt(chisq);
    results << "point_residuals" << sqrt(chisq/ds->nbRows());
    results << "chi_sqr" << chisq;
    
    Terminal::out << results.prettyPrint() << endl;
    results.handleOutput(ds, opts);

    if(mode == "compute-only") {
      // Display the results if the dataset is displayed.
      QList<DataSet*> displayed = soas().view().displayedDataSets();
      if(displayed.contains(const_cast<DataSet*>(ds))) {
        CurveData * reg = new CurveData;
        const GraphicsSettings & gs = soas().graphicsSettings();
        reg->pen = gs.getPen(GraphicsSettings::ResultPen);
        reg->xvalues = ds->x();
        reg->yvalues = ds->x();
        fcn.computeFunction(res, reg->yvalues);
        soas().view().addItem(reg, true);
      }
    }
    if(mode == "fits") {
      DataSet * nds = ds->derivedDataSet("_lls_fits.dat");
      fcn.computeFunction(res, v.toGSLVector());
      pusher << nds;
    }
    if(mode == "residuals") {
      DataSet * nds = ds->derivedDataSet("_lls_res.dat");
      fcn.computeFunction(res, v.toGSLVector());
      int idx = col.getValue(ds);
      if(idx < 0)
        throw RuntimeError("Could not set the target column: '%1'").
          arg(col.specification());
      nds->column(idx) -= v;
      nds->column(idx) *= -1;
      pusher << nds;
    }
  }
}

static ArgumentList 
llA(QList<Argument *>() 
    << new CodeArgument("formula", 
                        "Formula",
                        "formula"));

static ArgumentList 
llO(QList<Argument *>() 
    << new BoolArgument("use-stats", 
                        "Use statistics",
                        "if on (by default), you can use `$stats` to refer to statistics (off by default)")
    << new BoolArgument("use-meta", 
                        "Use meta-data",
                        "if on (by default), you can use `$meta` to refer to "
                        "the dataset meta-data")
    << new BoolArgument("use-names", 
                        "Use column names",
                        "if on the columns will not be called x,y, and so on, but will take their name based on the column names")
    << new ChoiceArgument(QStringList() << "compute-only"
                          << "residuals" << "fits",
                          "mode",
                          "Operation mode",
                          "Whether to just compute the parameters or also push residuals or fits")
    << new ColumnArgument("column", 
                          "Y column",
                          "the column to run the regression against (defaults to Y)")
    << DataSetList::listOptions("Buffers to work on")
    << ValueHash::outputOptions()
    << DataStackHelper::helperOptions(DataStackHelper::HelperFeatures(DataStackHelper::All^DataStackHelper::SetMeta))
    );


static Command 
lls("linear-least-squares", // command name
    effector(linearLeastSquaresCommand), // action
    "math",  // group name
    &llA, // arguments
    &llO, // options
    "Linear least squares",
    "determines the values of the parameters by linear least squares");


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

  /// number imprecision
  int imprecisions;

  /// number failed (not true)
  int failed;

  /// number for which there was an exception raised
  int exceptions;

  AssertionsList() : total(0), imprecisions(0),
                     failed(0), exceptions(0) {;};

  void add(const AssertionsList & o) {
    total += o.total;
    imprecisions += o.imprecisions;
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
    Imprecision,
    Failed,
    Exception
  };

  /// Whether or not a tolerance was specified
  bool useTolerance;

  /// Whether the tolerance is tweaked by OS
  bool osSpecific;

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

  SingleAssertion(bool tol, bool os = false) :
    useTolerance(tol),
    osSpecific(os),
    tolerance(-1), value(-1),
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
    case Imprecision:
      res = "imprecision";
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
        o << "\tvalue:" << value << "\n\ttarget:" << tolerance << "\n";
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
  double orgTol = 0.0;
  bool pf = false;
  if(opts.contains("tolerance")) {
    updateFromOptions(opts, "tolerance", tolerance);
    orgTol = tolerance;
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
        std::sort(keys.begin(), keys.end());
      }
      else
        keys = code.split(QRegExp("\\s+"));

      Debug::debug() << "\nTest suite summary:\n";
      for(int i = 0; i < keys.size(); i++) {
        const QString & key = keys[i];
        const AssertionsList & as = assertResults[key];
        Terminal::out << key << ": " << as.total << " total, "
                      << as.failed << " failed, " 
                      << as.imprecisions << " imprecisions, " 
                      << as.exceptions << " exceptions." << endl;
        Debug::debug() << key << ": " << as.total << " total, "
                       << as.failed << " failed, " 
                       << as.imprecisions << " imprecisions, " 
                       << as.exceptions << " exceptions." << endl;
        totl.add(as);
      }
      if(keys.size() > 0) {
        Terminal::out << "Overall: " << totl.total << " total, "
                      << totl.failed << " failed, " 
                      << totl.imprecisions << " imprecisions, " 
                      << totl.exceptions << " exceptions." << endl;
        Debug::debug() << "Overall: " << totl.total << " total, "
                       << totl.failed << " failed, " 
                       << totl.imprecisions << " imprecisions, " 
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
      File f(code, File::TextOverwrite);

      QTextStream o(f);
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
  SingleAssertion as(useTol, pf);
  try {
    value = eval();

    bool check;
    bool imp = false;
    if(useTol) {
      double v = mr->floatValue(value);
      check = fabs(v) <= tolerance;
      if(! check) {
        if(tolerance > 0) {
          if(fabs(v) <= 100 * tolerance)
            imp = true;
        }
        else {
          if(fabs(v) <= 1e-13)  // completely arbitrary
            imp = true;
        }
      }
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
      QString let;
      QString adj;
      if(imp) {
        let = "I";
        adj = "imprecise";
        cur->imprecisions++;
        as.result = SingleAssertion::Imprecision;
      }
      else {
        let = "F";
        adj = "failed";
        cur->failed++;
        as.result = SingleAssertion::Failed;
      }
      if(useTol) {
        Terminal::out << "assertion " << adj << ": " << code
                      << " (should be below: " << tolerance
                      << " but is: " << mr->floatValue(value) << ")" << endl;
        Debug::debug() << let << "(" << as.commandContext.scriptFile
                       << ") : " << code
                       << " (should be below: " << tolerance
                       << " but is: " << mr->floatValue(value) << ")"
                       << context << endl;
      }
      else {
        Terminal::out << "assertion failed: " << code  << endl;
        Debug::debug() << let << "(" << as.commandContext.scriptFile
                       << ") : " << code  << context << endl;
      }
    }
  }
  catch(const Exception & e) {
    cur->exceptions++;
    as.result = SingleAssertion::Failed;
    as.exceptionMessage = e.message();
    Terminal::out << "assertion failed with exception: " << code  << ":\n";
    Terminal::out << e.message() << endl;
    Debug::debug() << "E(" << as.commandContext.scriptFile
                   << ") : " << code  << " -- "
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
   << new CodeArgument("code", 
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



//////////////////////////////////////////////////////////////////////

static void linearSolveCommand(const QString &, QStringList formulas,
                               const CommandOptions & opts)
{

  const DataSet * ds = soas().currentDataSet();

  bool useStats = false;
  updateFromOptions(opts, "use-stats", useStats);
  bool useMeta = false;
  updateFromOptions(opts, "use-meta", useMeta);

  // We only need one expression
  DataSetExpression dse(ds, useStats, useMeta, true);
  dse.prepareExpression("0");

  
  PossessiveList<Expression> expressions;


  int sz = formulas.size();
  QStringList variables;
  for(int i = 0; i < sz; i++)
    variables << QString("a_%1").arg(i+1);

  QStringList allVariables = dse.dataSetParameters();
  int baseIdx = allVariables.size();
  allVariables += variables;

  double tmp[allVariables.size()];

  /// The additional columns created
  QList<Vector> addCols;

  for(const QString & expr : formulas) {
    expressions << new Expression(expr, allVariables);
    addCols << Vector();
  }

  GSLVector vect(sz);
  double solutions[sz];
  gsl_vector_view v = gsl_vector_view_array(solutions, sz);
    
  GSLMatrix m(sz, sz);
  /// @todo Should join
  GSLPermutation perm(sz);
  int idx = 0;
  while(dse.nextValues(tmp, &idx)) {
    // First RHS
    for(int i = 0; i < sz; i++)
      tmp[baseIdx + i] = 0;
    for(int i = 0; i < sz; i++)
      vect[i] = -expressions[i]->evaluate(tmp);

    // Now, the matrix
    for(int i = 0; i < sz; i++) {
      // Set only the coefficient of the ith variable
      for(int j = 0; j < sz ; j++)
        tmp[baseIdx + j] = (i == j ? 1.0 : 0);
      /// Loop over the rows
      for(int j = 0; j < sz; j++) {
        double v = expressions[j]->evaluate(tmp);
        v += vect[j];
        gsl_matrix_set(m, j, i, v);
      }
    }
    int sgn;
    try {
      gsl_linalg_LU_decomp(m, perm, &sgn);
      gsl_linalg_LU_solve(m, perm, vect, &v.vector);
      
      for(int i = 0; i < sz; i++)
        addCols[i] << solutions[i];
    }
    catch(const RuntimeError & re) {
      Terminal::out << "Error at row #" << idx
                    << ": " << re.message() << endl;
      for(int i = 0; i < sz; i++)
        addCols[i] << std::nan("0");
    }
  }

  QList<Vector> cols = ds->allColumns();
  int b = cols.size();
  cols += addCols;
  DataSet * nds = ds->derivedDataSet(cols, "_ls.dat");
  for(int i = 0; i < sz; i++)
    nds->setColumnName(b+i, QString("a_%1").arg(i+1));
  
  soas().pushDataSet(nds);
}


static ArgumentList 
lsA(QList<Argument *>() 
    << new SeveralCodesArgument("equations", 
                                "Equations",
                                "linear equations of the variables, all equal to 0"));


static ArgumentList 
lsO(QList<Argument *>() 
    << new BoolArgument("use-stats", 
                        "Use statistics",
                        "if on (by default), you can use `$stats` to refer to statistics (off by default)")
    << new BoolArgument("use-meta", 
                        "Use meta-data",
                        "if on (by default), you can use `$meta` to refer to "
                        "the dataset meta-data")
    );

static Command 
lsv("linear-solve", // command name
    effector(linearSolveCommand), // action
    "math",  // group name
    &lsA, // arguments
    &lsO,
    "Linear solve");
