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

#include <file.hh>


#include <expression.hh>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_math.h>


/// This is a base class for fits that hold formulas
class FormulaBasedFit {
public:

  static QStringList splitFormulas(const QString & fms) {
    return fms.split("|");
  };

  /// The last formula used.
  ///
  /// The formula is a list of |-separated formulas, applying each in
  /// turn to the buffers.
  QString lastFormula;

  /// The formulas, split.
  QStringList formulas;

  /// The expressions being used !
  QList<Expression> expressions;

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

  static ArgumentList hardOptions() {
    return
      ArgumentList(QList<Argument *>()
                   << new TDPArgument("with", 
                                      "Time dependent parameters",
                                      "Make certain parameters depend "
                                      "upon time")
                   );
    
  };

  void parseFormulas(const QString &formula)
  {
    lastFormula = formula;
    params.clear();
    nbXSteps = 0;
    nbYSteps = 0;
    fixedParameters.clear();

    QStringList naturalParameters;
    
    formulas = splitFormulas(formula); 

    for(int i = 0; i < formulas.size(); i++) {
      Expression s(formulas[i]);
      naturalParameters << s.naturalVariables();
    }

    std::sort(naturalParameters.begin(), naturalParameters.end());

    
    params << "x" << "i" << "seg";
    if(naturalParameters.contains("fara") ||
       naturalParameters.contains("temperature")) {
      hasTemperature = true;
      params << "fara" << "temperature";
    }
    else
      hasTemperature = false;
    params += naturalParameters;

    Utils::makeUnique(params);

    expressions.clear();
    for(int i = 0; i < formulas.size(); i++)
      expressions << Expression(formulas[i], params);

    for(int i = 0; i < (hasTemperature ? 4 : 3); i++)
      params.takeFirst();

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
  }

  /// Returns the initial guess for the named parameter
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
      return l + d*re.cap(2).toInt();
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
    for(int i = 0; i < params.size(); i++)
      p << ParameterDefinition(params[i], paramFixed(params[i]), 
                               formulas.size() == 1);
    p += timeDependentParameters.fitParameters();
    return p;
  };

  /// Computes the function for the given dataset, assuming that the
  /// parameters are those for the given dataset. 
  void computeDataSet(const double *a, 
                      const DataSet * ds,
                      FitData *data, 
                      gsl_vector *target,
                      int formulaIndex = 0) {
    int k = 0;
    int nbparams = data->parametersPerDataset() +
      skippedIndices.size();

    int seg = 0;
    int base = (hasTemperature ? 4 : 3);
    QVarLengthArray<double, 100> args(base + nbparams);
    args[0] = 0; // x
    args[1] = 0; // i
    args[2] = 0; // seg
    if(hasTemperature)
      args[3] = GSL_CONST_MKSA_FARADAY/ 
        (a[0] * GSL_CONST_MKSA_MOLAR_GAS);

    for(int i = 0; i < nbparams; i++)
      Utils::skippingCopy(a, args.data()+base, nbparams, skippedIndices);

    timeDependentParameters.initialize(a + params.size());

    const Vector &xv = ds->x();

    const Expression * expr =  &expressions[formulaIndex];
    for(int j = 0; j < xv.size(); j++) {
      while(seg < ds->segments.size() && j >= ds->segments[seg])
        seg++;

      args[0] = xv[j]; // x
      args[1] = j;     // i !
      args[2] = seg;
      
      timeDependentParameters.computeValues(args[0], args.data()+base,
                                            a + params.size());

      gsl_vector_set(target, k++, expr->evaluate(args.data()));
    }
  };

};

/// Multi-buffer arbitrary fits, using the Expression class as formula
/// backend.
///
/// @warning Two fits cannot be run at the same time, as the main
/// instance is used to keep track of the current formula.
///
/// @todo Remove the passing of constants in such a ugly way... They
/// should be passed as constants !
///
/// @todo In addition to x as function parameter, provide i (or
/// anything else) as the index ?
class MultiBufferArbitraryFit : public Fit {

  class Storage : public FitInternalStorage {
  public:
    FormulaBasedFit fbf;
  };

  QString formulaString;

  FormulaBasedFit * getFbf(FitData * data) const {
    return &storage<Storage>(data)->fbf;
  };
  
protected:
  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds*/) const override {
    return deepCopy<Storage>(source);
  };
  
  
