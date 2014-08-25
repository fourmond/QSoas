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

#include <expression.hh>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_math.h>


/// This is a base class for fits that hold formulas
class FormulaBasedFit {
public:

  static QStringList splitFormulas(const QString & fms) {
    return fms.split("|");
  };
protected:

  /// The last formula used.
  ///
  /// The formula is a list of |-separated formulas, applying each in
  /// turn to the buffers.
  QString lastFormula;

  /// The formulas, split.
  QStringList formulas;

  /// The expressions being used !
  QList<Expression> expressions;

  /// The parameters
  QStringList params;

  void parseFormulas(const QString &formula)
  {
    lastFormula = formula;
    params.clear();
    params << "x" << "i" << "seg" << "fara" << "temperature";
    formulas = splitFormulas(formula); 

    for(int i = 0; i < formulas.size(); i++) {
      Expression s(formulas[i]);
      params << s.naturalVariables();
    }

    Utils::makeUnique(params);

    expressions.clear();
    for(int i = 0; i < formulas.size(); i++)
      expressions << Expression(formulas[i], params);

    for(int i = 0; i < 4; i++)
      params.takeFirst();  
  }

  /// Returns the initial guess for the named parameter
  double paramGuess(const QString & name, const DataSet * ds) const {
    if(name == "temperature")
      return soas().temperature();
    if(name == "y_0")
      return ds->y().first();
    if(name == "x_0")
      return ds->x().first();
    return 1;
  };

  /// Returns whether the parameter is fixed by default
  bool paramFixed(const QString & name) const {
    if(name == "temperature" || name == "y_0" || name == "x_0")
      return true;
    return false;
  }

  void initialGuessForDataset(const DataSet * ds,
                              double * a)
  {
    for(int i = 0; i < params.size(); i++)
      a[i] = paramGuess(params[i], ds);
  };

  QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> p;
    for(int i = 0; i < params.size(); i++)
      p << ParameterDefinition(params[i], paramFixed(params[i]), 
                               formulas.size() == 1);
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
    int nbparams = data->parametersPerDataset();

    int seg = 0;
    QVarLengthArray<double, 100> args(4 + nbparams);
    args[0] = 0; // x
    args[1] = 0; // i
    args[2] = 0; // seg
    args[3] = GSL_CONST_MKSA_FARADAY/ 
      (a[0] * GSL_CONST_MKSA_MOLAR_GAS);

    for(int i = 0; i < nbparams; i++)
      args[i + 4] = a[i];

    const Vector &xv = ds->x();

