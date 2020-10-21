/**
   \file custom-fits.cc custom fits
   Copyright 2011 by Vincent Fourmond
             2012, 2013 by CNRS/AMU

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

#include <perdatasetfit.hh>
#include <dataset.hh>
#include <vector.hh>
#include <fitdata.hh>
#include <timedependentparameters.hh>


#include <expression.hh>
#include <solver.hh>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_math.h>



/// This class 
class ImplicitFitBase {
public:

  static QStringList splitFormulas(const QString & fms) {
    return fms.split("|");
  };

  /// The formula
  QString formula;

  /// The expressions being used !
  Expression * expression;

  /// The parameters (the final ones)
  QStringList params;

  /// Whether or not the fit has a temperature-related parameter.
  bool hasTemperature;

  /// Parameters that default to "fixed"
  QSet<QString> fixedParameters;

  /// The number of parameters that look like x_i  (with i != 0)
  int nbXSteps;

  /// The number of parameters that look like y_i  (with i != 0)
  int nbYSteps;

  /// The time dependent parameters of the fit.
  TimeDependentParameters timeDependentParameters;

  /// The set of indices which should be skipped while reading from
  /// the fit parameters + parametersBase to fill the target system
  /// parameters.
  QSet<int> skippedIndices;

  /// The solver
  LambdaSolver solver;

  ImplicitFitBase() :
    expression(NULL),
    solver(0)
  {
  };

  ImplicitFitBase(const ImplicitFitBase & o) :
    expression(NULL),
    solver(o.solver)
  {
    if(! o.formula.isEmpty())
      parseFormula(o.formula);
  };

  static ArgumentList * hardOptions() {
    return new
      ArgumentList(QList<Argument *>()
                   << new TDPArgument("with", 
                                      "Time dependent parameters",
                                      "Make certain parameters depend "
                                      "upon time")
                   );
    
  };

  static QList<Argument*> softOptions() {
    return Solver::commandOptions();
  };

  void parseFormula(const QString &form)
  {
    formula = form;
    params.clear();
    nbXSteps = 0;
    nbYSteps = 0;
    fixedParameters.clear();

    delete expression;

    QStringList naturalParameters;
    Expression s(formula);
    naturalParameters =  s.naturalVariables();


    std::sort(naturalParameters.begin(), naturalParameters.end());


    params << "x" << "i" << "seg" << "y";
    if(naturalParameters.contains("fara") ||
       naturalParameters.contains("temperature")) {
      hasTemperature = true;
      params << "fara" << "temperature";
    }
    else
      hasTemperature = false;
    params += naturalParameters;

    Utils::makeUnique(params);
    expression = new Expression(formula, params);

    for(int i = 0; i < (hasTemperature ? 5 : 4); i++)
      params.takeFirst();

    /// @todo Gather the logic for finding pre-determined values for
    /// parameters in a separate class to be used for custom fits and
    /// also possibly reparametrized fits. This could include custom
    /// expressions for setting the default values, and possibly
    /// giving defaults for whether the parameters are fixed or not.
    
    // Now a little post-processing.
    for(const QString & n : params) {
      if(n == "temperature")
        fixedParameters.insert(n);
      if(n == "dx")
        fixedParameters.insert(n);
      QRegExp re("([xy])_([0-9]+)");
      if(re.exactMatch(n)) {
        if(re.cap(2) == "0")
          fixedParameters.insert(n);
        else {
          int idx = re.cap(2).toInt();
          if(re.cap(1) == "x") {
            fixedParameters.insert(n);
            nbXSteps = std::max(nbXSteps, idx);
          }
          else 
            nbYSteps = std::max(nbYSteps, idx);
        }
      }
    }
    
  }

  /// Compute the time-dependent stuff...
  void processOptions(const CommandOptions & opts)
  {
    QStringList lst;
    updateFromOptions(opts, "with", lst);

    // Get rid of all time-dependent parameters first.
    timeDependentParameters.clear();
    timeDependentParameters.parseFromStrings(lst, [this](const QString & n) {
        return params.indexOf(n);
      });
    skippedIndices = timeDependentParameters.keys().toSet();

    // And now, remove them from the parameters !
    for(int i = params.size()-1; i >=0; i--)
      if(skippedIndices.contains(i))
        params.takeAt(i);
    processSoftOptions(opts);
  }

  /// Processes the soft options -- solver options.
  void processSoftOptions(const CommandOptions & opts) {
    solver.parseOptions(opts);
  };

  CommandOptions currentSoftOptions() const {
    return solver.currentOptions();
  };


  /// Returns the initial guess for the named parameter
  ///
  /// @todo Should join a special class for that.
  double paramGuess(const QString & name, const DataSet * ds) const {
    if(name == "temperature")
      return soas().temperature();
    QRegExp re("([xy])_([0-9]+)");
    if(re.exactMatch(name)) {
      int idx = re.cap(2).toInt();
      int nbt;
      double l, r;
      if(re.cap(1) == "x") {
        l = ds->x().first();
        r = ds->x().last();
        nbt = nbXSteps;
      }
      else {
        l = ds->y().first();
        r = ds->y().last();
        nbt = nbYSteps;
      }
      double d = (r - l)/(nbt + 1);
      return l + d*idx;
    }
    else {
      if(name == "dx")
        return ds->x()[1] - ds->x()[0];
    }
    return 1;
  };

  /// Returns whether the parameter is fixed by default
  bool paramFixed(const QString & name) const {
    return fixedParameters.contains(name);
  }

  void initialGuessForDataset(const DataSet * ds,
                              double * a) const
  {
    for(int i = 0; i < params.size(); i++)
      a[i] = paramGuess(params[i], ds);
    timeDependentParameters.setInitialGuesses(a + params.size(), ds);
  };

  QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> p;
    for(int i = 0; i < params.size(); i++) {
      p << ParameterDefinition(params[i], paramFixed(params[i]));
    }
    p += timeDependentParameters.fitParameters();
    return p;
  };

  /// Computes the function for the given dataset, assuming that the
  /// parameters are those for the given dataset. 
  void computeDataSet(const double *a, 
                      const DataSet * ds,
                      FitData *data, 
                      gsl_vector *target) {
    int k = 0;
    int nbparams = data->parametersPerDataset() +
      skippedIndices.size();

    int seg = 0;
    int base = (hasTemperature ? 5 : 4);
    QVarLengthArray<double, 100> args(base + nbparams);
    args[0] = 0; // x
    args[1] = 0; // i
    args[2] = 0; // seg
    args[3] = 0; // y
    if(hasTemperature)
      args[4] = GSL_CONST_MKSA_FARADAY/ 
        (a[0] * GSL_CONST_MKSA_MOLAR_GAS);

    for(int i = 0; i < nbparams; i++)
      Utils::skippingCopy(a, args.data()+base, nbparams, skippedIndices);

    timeDependentParameters.initialize(a + params.size());

    solver.setFunction([&args,this](double y) -> double {
                          args[3] = y;
                          return expression->evaluate(args.data());
                       }
      );

    const Vector &xv = ds->x();
    const Vector &yv = ds->y();

    // whether we have already found one solution
    bool found = false;
    double lastFound = 0;

    QSet<int> notFound;

    for(int j = 0; j < xv.size(); j++) {
      while(seg < ds->segments.size() && j >= ds->segments[seg])
        seg++;

      /// @todo Use DatasetExpression ?
      args[0] = xv[j]; // x
      args[1] = j;     // i !
      args[2] = seg;
      
      timeDependentParameters.computeValues(args[0], args.data()+base,
                                            a + params.size());


      double val = 0;
      auto tryVal = [this,&val](double seed) -> bool {
                      try {
                        val = solver.solve(seed);
                      }
                      catch(const RuntimeError & re) {
                        return false;
                      }
                      return true;
                    };
      if((found && tryVal(lastFound))
         || tryVal(yv[j])
         || tryVal(xv[j])
         || tryVal(-xv[j])
         || tryVal(1)
         ) {
        gsl_vector_set(target, k++, val);
        lastFound = val;
        found = true;
      }
      else {
        notFound.insert(j);
      }
    }
    if(! found)
      throw RuntimeError("Could not find any proper seed for solving");

    for(int j : notFound) {
      while(seg < ds->segments.size() && j >= ds->segments[seg])
        seg++;

      /// @todo Use DatasetExpression ?
      args[0] = xv[j]; // x
      args[1] = j;     // i !
      args[2] = seg;
      
      timeDependentParameters.computeValues(args[0], args.data()+base,
                                            a + params.size());
      double seed = 0;
      if(j > 0)
        seed = gsl_vector_get(target, j-1);
      else {
        for(int s = 1; s < xv.size(); s++) {
          if(! notFound.contains(s)) {
            seed = gsl_vector_get(target, s);
            break;
          }
        }
      }
      try {
        gsl_vector_set(target, j, solver.solve(seed));
      }
      catch(const RuntimeError & re) {
        throw RuntimeError("Could not solve at X = %1: %2").
          arg(xv[j]).arg(re.message());
      }
    }
  };

};

//////////////////////////////////////////////////////////////////////

/// This class holds a fit using a single formula, that hence applies
/// naturally to several datasets. There is no 'arb' fit from that
class ImplicitFit : public PerDatasetFit {

  class Storage : public FitInternalStorage {
  public:
    ImplicitFitBase fb;
  };

  QString formulaString;

  ImplicitFitBase * getFb(FitData * data) const {
    return &storage<Storage>(data)->fb;
  };

protected:
  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds*/) const override {
    return deepCopy<Storage>(source);
  };

