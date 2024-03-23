/*
  monotonefit.cc: fit constrained by sign
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

#include <idioms.hh>

#include <soas.hh>
#include <datasetexpression.hh>

/// This fit creates a new fit, but with an additional restriction in
/// terms of signs of arbitrary derivatives.
///
/// That is based on a string, whose length corresponds to the number
/// of derivatives we're looking at, and whose values are:
/// * '=' for a single sign (no matter what)
/// * '*' for not checked
/// * '+' for positive
/// * '-' for negative
///
/// Thus, a monotonic function can be represented thus: '*='
class MonotoneFit : public PerDatasetFit {
protected:

  /// The underlying fit
  PerDatasetFit * underlyingFit;

  class Storage : public FitInternalStorage {
  public:
    QString signs;

        /// Storage for the underlying fit
    FitInternalStorage * underlyingStorage;

    /// The indices skipped from copying from full to final
    QSet<int> skippedIndices;
    
    virtual ~Storage()
    {
      delete underlyingStorage;
    };

    Storage() {
    };
    
    Storage(const Storage & o) :
      signs(o.signs),
      underlyingStorage(NULL)
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


  virtual void processOptions(const CommandOptions & opts, FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    s->signs = "*=";
    updateFromOptions(opts, "signs", s->signs);
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      Fit::processOptions(underlyingFit, opts, data);
    }
  }

  virtual bool hasSubFunctions (FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->underlyingStorage);
    return underlyingFit->hasSubFunctions(data);
  };

  virtual bool displaySubFunctions (FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->underlyingStorage);
    return underlyingFit->displaySubFunctions(data);
  };


  ArgumentList fitSoftOptions() const override
  {
    ArgumentList lst = Fit::fitSoftOptions(underlyingFit);
    lst << new StringArgument("signs", "Signs", "Signs of the various derivatives");
    return lst;
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
    s->signs = "*=";            // Default
    updateFromOptions(opts, "signs", s->signs);
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

    return QString("fit: %1%2, with sign restrictions").
      arg(underlyingFit->fitName(false)).arg(so);
  };

  virtual QList<ParameterDefinition> parameters(FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->underlyingStorage);
    return underlyingFit->parameters(data);
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      underlyingFit->initialGuess(data, ds, a);
    }
  }

  virtual void function(const double * parameters,
                        FitData * data, 
                        const DataSet * ds,
                        gsl_vector * target) const override
  {
    Storage * s = storage<Storage>(data);
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      underlyingFit->function(parameters, data, ds, target);
    }
    // Now checking

    /// @todo Very bad in terms of memory allocation
    Vector tmp = ds->y();
    gsl_vector_memcpy(tmp, target);
    Vector tmp2 = tmp;

    for(int i = 0; i < s->signs.size(); i++) {
      if(s->signs[i] == '*')
        continue;
      if(i == 0)
        tmp2 = tmp;
      else
        DataSet::arbitraryDerivative(i, std::min(i+2, 5), tmp.size(),
                                     ds->x().data(),
                                     tmp.data(), tmp2.data());
      // Let's look at the sign
      int sgn = 0;
      bool uniform = true;
      for(int j = 0; j < tmp2.size(); j++) {
        double v = tmp2[j];
        if(sgn == 0) {
          if(v > 0)
            sgn = 1;
          else if(v < 0)
            sgn = -1;
        }
        else {
          if(sgn * v < 0) {
            uniform = false;
            break;
          }
        }
      }
      if(! uniform)
        throw RangeError("Not uniform %1th derivative").arg(i);
      if(s->signs[i] == '+' && sgn < 0)
        throw RangeError("%1th derivative is negative but should be positive").arg(i);
      if(s->signs[i] == '-' && sgn > 0)
        throw RangeError("%1th derivative is positive but should be negative").arg(i);
    }
    
  };

  virtual void computeSubFunctions(const double * parameters,
                                   FitData * data, 
                                   const DataSet * ds,
                                   QList<Vector> * targetData,
                                   QStringList * targetAnnotations) const override
  {
    Storage * s = storage<Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
    underlyingFit->computeSubFunctions(parameters, data, ds,
                                       targetData, targetAnnotations);
  };


  MonotoneFit(const QString & name,
              PerDatasetFit * under) :
  PerDatasetFit(name, 
                "Monotone fit",
                "Monotone fit", 1, -1, false),
  underlyingFit(under)
  {
    ArgumentList opts = underlyingFit->fitHardOptions();
    opts << fitSoftOptions();
    makeCommands(ArgumentList(), NULL, NULL, opts);
  }

  ~MonotoneFit() {
  };
  
};

//////////////////////////////////////////////////////////////////////
// Now, the command !

static void defineMonotoneFit(const QString &, QString newName,
                              QString fitN,
                              const CommandOptions & opts)
{
  
  QList<PerDatasetFit *> fts;
  bool overwrite  = false;
  updateFromOptions(opts, "redefine", overwrite);
  Fit::safelyRedefineFit(newName, overwrite);

  PerDatasetFit * fit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(fitN));
  if(! fit)
    throw RuntimeError("Cannot find fit %1").arg(fitN);

  new MonotoneFit(newName, fit);
}


static ArgumentList 
mfA(QList<Argument *>() 
    << new StringArgument("name", "Name",
                          "name of the new fit")
    << new FitNameArgument("fit", "Fit",
                           "the fit to modify")
    );



static ArgumentList 
mfO(QList<Argument *>() 
    << new BoolArgument("redefine", 
                        "Redefine",
                        "If the new fit already exists, redefines it")
    );

static Command 
rf("define-monotone-fit",         // command name
   effector(defineMonotoneFit),  // action
   "fits",                      // group name
   &mfA,                        // arguments
   &mfO,                        // options
   "Define a monotone fit",
   "Forces a fit function to be monotone");
