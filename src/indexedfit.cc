/*
  indexedfit.cc: fit with parameters indexed on meta-data
  Copyright 2022 by CNRS/AMU

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
#include <combinedfit.hh>

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

/// This fit takes another fit and allows some of the parameters to be
/// indexed to a meta-data.
///
/// In short, this allows to have "partially global" parameters
class IndexedFit : public PerDatasetFit {
protected:

  /// The underlying fit
  PerDatasetFit * underlyingFit;

  /// The indexed parameters
  QStringList indexedParameters;

  /// The (default) name of the meta involved. It can be redefined, so
  /// that one should rely on the value in Storage
  QString metaName;
  

  class Storage : public FitInternalStorage {
  public:

    /// The actual name of the meta used for indexing
    QString metaName;

    /// Parameters of the underlying fit
    QList<ParameterDefinition> originalParameters;

    /// Final parameters
    QList<ParameterDefinition> finalParameters;

    /// The first parameter that corresponds to an indexed parameter
    int indexedBase;

    /// The correspondance "meta-data value (as string)" ->
    /// index in the parameters
    QHash<QString, int> metaIndices;

    /// The skipped indices
    QSet<int> skippedIndices;

    /// The correspondance index of the indexed parameter -> index of
    /// the original parameter.
    QList<int> targets;

    /// Storage for the underlying fit
    FitInternalStorage * underlyingStorage;
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

  /// Returns the value of the indexing meta for the given dataset.
  QString metaValue(FitData * data, const DataSet * ds) const {
    Storage * s = storage<Storage>(data);
    const QString & n = s->metaName;
    if(ds->hasMetaData(n)) {
      QVariant v = ds->getMetaData(n);
      QString sv = v.toString();
      if(! sv.isEmpty())
        return sv;
    }
    return "none";
  };

  void prepareParameters(FitData * data) const {
    Storage * s = storage<Storage>(data);
    s->metaIndices.clear();
    QStringList values;
    for(const DataSet * ds : data->datasets) {
      QString v = metaValue(data, ds);
      if(! s->metaIndices.contains(v)) {
        s->metaIndices[v] = values.size();
        values << v;
      }
    }

    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      s->originalParameters = underlyingFit->parameters(data);
    }
    s->finalParameters.clear();

    QList<ParameterDefinition> additional;

    
    for(int i = 0; i < s->originalParameters.size(); i++) {
      const ParameterDefinition & def = s->originalParameters[i];
      if(indexedParameters.contains(def.name)) {
        s->skippedIndices.insert(i);
        s->targets << i;
        for(const QString & v : values) {
          additional << def;
          additional.last().name += "_" + v;
        }
      }
      else
        s->finalParameters << def;
    }
    s->indexedBase = s->finalParameters.size();

    s->finalParameters += additional;
  };

  virtual void processOptions(const CommandOptions & opts, FitData * data) const override
  {
    Storage * s = storage<Storage>(data);

    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      Fit::processOptions(underlyingFit, opts, data);
    }

    // Now, should process the parameters
    s->metaName = metaName;
    // change from option ?
    prepareParameters(data);
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

    QStringList defs = indexedParameters;
    std::sort(defs.begin(), defs.end());

    return QString("fit: %1%2, with %3 indexed").
      arg(underlyingFit->fitName(false)).arg(so).
      arg(defs.join(", "));
  };

  virtual QList<ParameterDefinition> parameters(FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    return s->finalParameters;
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);
    double ii[s->originalParameters.size()];

    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      underlyingFit->initialGuess(data, ds, ii);
    }
    Utils::skippingCopy(ii, a, s->originalParameters.size(),
                        s->skippedIndices, true);
    // Then we initialize the final parameters, using the values that
    // were guessed by the initial fit.
    for(int i = 0; i < s->targets.size(); i++) {
      for(int j = 0; j < s->metaIndices.size(); j++) {
        a[s->indexedBase + i * s->metaIndices.size() + j] =
          ii[s->targets[i]];
      }
    }
  }

  virtual void function(const double * parameters,
                        FitData * data, 
                        const DataSet * ds,
                        gsl_vector * target) const override
  {
    Storage * s = storage<Storage>(data);
    double buf[s->originalParameters.size()];
    QString mv = metaValue(data, ds);
    
    int midx = s->metaIndices[mv];

    // QTextStream o(stdout);
    // o << "Ds: " << ds << " -> " << mv << " (" << midx << ")" << endl;

    Utils::skippingCopy(parameters, buf,
                        s->originalParameters.size(),
                        s->skippedIndices);
    for(int i = 0; i < s->targets.size(); i++) {
        buf[s->targets[i]] =
          parameters[s->indexedBase + i * s->metaIndices.size() + midx];
    }

    
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      underlyingFit->function(buf, data, ds, target);
    }
  };

  /// @todo Subfunctions !

  // virtual void computeSubFunctions(const double * parameters,
  //                                  FitData * data, 
  //                                  const DataSet * ds,
  //                                  QList<Vector> * targetData,
  //                                  QStringList * targetAnnotations) const override
  // {
  //   Storage * s = storage<Storage>(data);
  //   double buf[s->originalParameters.size()];

  //   computeParameters(data, ds, parameters, buf);
    
  //   TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
  //                                                       s->underlyingStorage);
  //   underlyingFit->computeSubFunctions(buf, data, ds,
  //                                      targetData, targetAnnotations);
  // };


  IndexedFit(const QString & name,
             const QStringList & indexed,
             const QString & mn,
             PerDatasetFit * under) :
    PerDatasetFit(name, 
                "Indexed fit",
                "Indexed fit", 1, -1, false),
  underlyingFit(under),
  indexedParameters(indexed),
  metaName(mn)  {
    ArgumentList opts = underlyingFit->fitHardOptions();
    opts << underlyingFit->fitSoftOptions();
    makeCommands(ArgumentList(), NULL, NULL, opts);
  };
  
};

//////////////////////////////////////////////////////////////////////
// Now, the command !

static void defineIndexedFit(const QString &, QString newName,
                             QString fitN,
                             QString meta,
                             QStringList params,
                             const CommandOptions & opts)
{
  
  QList<PerDatasetFit *> fts;
  bool overwrite  = false;
  updateFromOptions(opts, "redefine", overwrite);
  Fit::safelyRedefineFit(newName, overwrite);

  PerDatasetFit * fit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(fitN));
  if(! fit)
    throw RuntimeError("Cannot find fit %1").arg(fitN);

  new IndexedFit(newName, params, meta, fit);
}


static ArgumentList 
rfA(QList<Argument *>() 
    << new StringArgument("name", "Name",
                          "name of the new fit")
    << new FitNameArgument("fit", "Fit",
                           "the fit to modify")
    << new StringArgument("meta", "Meta",
                          "Meta-data to index on")
    << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                  "parameters", 
                                  "Parameters",
                                  "a comma-separated list of parameters to index")
    );



static ArgumentList 
rfO(QList<Argument *>() 
    << new BoolArgument("redefine", 
                        "Redefine",
                        "If the new fit already exists, redefines it")
    );

static Command 
rf("define-indexed-fit",         // command name
   effector(defineIndexedFit),   // action
   "fits",                       // group name
   &rfA,                         // arguments
   &rfO,                         // options
   "Define an indexed fit");