protected:

  virtual QString optionsString(FitData * data) const override {
    ImplicitFitBase * f = getFb(data);
    return "formula: " + f->formula;
  };

  /// @hack get rid of the const-cast, this isn't very clean
  virtual void processOptions(const CommandOptions &opts,
                              FitData * data) const override {
    ImplicitFitBase * f = getFb(data);
    if(! formulaString.isEmpty())
      f->parseFormula(formulaString);
    
    f->processOptions(opts);
  };


  void runFit(const QString & name, QString formula, 
              QList<const DataSet *> datasets,
              const CommandOptions & opts)
  {

    Fit::runFit([this, &formula] (FitData * data) {
                  ImplicitFitBase * f = getFb(data);
                  Terminal::out << "Fitting using formula '" << formula
                                << "'" << endl;
                  f->parseFormula(formula);
                  Terminal::out << " -> detected parameters:  "
                                << f->params.join(", ") 
                                << endl;
      }, name, datasets, opts);
  }
  
  void computeFit(const QString & name, QString formula,
                  QString params,
                  QList<const DataSet *> datasets,
                  const CommandOptions & opts)
  {
    Fit::computeFit([this, &formula](FitData * data) {
                      ImplicitFitBase * f = getFb(data);
                      Terminal::out << "Computing using formula '"
                                    << formula << "'" << endl;
                      f->parseFormula(formula);
                      Terminal::out << " -> detected parameters:  "
                                    << f->params.join(", ") 
                                    << endl;
                    }, name, params, datasets, opts);
  }

  void runFitCurrentDataSet(const QString & n, 
                            QString formula, const CommandOptions & opts)
  {
    QList<const DataSet *> ds;
    ds << soas().currentDataSet();
    runFit(n, formula, ds, opts);
  }


    