private:


  void runFit(const QString & name, QString formula, 
              QList<const DataSet *> datasets,
              const CommandOptions & opts)
  {

    Fit::runFit([this, &formula] (FitData * data) {
        FormulaBasedFit * f = getFbf(data);
        Terminal::out << "Fitting using formula '" << formula << "'" << endl;
        f->parseFormulas(formula);
        f->lastFormula = formula;
        Terminal::out << " -> detected parameters:  " << f->params.join(", ") 
                      << endl;
      }, name, datasets, opts);
  }

  void computeFit(const QString & name, QString formula,
                  QString params,
                  QList<const DataSet *> datasets,
                  const CommandOptions & opts)
  {
    Fit::computeFit([this, &formula](FitData * data) {
        FormulaBasedFit * f = getFbf(data);
        Terminal::out << "Computing using formula '" << formula << "'" << endl;
        f->parseFormulas(formula);
        f->lastFormula = formula;
        Terminal::out << " -> detected parameters:  " << f->params.join(", ") 
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
    




protected:

  virtual QString optionsString(FitData * data) const override {
    FormulaBasedFit * f = getFbf(data);
    return "formula: " + f->lastFormula;
  };

  virtual void processOptions(const CommandOptions &opts,
                              FitData * data ) const override {
    FormulaBasedFit * f = getFbf(data);
    if(! formulaString.isEmpty())
      f->lastFormula = formulaString;
    f->parseFormulas(f->lastFormula);
    f->processOptions(opts);
  };



    
public:

  ArgumentList fitHardOptions() const override {
    return FormulaBasedFit::hardOptions();
  };


  virtual void function(const double *a, 
                        FitData *data, gsl_vector *target) const override {
    FormulaBasedFit * f = getFbf(data);
    int nbparams = data->parametersPerDataset();

    for(int i = 0; i < data->datasets.size(); i++) {
      gsl_vector_view v = data->viewForDataset(i, target);

      f->computeDataSet(a + i * nbparams, 
                     data->datasets[i],
                     data, &v.vector,
                     i % f->formulas.size());
    }
  };

  virtual void initialGuess(FitData * data, 
                            double * a) const override
  {
    FormulaBasedFit * f = getFbf(data);
    int nbparams = data->parametersPerDataset();
    for(int j = 0; j < data->datasets.size(); j++)
      f->initialGuessForDataset(data->datasets[j], a + j*nbparams);
  };

  MultiBufferArbitraryFit() : 
    Fit("arb", 
        "Arbitrary fit",
        "Arbitrary fit with user-supplied formula "
        "(possibly spanning multiple buffers)\n"
        "Special parameters: x_0, y_0, temperature, fara.\n"
        "Already defined constants: f, pi",
        1, -1, false)
  {
    ArgumentList al =
      ArgumentList(QList<Argument *>()
                   << new StringArgument("formulas", 
                                         "Formulas",
                                         "|-separated formulas for the fit"));

    makeCommands(al, effector(this, &MultiBufferArbitraryFit::runFitCurrentDataSet, true),
                 effector(this, &MultiBufferArbitraryFit::runFit, true),
                 ArgumentList(),
                 effector(this, &MultiBufferArbitraryFit::computeFit)
                 );
  };

  /// This alternative constructor is to help create named fits based
  /// on formulas.
  MultiBufferArbitraryFit(const QString & name, const QString & formula) : 
    Fit(name.toLocal8Bit(), 
        QString("Fit: %1").arg(formula).toLocal8Bit(),
        QString("Fit of the formula %1").arg(formula).toLocal8Bit(), 1, -1)
  {
    formulaString = formula;
  };

  virtual void checkDatasets(const FitData * data) const override {
    FormulaBasedFit * f = getFbf(const_cast<FitData*>(data));
    if(f->formulas.size() > 1 && f->formulas.size() != data->datasets.size()) {
      throw RuntimeError(QString("Fit %1 needs %2 datasets, "
                                 "but got %3").
                         arg(name).arg(f->formulas.size()).
                         arg(data->datasets.size()));
    }
      
  };

  virtual QString annotateDataSet(int index, FitData * data) const override {
    FormulaBasedFit * f = getFbf(data);
    if(f->formulas.size() > 1)
      return QString("(%1)").arg(f->formulas[index]);
    else
      return QString();
  };	

  const QString & formula() const {
    if(formulaString.isEmpty())
      throw InternalError("Should not get here");
    return formulaString;
  };

  QList<ParameterDefinition> parameters(FitData * data) const override {
    FormulaBasedFit * f = getFbf(data);
    return f->parameters();
  };

};

static MultiBufferArbitraryFit mBarbFit;


//////////////////////////////////////////////////////////////////////

/// This class holds a fit using a single formula, that hence applies
/// naturally to several datasets. There is no 'arb' fit from that
class SingleBufferArbitraryFit : public PerDatasetFit {

  class Storage : public FitInternalStorage {
  public:
    FormulaBasedFit fbf;
  };

  QString formulaString;

  FormulaBasedFit * getFbf(FitData * data) const {
    return &storage<Storage>(data)->fbf;
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
    FormulaBasedFit * f = getFbf(data);
     return "formula: " + f->lastFormula;
  };

  /// @hack get rid of the const-cast, this isn't very clean
  virtual void processOptions(const CommandOptions &opts,
                              FitData * data) const override {
    FormulaBasedFit * f = getFbf(data);
    f->lastFormula = formulaString;
    f->parseFormulas(f->lastFormula);
    if(f->formulas.size() > 1)
      throw InternalError("Somehow got to define a single buffer fit "
                          "with several formulas");
    f->processOptions(opts);
  };


    
public:
  
  ArgumentList fitHardOptions() const override {
    return FormulaBasedFit::hardOptions();
  };

  virtual void function(const double *parameters, FitData *data, 
                        const DataSet *ds, gsl_vector *target) const override {
    FormulaBasedFit * f = getFbf(data);
    f->computeDataSet(parameters, ds, data, target);
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    FormulaBasedFit * f = getFbf(data);
    f->initialGuessForDataset(ds, a);
  };


  /// This alternative constructor is to help create named fits based
  /// on formulas.
  SingleBufferArbitraryFit(const QString & name, const QString & formula) : 
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
    FormulaBasedFit * f = getFbf(data);
    return f->parameters();
  };

};




//////////////////////////////////////////////////////////////////////

static QHash<QString, Fit *> customFits;

static Fit * createCustomFit(const QString & name,
                             const QString & formula,
                             bool overwrite = false)
{
  Fit::safelyRedefineFit(name, overwrite);
  customFits.remove(name);

  int nb = FormulaBasedFit::splitFormulas(formula).size();
  Fit * fit;
  if(nb > 1)
    fit = new MultiBufferArbitraryFit(name, formula);
  else
    fit = new SingleBufferArbitraryFit(name, formula);
     
  customFits[name] = fit;
  return fit;
}

/// This function loads fits from a text stream
static void loadFits(QIODevice * source, bool verbose = true,
                     bool overwrite = false) {
  QList<QPair<int, int> > numbers;
  QStringList lines = 
    Utils::parseConfigurationFile(source, false, NULL, &numbers, true);

  QRegExp sep("^\\s*([a-z0-9A-Z-]+):(.*)");
  int init = customFits.size();

  QStringList loadedFits;

  for(int i = 0; i < lines.size(); i++) {
    QString line = lines[i];
    QString lnNb = (numbers[i].first == numbers[i].second ?
                    QString("line %1").arg(numbers[i].first) :
                    QString("lines %1-%2").
                    arg(numbers[i].first).arg(numbers[i].second));
                    
    if(sep.indexIn(line, 0) >= 0) {
      QString name = sep.cap(1);
      QString formula = sep.cap(2);
      try {
        createCustomFit(name, formula, overwrite);
        loadedFits << name;
      }
      catch(Exception & er) {
        Terminal::out << "Error loading fit " << name << " on " 
                      << lnNb << " : "
                      << er.message() << endl;
      }
    }
    else 
      Terminal::out << "Error: " << lnNb <<  " not understood" << endl;
  }
  Terminal::out << "Loaded " << customFits.size() - init 
                << " fits: " << loadedFits.join(", ") << endl;
}

//////////////////////////////////////////////////////////////////////

static ArgumentList 
lfArgs(QList<Argument *>() 
        << new FileArgument("file", 
                            "Fits file",
                            "File containing the fits to load"));


static void loadFitsCommand(const QString &, QString fitsFile,
                            const CommandOptions & opts)
{
  File file(fitsFile, File::TextRead);
  bool redefine = false;
  updateFromOptions(opts, "redefine", redefine);
  loadFits(file, true, redefine);
}

static ArgumentList 
lfOpts(QList<Argument *>() 
        << new BoolArgument("redefine", 
                            "Redefine",
                            "If a fit already exists, redefines it")
       );

static Command 
loadFitsC("load-fits", // command name
         effector(loadFitsCommand), // action
         "fits",  // group name
         lfArgs, // arguments
         lfOpts, 
         "Load fits",
         "Load fits from a file");

//////////////////////////////////////////////////////////////////////



static void defineCustomFitCommand(const QString &, 
                                   QString name, QString formula,
                                   const CommandOptions & opts)
{
  bool overwrite  = false;
  updateFromOptions(opts, "redefine", overwrite);
  createCustomFit(name, formula, overwrite);
}

static ArgumentList 
cfArgs(QList<Argument *>() 
        << new StringArgument("name", 
                              "Name",
                              "Name for the new fit")
        << new StringArgument("formula", 
                              "Formula",
                              "Mathematical expression for the fit")
       );

static ArgumentList 
cfOpts(QList<Argument *>() 
        << new BoolArgument("redefine", 
                            "Redefine",
                            "If the fit already exists, redefines it")
       );

static Command 
defineCustom("custom-fit", // command name
             effector(defineCustomFitCommand), // action
             "fits",  // group name
             cfArgs, // arguments
             cfOpts, // options
             "Define fit",
             "Define custom fit from a formula");
