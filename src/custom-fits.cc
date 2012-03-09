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
#include <ruby.hh>
#include <ruby-templates.hh>

#include <perdatasetfit.hh>
#include <dataset.hh>
#include <vector.hh>
#include <fitdata.hh>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_math.h>
/// Arbitrary fits, using Ruby as formula backend.
///
/// @warning Two fits cannot be run at the same time, as the main
/// instance is used to keep track of the current formula.
class ArbitraryFit : public FunctionFit {

  /// The ID for :call
  ID callID;

  /// Block for the last formula used
  VALUE block;

  /// The last formula used.
  QString lastFormula;

  /// The parameters
  QStringList params;

  void parseBlock(const QString &formula)
  {
    params.clear();
    params << "x" << "f" << "pi" << "fara" << "temperature";
    lastFormula = formula;
    QByteArray bta = formula.toLocal8Bit();
    const QByteArray & c = bta;
    block = Ruby::run<QStringList *, const QByteArray &>(&Ruby::makeBlock, &params, c);
    for(int i = 0; i < 4; i++)
      params.takeFirst();       // Remove constants and/or derived stuff

  }
    
  void runFitCurrentDataSet(const QString & n, 
                            QString formula, const CommandOptions & opts)
  {
    QList<const DataSet *> ds;
    ds << soas().currentDataSet();
    runFit(n, formula, ds, opts);
  }

  void runFit(const QString & name, QString formula, 
              QList<const DataSet *> datasets,
              const CommandOptions & opts)
  {
    callID = rb_intern("call"); // Shouldn't be done in the
    // constructor, called to early.
    Terminal::out << "Fitting using formula '" << formula << "'" << endl;
    parseBlock(formula);
    lastFormula = formula;
    Terminal::out << " -> detected parameters:  " << params.join(", ") 
                  << endl;

    Fit::runFit(name, datasets, opts);
  }

  /// Returns whether the parameter is fixed by default
  bool paramFixed(const QString & name) const {
    if(name == "temperature" || name == "y_0" || name == "x_0")
      return true;
    return false;
  }

  /// Returns the initial guess for the named parameter
  double paramGuess(const QString & name, const DataSet * ds) const {
    if(name == "temperature")
      return soas().temperature();
    // return GSL_CONST_MKSA_FARADAY/ (soas().temperature() * 
    //                                 GSL_CONST_MKSA_MOLAR_GAS);
    if(name == "y_0")
      return ds->y().first();
    if(name == "x_0")
      return ds->x().first();
    return 1;
  };

protected:

  virtual QString optionsString() const {
    return "formula: " + lastFormula;
  };

  virtual void processOptions(const CommandOptions & ) {
    // In this function, we prepare the block if it is nil

    if(block == Qnil) {
      callID = rb_intern("call"); // Shouldn't be done in the
      parseBlock(lastFormula);
    }
  };


    
public:

  virtual double function(const double * a, 
                          FitData * params, double x) {
    int nbargs = params->parameterDefinitions.size() + 4;
    VALUE args[nbargs];
    args[0] = rb_float_new(x); // x
    args[1] = rb_float_new(GSL_CONST_MKSA_FARADAY); // f
    args[2] = rb_float_new(M_PI); // pi
    args[3] = rb_float_new(GSL_CONST_MKSA_FARADAY/ 
                           (a[0] * GSL_CONST_MKSA_MOLAR_GAS));
    for(int i = 0; i < params->parameterDefinitions.size(); i++)
      args[i + 4] = rb_float_new(a[i]);
    return NUM2DBL(Ruby::run(&rb_funcall2, block, 
                             callID, nbargs, (const VALUE *) args));
  };

  virtual void initialGuess(FitData * /*params*/, 
                            const DataSet * ds,
                            double * a)
  {
    // Or shall I use FitData ?
    for(int i = 0; i < params.size(); i++)
      a[i] = paramGuess(params[i], ds);
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> p;
    for(int i = 0; i < params.size(); i++)
      p << ParameterDefinition(params[i], paramFixed(params[i]));
    return p;
  };


  ArbitraryFit() : FunctionFit("arb", 
                               "Arbitrary fit",
                               "Arbitrary fit, with user-supplied formula\n"
                               "Special parameters: temp, fara, y_0, x_0.\n"
                               "Already defined constants: f, pi",
                               1, -1, false), block(Qnil)
  { 
    ArgumentList * al = new 
      ArgumentList(QList<Argument *>()
                   << new StringArgument("formula", 
                                         "Formula",
                                         "Formula for the fit"));

    makeCommands(al, 
                 effector(this, &ArbitraryFit::runFitCurrentDataSet),
                 effector(this, &ArbitraryFit::runFit));
  };

  /// This alternative constructor is to help create named fits based
  /// on formulas.
  ArbitraryFit(const QString & name, const QString & formula) : 
    FunctionFit(name.toLocal8Bit(), 
                QString("Fit: %1").arg(formula).toLocal8Bit(),
                QString("Fit of the formula %1").arg(formula).toLocal8Bit()), 
    block(Qnil)
  { 
    lastFormula = formula;
  };

  const QString & formula() const {
    return lastFormula;
  };

};

static ArbitraryFit arbFit;

static QHash<QString, ArbitraryFit *> customFits;

/// This function loads fits from a text stream
static void loadFits(QTextStream & in, bool verbose = true) {
  QString line;
  QRegExp sep("^\\s*([a-z0-9A-Z-]+):(.*)");
  QRegExp comment("^\\s*#");
  int ln = 0;
  do {
    line = in.readLine();
    ++ln;
    if(sep.indexIn(line, 0) >= 0) {
      QString name = sep.cap(1);
      QString formula = sep.cap(2);
      try {
        if(customFits.contains(name)) {
          Terminal::out << "Fit '" << name << "' (line " << ln 
                        << ") is already defined " << endl;
          continue;
        }
        ArbitraryFit * fit = new ArbitraryFit(name, formula);
        customFits[name] = fit;
      }
      catch(Exception & er) {
        Terminal::out << "Error loading fit " << name << " : " 
                      << er.message() << endl;
      }
    }
    else {
      if(comment.indexIn(line, 0) < 0)
        Terminal::out << "Line " << ln << " not understood" << endl;
    }
  } while(! line.isNull());
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
  QTextStream s(&file);
  loadFits(s);
}

static Command 
loadFitsC("load-fits", // command name
         optionLessEffector(loadFitsCommand), // action
         "file",  // group name
         &lfArgs, // arguments
         NULL, 
         "Load fits",
         "Load fits from a file",
         "Load fits from a file");