public:
  
  ArgumentList * fitHardOptions() const override {
    return ImplicitFitBase::hardOptions();
  };

  ArgumentList * fitSoftOptions() const override {
    return new
      ArgumentList(QList<Argument *>()
                   << ImplicitFitBase::softOptions()
                   );
  };

  virtual void function(const double *parameters, FitData *data, 
                        const DataSet *ds, gsl_vector *target) const override {
    ImplicitFitBase * f = getFb(data);
    f->computeDataSet(parameters, ds, data, target);
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    ImplicitFitBase * f = getFb(data);
    f->initialGuessForDataset(ds, a);
  };
  
  CommandOptions currentSoftOptions(FitData * data) const override {
    ImplicitFitBase * f = getFb(data);
    return f->currentSoftOptions();
  };

  void processSoftOptions(const CommandOptions & opts,
                          FitData * data) const override {
    ImplicitFitBase * f = getFb(data);
    f->processSoftOptions(opts);
  };


  ImplicitFit() : 
    PerDatasetFit("implicit", 
                  "Implicit fit",
                  "Implicit fit with user-supplied formula ",
                  1, -1, false)
  {
    ArgumentList * al = new
      ArgumentList(QList<Argument *>()
                   << new StringArgument("formula", 
                                         "Formula",
                                         "formula for the fit (y is the variable)"));

    makeCommands(al, effector(this, &ImplicitFit::runFitCurrentDataSet, true),
                 effector(this, &ImplicitFit::runFit, true),
                 NULL,
                 effector(this, &ImplicitFit::computeFit)
                 );
  };


  /// This alternative constructor is to help create named fits based
  /// on formulas.
  ImplicitFit(const QString & name, const QString & formula) : 
    PerDatasetFit(name.toLocal8Bit(), 
        QString("Fit: %1").arg(formula).toLocal8Bit(),
                  QString("Fit of the formula %1").arg(formula).toLocal8Bit(), 1, -1, false)
  { 
    formulaString = formula;
    makeCommands();
  };


  const QString & formula() const {
    return formulaString;
  };

  QList<ParameterDefinition> parameters(FitData * data) const override {
    ImplicitFitBase * f = getFb(data);
    return f->parameters();
  };

};

static ImplicitFit implicitFit;



//////////////////////////////////////////////////////////////////////

static QHash<QString, ImplicitFit *> implicitFits;

static ImplicitFit * createImplicitFit(const QString & name,
                             const QString & formula,
                             bool overwrite = false)
{
  Fit::safelyRedefineFit(name, overwrite);
  implicitFits.remove(name);
  ImplicitFit * fit = new ImplicitFit(name, formula);
  implicitFits[name] = fit;
  return fit;
}



static void defineImplicitFitCommand(const QString &, 
                                     QString name, QString formula,
                                     const CommandOptions & opts)
{
  bool overwrite  = false;
  updateFromOptions(opts, "redefine", overwrite);
  createImplicitFit(name, formula, overwrite);
}

static ArgumentList 
cfArgs(QList<Argument *>() 
        << new StringArgument("name", 
                              "Name",
                              "Name for the new fit")
        << new StringArgument("formula", 
                              "Formula",
                              "Mathematical expression for the implicit fit")
       );

static ArgumentList 
cfOpts(QList<Argument *>() 
        << new BoolArgument("redefine", 
                            "Redefine",
                            "If the fit already exists, redefines it")
       );

static Command 
defineCustom("define-implicit-fit", // command name
             effector(defineImplicitFitCommand), // action
             "fits",  // group name
             &cfArgs, // arguments
             &cfOpts, // options
             "Define implicit fit",
             "Define custom implicit fit from a formula");
