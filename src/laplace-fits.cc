/**
   \file laplace-fits.cc reverse Laplace transform fits
   Copyright 2024 by CNRS/AMU

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

#include <perdatasetfit.hh>
#include <dataset.hh>
#include <vector.hh>
#include <fitdata.hh>

#include <soas.hh>

#include <complexexpression.hh>


/// This class permits fitting the reverse Laplace transform of a
/// complex expression.
class ReverseLaplaceFit : public PerDatasetFit {

  class Storage : public FitInternalStorage {
  public:
    ComplexExpression * expression = NULL;

    ~Storage() {
      delete expression;
    };

    Storage() = default;
    
    Storage(const Storage & other) {
      expression = new ComplexExpression(*(other.expression));
    };
  };


protected:

  /// The formula, in case we are using a predefined formula
  QString formula;
  
  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/,
                                           FitInternalStorage * source,
                                           int /*ds*/) const override {
    return deepCopy<Storage>(source);
  };

protected:

  virtual QString optionsString(FitData * data) const override {
    if(formula.isEmpty())
      return "formula: " + storage<Storage>(data)->expression->formula();
    else
      return "formula: " + formula;
  };

  virtual void processOptions(const CommandOptions &opts,
                              FitData * data) const override {
    if(! formula.isEmpty()) {
      Storage * s = storage<Storage>(data);
      delete s->expression;
      s->expression = new ComplexExpression("s", formula);
    }
  };

  void prepareFit(const QString & formula,
                  FitData * data,
                  const CommandOptions & opts)
  {
    Storage * s = storage<Storage>(data);
    delete s->expression;
    s->expression = new ComplexExpression("s", formula);
  }

  void runFit(const QString & name, QString formula, 
              QList<const DataSet *> datasets,
              const CommandOptions & opts)
  {
    Fit::runFit([this, formula, opts](FitData * data) {
      prepareFit(formula, data, opts);
    }, name, datasets, opts);
  }

  void runFitCurrentDataSet(const QString & n, 
                            QString formula, const CommandOptions & opts)
  {
    QList<const DataSet *> ds;
    ds << soas().currentDataSet();
    runFit(n, formula, ds, opts);
  }

  void computeFit(const QString & name, QString formula,
                  QString params,
                  QList<const DataSet *> datasets,
                  const CommandOptions & opts)
  {
    Fit::computeFit([this, formula, opts](FitData * data) {
      prepareFit(formula, data, opts);
      }, name, params, datasets, opts);
  }

public:

  
  ArgumentList fitHardOptions() const override {
    return ArgumentList();
  };

  virtual void function(const double *parameters, FitData *data, 
                        const DataSet *ds, gsl_vector *target) const override {
    Storage * s = storage<Storage>(data);
    s->expression->reverseLaplace(parameters, ds->x().data(),
                                  target);
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);
    int nb = s->expression->currentVariables().size() - 1;
    for(int i = 0; i < nb; i++)
      a[i] = 1;                 // As good as anything
  };

  virtual ArgumentList fitArguments() const override {
    if(formula.isEmpty())
      return
        ArgumentList(QList<Argument *>()
                     << new StringArgument("formula",
                                           "Formula",
                                           "formula of the Laplace transform "
                                           "(the variable is s)"));
    else
      return ArgumentList();
  };

  

  QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    QStringList variables = s->expression->currentVariables();
    QString first = variables.takeFirst();
    if(first != "s")
      throw InternalError("The first variable should be 's' but is '%1'").
        arg(first);
    QList<ParameterDefinition> defs;
    for(const QString & v : variables)
      defs << ParameterDefinition(v);
    return defs;
  };

  ////////////////////
  // constructors

  /// This alternative constructor is to help create named fits based
  /// on formulas.
  ReverseLaplaceFit() :
    PerDatasetFit("laplace",
                  "Reverse Laplace transform", "", 1, -1, false)
  {
    makeCommands(ArgumentList(), 
                 effector(this, &ReverseLaplaceFit::runFitCurrentDataSet, true),
                 effector(this, &ReverseLaplaceFit::runFit, true),
                 ArgumentList(),
                 effector(this, &ReverseLaplaceFit::computeFit)
                 );
  };

  ReverseLaplaceFit(const QString & name,
                    const QString & frml) : 
    PerDatasetFit(name,
                  "Reverse Laplace transform", "", 1, -1, false),
    formula(frml)
  {
    makeCommands();
  }

};

static ReverseLaplaceFit rf;

//////////////////////////////////////////////////////////////////////

static ArgumentList 
dRLArgs(QList<Argument *>() 
         << new StringArgument("formula", 
                               "Formula",
                               "The formula in Laplace space (variable: s)")
         << new StringArgument("name", 
                               "Name",
                               "Name of the newly created fit")
         );


static ArgumentList 
dRLOpts(QList<Argument *>() 
         << new BoolArgument("redefine", 
                             "Redefine",
                             "If the fit already exists, redefines it")
         );

static void defineRLFitCommand(const QString &, QString formula, 
                               QString name, const CommandOptions & opts)
{
  bool overwrite = false;
  updateFromOptions(opts, "redefine", overwrite);
  Fit::safelyRedefineFit(name, overwrite);

  new ReverseLaplaceFit(name, formula);
}


static Command 
dlwf("define-reverse-laplace-fit", // command name
     effector(defineRLFitCommand), // action
     "fits",                       // group name
     &dRLArgs,                     // arguments
     &dRLOpts,                     // options
     "Define a reverse Laplace fit based on a formula");
