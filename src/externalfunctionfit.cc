/**
   \file externalfunctionfit.cc external function fits
   Copyright 2021 by CNRS/AMU

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
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <terminal.hh>

#include <perdatasetfit.hh>
#include <dataset.hh>
#include <externalfunction.hh>


/// This class fits an external function
///
/// @todo Implement 
class ExternalFunctionFit : public PerDatasetFit {

  /// The function
  ExternalFunction * func;

  class Storage : public FitInternalStorage {
  public:
  };

protected:
  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds*/) const override {
    return deepCopy<Storage>(source);
  };

protected:


    
public:
  
  virtual void function(const double *parameters, FitData *data, 
                        const DataSet *ds, gsl_vector *target) const override {
    Vector vals = func->compute(ds->x(), parameters);
    gsl_vector_memcpy(target, vals);
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    QStringList ps = func->parameters();
    QHash<QString, double> vals = func->defaultValues();
    for(int i = 0; i < ps.size(); i++)
      a[i] = vals.value(ps[i], 1);
  };

  QList<ParameterDefinition> parameters(FitData * data) const override {
    QList<ParameterDefinition> rv;
    for(const QString & s : func->parameters())
      rv << ParameterDefinition(s);
    return rv;
  };

  ExternalFunctionFit(const QString & name, ExternalFunction * fnc) :
    PerDatasetFit(name.toLocal8Bit(), 
                  "External function fit",
                  "External function fit",
                  1, -1),
    func(fnc)
  {
  };




};



//////////////////////////////////////////////////////////////////////

static QHash<QString, ExternalFunctionFit *> externalFits;

static ExternalFunctionFit * createExternalFit(const QString & name,
                                               ExternalFunction * func,
                                               bool overwrite = false)
{
  Fit::safelyRedefineFit(name, overwrite);
  externalFits.remove(name);
  ExternalFunctionFit * fit =
    new ExternalFunctionFit(name, func);
  externalFits[name] = fit;
  return fit;
}



static void definePythonFitCommand(const QString &, 
                                   QString name, QString file,
                                   QString func,
                                   const CommandOptions & opts)
{
  bool overwrite  = false;
  updateFromOptions(opts, "redefine", overwrite);

  QString python;
  updateFromOptions(opts, "interpreter", python);
  
  ExternalFunction * fun =
    ExternalFunction::pythonFunction(file, func, python);
  Terminal::out << "Found function : " << func << " with parameters: "
                << fun->parameters().join(", ") << endl;
  createExternalFit(name, fun, overwrite);
}

static ArgumentList 
cfArgs(QList<Argument *>() 
       << new StringArgument("name", 
                             "Name",
                             "Name for the new fit")
       << new FileArgument("file", 
                           "Python file",
                           "Python file containing the definition")
       << new StringArgument("function", 
                             "Function",
                             "Name of the function in the file")
       );

static ArgumentList 
cfOpts(QList<Argument *>() 
        << new BoolArgument("redefine", 
                            "Redefine",
                            "If the fit already exists, redefines it")
        << new FileArgument("interpreter", 
                            "Python interpreter",
                            "Path to the python3 interpreter")
       );

static Command 
definePython("define-python-fit", // command name
             effector(definePythonFitCommand), // action
             "fits",  // group name
             &cfArgs, // arguments
             &cfOpts, // options
             "Define python fit",
             "Define a fit from python code");