    const Expression * expr =  &expressions[formulaIndex];
    for(int j = 0; j < xv.size(); j++) {
      while(seg < ds->segments.size() && j >= ds->segments[seg])
        seg++;

      args[0] = xv[j]; // x
      args[1] = j;     // i !
      args[2] = seg;
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
class MultiBufferArbitraryFit : public Fit, public  FormulaBasedFit {
private:


  void runFit(const QString & name, QString formula, 
              QList<const DataSet *> datasets,
              const CommandOptions & opts)
  {
    Terminal::out << "Fitting using formula '" << formula << "'" << endl;
    parseFormulas(formula);
    lastFormula = formula;
    Terminal::out << " -> detected parameters:  " << params.join(", ") 
                  << endl;
    Fit::runFit(name, datasets, opts);
  }

  void runFitCurrentDataSet(const QString & n, 
                            QString formula, const CommandOptions & opts)
  {
    QList<const DataSet *> ds;
    ds << soas().currentDataSet();
    runFit(n, formula, ds, opts);
  }
    




protected:

  virtual QString optionsString() const {
    return "formula: " + lastFormula;
  };

  virtual void processOptions(const CommandOptions & ) {
    parseFormulas(lastFormula);
  };


    
public:

  virtual void function(const double *a, 
                        FitData *data, gsl_vector *target) {
    int nbparams = data->parametersPerDataset();

    for(int i = 0; i < data->datasets.size(); i++) {
      gsl_vector_view v = data->viewForDataset(i, target);

      computeDataSet(a + i * nbparams, 
                     data->datasets[i],
                     data, &v.vector,
                     i % formulas.size());
    }
  };

  virtual void initialGuess(FitData * data, 
                            double * a)
  {
    int nbparams = data->parametersPerDataset();
    for(int j = 0; j < data->datasets.size(); j++)
      initialGuessForDataset(data->datasets[j], a + j*nbparams);
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
    ArgumentList * al = new 
      ArgumentList(QList<Argument *>()
                   << new StringArgument("formulas", 
                                         "Formulas",
                                         "|-separated formulas for the fit"));

    makeCommands(al, effector(this, &MultiBufferArbitraryFit::runFitCurrentDataSet, true),
                 effector(this, &MultiBufferArbitraryFit::runFit, true));
  };

  /// This alternative constructor is to help create named fits based
  /// on formulas.
  MultiBufferArbitraryFit(const QString & name, const QString & formula) : 
    Fit(name.toLocal8Bit(), 
        QString("Fit: %1").arg(formula).toLocal8Bit(),
        QString("Fit of the formula %1").arg(formula).toLocal8Bit(), 1, -1)
  { 
    /// @todo make sure that the fit- command isn't generated for
    /// intrinsically multi-buffer fits.
    lastFormula = formula;
  };

  virtual void checkDatasets(const FitData * data) const {
    if(formulas.size() > 1 && formulas.size() != data->datasets.size()) {
      throw RuntimeError(QString("Fit %1 needs %2 datasets, "
                                 "but got %3").
                         arg(name).arg(formulas.size()).
                         arg(data->datasets.size()));
    }
      
  };

  virtual QString annotateDataSet(int index) const {
    if(formulas.size() > 1)
      return QString("(%1)").arg(formulas[index]);
    else
      return QString();
  };	

  const QString & formula() const {
    return lastFormula;
  };

  QList<ParameterDefinition> parameters() const {
    return FormulaBasedFit::parameters();
  };

};

static MultiBufferArbitraryFit mBarbFit;


//////////////////////////////////////////////////////////////////////

/// This class holds a fit using a single formula, that hence applies
/// naturally to several datasets. There is no 'arb' fit from that
class SingleBufferArbitraryFit : public PerDatasetFit, 
                                 public FormulaBasedFit {

protected:

  virtual QString optionsString() const {
    return "formula: " + lastFormula;
  };

  virtual void processOptions(const CommandOptions & ) {
    parseFormulas(lastFormula);
    if(formulas.size() > 1)
      throw InternalError("Somehow got to define a single buffer fit "
                          "with several formulas");
  };


    
public:

  virtual void function(const double *parameters, FitData *data, 
                        const DataSet *ds, gsl_vector *target) {
    computeDataSet(parameters, ds, data, target);
  };

  virtual void initialGuess(FitData * /*data*/, 
                            const DataSet * ds,
                            double * a)
  {
    initialGuessForDataset(ds, a);
  };


  /// This alternative constructor is to help create named fits based
  /// on formulas.
  SingleBufferArbitraryFit(const QString & name, const QString & formula) : 
    PerDatasetFit(name.toLocal8Bit(), 
        QString("Fit: %1").arg(formula).toLocal8Bit(),
        QString("Fit of the formula %1").arg(formula).toLocal8Bit(), 1, -1)
  { 
    /// @todo make sure that the fit- command isn't generated for
    /// intrinsically multi-buffer fits.
    lastFormula = formula;
  };


  const QString & formula() const {
    return lastFormula;
  };

  QList<ParameterDefinition> parameters() const {
    return FormulaBasedFit::parameters();
  };

};




//////////////////////////////////////////////////////////////////////

static QHash<QString, FormulaBasedFit *> customFits;

static FormulaBasedFit * createCustomFit(const QString & name,
                                         const QString & formula,
                                         bool overwrite = false)
{
  if(customFits.contains(name)) {
    if(overwrite) {
      Terminal::out << "Replacing fit '" << name  
                    << "' with a new definition" << endl;
      /// @todo Implement safe deletion ! (because for now, in
      /// terms of commands, that won't work so well...
    }
    else
      throw RuntimeError("Fit '%1' is already defined").arg(name);
  }

  int nb = FormulaBasedFit::splitFormulas(formula).size();
  FormulaBasedFit * fit;
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


static void loadFitsCommand(const QString &, QString fitsFile)
{
  QFile file(fitsFile);
  Utils::open(&file, QIODevice::ReadOnly);
  loadFits(&file);
}

static Command 
loadFitsC("load-fits", // command name
         optionLessEffector(loadFitsCommand), // action
         "fits",  // group name
         &lfArgs, // arguments
         NULL, 
         "Load fits",
         "Load fits from a file",
         "Load fits from a file");

//////////////////////////////////////////////////////////////////////



static void defineCustomFitCommand(const QString &, 
                                   QString name, QString formula)
{
  /// @todo overwrite ?
  bool overwrite  = false;
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

static Command 
defineCustom("custom-fit", // command name
             optionLessEffector(defineCustomFitCommand), // action
             "fits",  // group name
             &cfArgs, // arguments
             NULL, 
             "Define fit",
             "Define custom fit from a formula");
