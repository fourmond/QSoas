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

/// Multi-buffer arbitrary fits, using Ruby as formula backend.
///
/// @warning Two fits cannot be run at the same time, as the main
/// instance is used to keep track of the current formula.
///
/// @todo There is too much code copypasted between this fit and
/// ArbitraryFit. They should be merged.
class MultiBufferArbitraryFit : public Fit {
private:

  /// The ID for :call
  ID callID;

  /// Blocks 
  QList<VALUE> blocks;

  /// The last formula used.
  ///
  /// The formula is a list of |-separated formulas, applying each in
  /// turn to the buffers.
  QString lastFormula;

  QStringList formulas;

  /// The parameters
  QStringList params;

  /// A small wrapper around Ruby::makeBlock
  static VALUE parseBlock(const QString & formula, QStringList & parameters)
  {
    QByteArray bta = formula.toLocal8Bit();
    const QByteArray & c = bta;
    return Ruby::run<QStringList *, const QByteArray &>(&Ruby::makeBlock, &parameters, c);
  }


  void parseBlocks(const QString &formula)
  {
    lastFormula = formula;
    params.clear();
    params << "x" << "f" << "pi" << "fara" << "temperature";
    formulas = formula.split("|");
    for(int i = 0; i < blocks.size(); i++)
      rb_gc_unregister_address(&blocks[i]);
    blocks.clear();
    for(int i = 0; i < formulas.size(); i++)
      parseBlock(formulas[i], params); // Just to update
                                                     // the parameters
                                                     // list
    for(int i = 0; i < formulas.size(); i++) {
      blocks << parseBlock(formulas[i], params);
      rb_gc_register_address(&blocks[i]);
    }
    for(int i = 0; i < 4; i++)
      params.takeFirst();       // Remove constants and/or derived stuff
  }

  void runFit(const QString & name, QString formula, 
              QList<const DataSet *> datasets,
              const CommandOptions & opts)
  {
    callID = rb_intern("call"); // Shouldn't be done in the
    // constructor, called to early.
    Terminal::out << "Fitting using formula '" << formula << "'" << endl;
    parseBlocks(formula);
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
    callID = rb_intern("call");
    parseBlocks(lastFormula);
  };


    
public:

  virtual void function(const double *a, 
                        FitData *data, gsl_vector *target) {
    int k = 0;
    int nbparams = data->parameterDefinitions.size();
    int nbargs = nbparams + 4;

    VALUE args[nbargs];
    args[1] = rb_float_new(GSL_CONST_MKSA_FARADAY); // f
    args[2] = rb_float_new(M_PI); // pi
    args[3] = rb_float_new(GSL_CONST_MKSA_FARADAY/ 
                           (a[0] * GSL_CONST_MKSA_MOLAR_GAS));

    for(int i = 0; i < nbparams; i++)
      args[i + 4] = rb_float_new(a[i]);
    for(int i = 0; i < data->datasets.size(); i++) {
      const Vector &xv = data->datasets[i]->x();
      VALUE blk;
      if(blocks.size() > 1)
        blk = blocks[i];
      else {
        // We need to initialize the correct parameters too
        blk = blocks[0];
        args[3] = rb_float_new(GSL_CONST_MKSA_FARADAY/ 
                               (a[i * nbparams] * GSL_CONST_MKSA_MOLAR_GAS));
        
        for(int l = 0; l < data->parameterDefinitions.size(); l++)
          args[l + 4] = rb_float_new(a[l + i * nbparams]);
      }
      for(int j = 0; j < xv.size(); j++) {
        args[0] = rb_float_new(xv[j]); // x
        gsl_vector_set(target, k++,  
                       NUM2DBL(Ruby::run(&rb_funcall2, blk, 
                                         callID, nbargs, (const VALUE *) args)));
      }
    }
    
  };

  virtual void initialGuess(FitData * data, 
                            double * a)
  {
    int nbparams = data->parameterDefinitions.size();
    for(int j = 0; j < data->datasets.size(); j++)
      for(int i = 0; i < params.size(); i++)
        a[i + j*nbparams] = paramGuess(params[i], data->datasets[j]);
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> p;
    for(int i = 0; i < params.size(); i++)
      p << ParameterDefinition(params[i], paramFixed(params[i]), 
                               blocks.size() == 1);
    return p;
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

    makeCommands(al, effector(this, &MultiBufferArbitraryFit::runFitCurrentDataSet),
                 effector(this, &MultiBufferArbitraryFit::runFit));
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
    if(blocks.size() > 1 && blocks.size() != data->datasets.size()) {
      throw RuntimeError(QString("Fit %1 needs %2 datasets, "
                                 "but got %3").
                         arg(name).arg(blocks.size()).arg(data->datasets.size()));
    }
      
  };

  virtual QString annotateDataSet(int index) const {
    if(blocks.size() > 1)
      return QString("(%1)").arg(formulas[index]);
    else
      return QString();
  };	

  const QString & formula() const {
    return lastFormula;
  };

};

static MultiBufferArbitraryFit mBarbFit;



//////////////////////////////////////////////////////////////////////

static QHash<QString, MultiBufferArbitraryFit *> customFits;

/// This function loads fits from a text stream
static void loadFits(QTextStream & in, bool verbose = true) {
  QString line;
  QRegExp sep("^\\s*([a-z0-9A-Z-]+):(.*)");
  QRegExp comment("^\\s*#|^\\s*$"); // Comment or fully blank line
  int ln = 0;
  int init = customFits.size();
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
        MultiBufferArbitraryFit * fit = 
          new MultiBufferArbitraryFit(name, formula);
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
  Terminal::out << "Loaded " << customFits.size() - init 
                << " fits" << endl;
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
         "fits",  // group name
         &lfArgs, // arguments
         NULL, 
         "Load fits",
         "Load fits from a file",
         "Load fits from a file");
