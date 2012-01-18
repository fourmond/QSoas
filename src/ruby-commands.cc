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
#include <terminal.hh>
#include <soas.hh>

#include <datastack.hh>
#include <ruby.hh>
#include <ruby-templates.hh>

#include <perdatasetfit.hh>
#include <dataset.hh>
#include <vector.hh>
#include <fitdata.hh>


//////////////////////////////////////////////////////////////////////



static ArgumentList 
fA(QList<Argument *>() 
   << new StringArgument("formula", 
                         "Formula",
                         "Formula (valid Ruby code)"));

static VALUE applyFormula(VALUE code, const DataSet * ds,
                          Vector * newX, Vector * newY)
{
  ID id = rb_intern("call");
  VALUE args[2];
  const Vector & xc = ds->x();
  const Vector & yc = ds->y();
  for(int i = 0; i < xc.size(); i++) {
    args[0] = rb_float_new(xc[i]);
    args[1] = rb_float_new(yc[i]);
    VALUE ret = rb_funcall2(code, id, 2, args);
    *newX << NUM2DBL(rb_ary_entry(ret, 0));
    *newY << NUM2DBL(rb_ary_entry(ret, 1));
  }      
  return Qnil;
}
  
static void applyFormulaCommand(const QString &, QString formula)
{
  const DataSet * ds = soas().currentDataSet();
  Terminal::out << QObject::tr("Applying formula '%1' to buffer %2").
    arg(formula).arg(ds->name) << endl;
  formula = QString("proc do |x,y|\n  %1\n  [x,y]\nend").arg(formula);
  QByteArray form = formula.toLocal8Bit();
  VALUE block = Ruby::run(Ruby::eval, form);
  Vector newX, newY;

  Ruby::run(&applyFormula, block, ds, &newX, &newY);

  DataSet * newDs = new DataSet(QList<Vector>() << newX << newY);
  newDs->name = ds->cleanedName() + "_mod.dat";
  soas().pushDataSet(newDs);
}


static Command 
load("apply-formula", // command name
     optionLessEffector(applyFormulaCommand), // action
     "stack",  // group name
     &fA, // arguments
     NULL, // options
     "Apply formula",
     "Applies a formula to the current dataset",
     QT_TR_NOOP("Applies a formula to the current dataset. "
                "The formula must be valid Ruby code."),
     "F");

//////////////////////////////////////////////////////////////////////

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
    params << "x";
    lastFormula = formula;
    QByteArray bta = formula.toLocal8Bit();
    const QByteArray & c = bta;
    block = Ruby::run<QStringList *, const QByteArray &>(&Ruby::makeBlock, &params, c);
    params.takeFirst();       // Remove x.
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

protected:

  virtual QString optionsString() const {
    return "formula: " + lastFormula;
  };
    
public:

  virtual double function(const double * a, 
                          FitData * params, double x) {
    int nbargs = params->parameterDefinitions.size() + 1;
    VALUE args[nbargs];
    args[0] = rb_float_new(x);
    for(int i = 0; i < params->parameterDefinitions.size(); i++)
      args[i + 1] = rb_float_new(a[i]);
    return NUM2DBL(Ruby::run(&rb_funcall2, block, 
                             callID, nbargs, (const VALUE *) args));
  };

  virtual void initialGuess(FitData * params, 
                            const DataSet *,
                            double * a)
  {
    for(int i = 0; i < params->parameterDefinitions.size(); i++)
      a[i] = 1;
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> p;
    for(int i = 0; i < params.size(); i++)
      p << ParameterDefinition(params[i]);
    return p;
  };


  ArbitraryFit() : FunctionFit("arb", 
                               "Arbitrary fit",
                               "Arbitrary fit, with user-supplied formula", 1, -1, false) 
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
};

static ArbitraryFit arbFit;
