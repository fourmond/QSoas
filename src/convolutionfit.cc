/*
  convolutionfit.cc: convolve another fit by a function
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

#include <fit.hh>
#include <expression.hh>
#include <fitdata.hh>
#include <dataset.hh>
#include <perdatasetfit.hh>
#include <command.hh>
#include <argumentlist.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>

#include <utils.hh>

#include <idioms.hh>

#include <soas.hh>

/// This Fit is based on another fit
class ConvolutionFit : public PerDatasetFit {
protected:


  /// The underlying fit
  PerDatasetFit * underlyingFit;

  // /// The formula for the convolution function
  QString convolutionFormula;


  /// Whether or not the convolution is symmetric
  bool isSymmetric;

  

  class Storage : public FitInternalStorage {
  public:

    /// Storage for the underlying fit
    FitInternalStorage * underlyingStorage;

    /// Parameters of the underlying fit
    QList<ParameterDefinition> originalParameters;

    /// Final parameters
    QList<ParameterDefinition> finalParameters;

    /// The convolution expression
    Expression * convolutionExpression;

    /// The convolution parameters
    QStringList convolutionParameters;

    Vector buffer, buffer2;

    void clearExpression() {
      delete convolutionExpression;
      convolutionExpression = NULL;
    }


    virtual ~Storage()
    {
      delete underlyingStorage;
      clearExpression();
    };

    Storage() :
      underlyingStorage(NULL),
      convolutionExpression(NULL)
    {
    };
    
    Storage(const Storage & o) :
      underlyingStorage(NULL),
      convolutionExpression(NULL),
      originalParameters(o.originalParameters),
      buffer(o.buffer),
      buffer2(o.buffer2)
    {
    };
  };
  
  virtual FitInternalStorage * allocateStorage(FitData * data) const override {
    Storage * s = new Storage;
    s->underlyingStorage = underlyingFit->allocateStorage(data);
    return s;
  };
  
  virtual FitInternalStorage * copyStorage(FitData * data, FitInternalStorage * source, int ds) const override {
    Storage * s = static_cast<Storage *>(source);
    Storage * ns = deepCopy<Storage>(s);

    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->underlyingStorage);
      ns->underlyingStorage = underlyingFit->copyStorage(data, s->underlyingStorage, ds);
    }
    
    return ns;
  };

  /// Prepares the convolution expression in the Storage class.
  /// Also prepares the final parameters, and the buffer
  void prepareConvolutionExpression(FitData * data) const {
    Storage * s = storage<Storage>(data);

    QStringList vars = Expression::variablesNeeded(convolutionFormula);
    vars.insert(0, "x");
    Utils::makeUnique(vars);

    s->clearExpression();
    s->convolutionExpression = new Expression(convolutionFormula, vars);

    vars.takeAt(0);

    s->finalParameters = s->originalParameters;
    for(const QString & var : vars) {
      // Tyr looking
      for(const ParameterDefinition & d : s->originalParameters)
        if(d.name == var)
          throw RuntimeError("Fit parameter also used as convolution parameter: '%1'").arg(var);
      s->finalParameters << ParameterDefinition(var);
    }

    for(const DataSet * ds : data->datasets) {
      if(s->buffer.size() < ds->nbRows())
        s->buffer = ds->x();
    }
    s->buffer2 = Vector(s->buffer.size() * 4, 0);
      
  };

  virtual void processOptions(const CommandOptions & opts, FitData * data) const override
  {
    Storage * s = storage<Storage>(data);

    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      Fit::processOptions(underlyingFit, opts, data);
      s->originalParameters = underlyingFit->parameters(data);
    }

    // Now make the expression
    prepareConvolutionExpression(data);
  }

  ArgumentList fitSoftOptions() const override
  {
    return Fit::fitSoftOptions(underlyingFit);
  }

  ArgumentList fitHardOptions() const override
  {
    return Fit::fitHardOptions(underlyingFit);
  }

  CommandOptions currentSoftOptions(FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      return Fit::currentSoftOptions(underlyingFit, data);
    }
  }

  void processSoftOptions(const CommandOptions & opts, FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      Fit::processSoftOptions(underlyingFit, opts, data);
    }
  }

  
public:

  virtual QString optionsString(FitData * data) const override
  {
    Storage * s = storage<Storage>(data);

    QString so;
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      so = underlyingFit->optionsString(data);
    }

    if(! so.isEmpty())
      so = " (options: " + so + ")";

    return QString("fit: %1%2 convolved by %3").
      arg(underlyingFit->fitName(false)).arg(so).
      arg(convolutionFormula);
  };

  virtual QList<ParameterDefinition> parameters(FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    if(! s->convolutionExpression)
      prepareConvolutionExpression(data);
    return s->finalParameters;
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);
    if(! s->convolutionExpression)
      prepareConvolutionExpression(data);

    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      underlyingFit->initialGuess(data, ds, a);
    }
    for(int i = s->originalParameters.size();
        i < s->finalParameters.size(); i++)
      a[i] = 1;                 // Good default
  }

  virtual void function(const double * parameters,
                        FitData * data, 
                        const DataSet * ds,
                        gsl_vector * target) const override  {
    Storage * s = storage<Storage>(data);
    if(! s->convolutionExpression)
      prepareConvolutionExpression(data);

    if(target->stride != 1)
      throw InternalError("ConvolutionFit doesn't support vectors with stride != 1");
    
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      underlyingFit->function(parameters, data, ds, s->buffer);
    }

    int conv_params = s->finalParameters.size() - s->originalParameters.size();
    // Now convolve
    double cv_vars[conv_params + 1];
    for(int i = 0; i < conv_params; i++)
      cv_vars[i+1] = parameters[s->originalParameters.size() + i];
    

    std::function<double (double v)> fn =
      [this, s, &cv_vars](double x) -> double {
        cv_vars[0] = x;
        return s->convolutionExpression->evaluate(cv_vars);
      };

    int nb = ds->nbRows();
    Vector::convolve(s->buffer.data(),
                     nb, target->data,
                     ds->x().first(), 
                     ds->x().last(),
                     fn,
                     isSymmetric,
                     s->buffer2.data()
                     );
  };



  ConvolutionFit(const QString & name,
                 const QString & formula,
                 PerDatasetFit * under,
                 bool symm) :
    PerDatasetFit(name, 
                  "Convolution fit",
                  "Convolution fit", 1, -1, false),
    underlyingFit(under),
    convolutionFormula(formula),
    isSymmetric(symm)
  {
    ArgumentList opts = underlyingFit->fitHardOptions();
    opts << underlyingFit->fitSoftOptions();
    makeCommands(ArgumentList(), NULL, NULL, opts);
    
  }
};

//////////////////////////////////////////////////////////////////////
// Now, the command !

static void convolveFit(const QString &, QString newName,
                        QString fitN,
                        QString convolution,
                        const CommandOptions & opts)
{
  
  QList<PerDatasetFit *> fts;
  bool overwrite  = false;
  updateFromOptions(opts, "redefine", overwrite);
  Fit::safelyRedefineFit(newName, overwrite);

  bool symmetric = false;
  updateFromOptions(opts, "symmetric", symmetric);

  PerDatasetFit * fit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(fitN));
  if(! fit)
    throw RuntimeError("Cannot find fit '%1 -- or it is a multibuffer fit'").arg(fitN);

  new ConvolutionFit(newName, convolution, fit, symmetric);
}


static ArgumentList 
rfA(QList<Argument *>() 
    << new StringArgument("name", "Name",
                          "name of the new fit")
    << new FitNameArgument("fit", "Fit",
                           "the fit to modify")
    << new StringArgument("convolution", "Convolution",
                          "The convolution formula (function of 'x')")
    );



static ArgumentList 
rfO(QList<Argument *>() 
    << new BoolArgument("redefine", 
                        "Redefine",
                        "If the new fit already exists, redefines it")
    << new BoolArgument("symmetric", 
                        "Symmetric",
                        "If true, the convolution is symmetric around 0 (default false)")
    );

static Command 
rf("convolve-fit",         // command name
   effector(convolveFit),  // action
   "fits",                      // group name
   &rfA,                        // arguments
   &rfO,                        // options
   "Convolve fit");
