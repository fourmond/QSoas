/*
  combinedfit.cc: combine several fits with a formula
  Copyright 2012, 2013, 2024 by CNRS/AMU

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

#include <fitdata.hh>
#include <dataset.hh>
#include <perdatasetfit.hh>
#include <command.hh>
#include <commandcontext.hh>
#include <argumentlist.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>

#include <argument-templates.hh>

#include <debug.hh>

#include <idioms.hh>


static QHash<QString, CombinedFit::DuplicatesMode> duplicatesNames =
  { {"suffix", CombinedFit::Suffix},
    {"suffix-if-needed", CombinedFit::SuffixIfNeeded},
    {"fail", CombinedFit::Fail},
    {"merge", CombinedFit::Merge}
  };


/// The cache for a single buffer
class BufferCache {
public:

  /// The computed Y values for each fit
  QList<Vector> fitValues;

  /// The corresponding parameters (global)
  Vector parameters;

  /// Returns true if the parameters for the fit number @a fit have changed since the last call to usedParameters
  bool hasChanged(int fit, const double * testParams,
                  int total, const QList<int> & indices) const {
    if(parameters.size() == 0)
      return true;
    int i = indices[fit];
    int end = indices.value(fit+1, total);
    for(;i < end; i++) {
      if(testParams[i] != parameters[i])
        return true;
    }
    return false;
  };

  void usedParameters(const double * used,
                      int total) {
    parameters.resize(total);
    for(int i = 0; i < total; i++)
      parameters[i] = used[i];
  }

  BufferCache(const DataSet * ds, int nbFits) {
    for(int i = 0; i < nbFits; i++)
      fitValues << ds->x();     // As good an initial value as it gets
  };

  BufferCache() {
  };

};

//////////////////////////////////////////////////////////////////////

CombinedFit::Storage::~Storage()
{
  int sz = subs.size();
  for(int i = 0; i < sz; i++)
    delete subs[i];
}


FitInternalStorage * CombinedFit::allocateStorage(FitData * data) const
{
  Storage * s = new Storage;
  s->duplicates = defaultMode;
  int sz = underlyingFits.size();
  for(int i = 0; i < sz; i++)
    s->subs << underlyingFits[i]->allocateStorage(data);
  return s;
}

FitInternalStorage * CombinedFit::copyStorage(FitData * data,
                                              FitInternalStorage * source,
                                              int ds) const
{
  Storage * s = static_cast<Storage *>(source);
  Storage * s2 = new Storage;
  int sz = underlyingFits.size();
  for(int i = 0; i < sz; i++) {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->subs[i]);
    s2->subs << underlyingFits[i]->copyStorage(data, s->subs[i], ds);
  }
  return s2;
}


void CombinedFit::processOptions(const CommandOptions & opts, FitData * data) const
{
  Storage * s = storage<Storage>(data);

  for(int i = 0; i < underlyingFits.size(); i++) {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->subs[i]);
    Fit::processOptions(underlyingFits[i], opts, data);
  }

  /// @todo any own options ?

  s->overallParameters.clear();
  s->firstParameterIndex.clear();
  s->duplicates = defaultMode;
  updateFromOptions(opts, "duplicates", s->duplicates);
  if(s->duplicates == Merge)
    throw RuntimeError("Merging duplicate parameters is not yet supported, sorry about that");
  
  // // Now, handle the parameters

  /// a hash parameter name -> list of fits in which that one is used,
  /// with -1 designating the combination's own parameters
  QHash<QString, QList<int> > definedParameters;
  
  for(const QString & n : ownParameters) {
    definedParameters[n] = QList<int>( {-1});
    s->overallParameters << ParameterDefinition(n);
  }

  QSet<QString> duplicates;

  s->originalParameters.clear();
  for(int i = 0; i < underlyingFits.size(); i++) {
    PerDatasetFit * f = underlyingFits[i];
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->subs[i]);
    QList<ParameterDefinition> defs = f->parameters(data);
    s->originalParameters << defs;
    for(const ParameterDefinition & def : defs) {
      if(! definedParameters.contains(def.name)) {
        definedParameters[def.name] = QList<int>();
      }
      else {
        duplicates.insert(def.name);
      }
      definedParameters[def.name] << i;
    }
  }

  if(duplicates.size() > 0 && s->duplicates == Fail)
    throw RuntimeError("The fail mode has been selected and "
                       "these parameters are in duplicate '%1'").
      arg(duplicates.toList().join("', '"));

  
  for(int i = 0; i < underlyingFits.size(); i++) {
    s->firstParameterIndex << s->overallParameters.size();
    for(ParameterDefinition def : s->originalParameters[i]) {
      if((s->duplicates == Suffix) ||
         duplicates.contains(def.name))
        def.name = QString("%1_f#%2").arg(def.name).arg(i+1);
      s->overallParameters << def;
    }
  }

}

void CombinedFit::initialGuess(FitData * data, 
                               const DataSet * ds,
                               double * guess) const
{
  Storage * s = storage<Storage>(data);
  for(int i = 0; i < ownParameters.size(); i++)
    guess[i] = 1;               // safe guess ?
  for(int i = 0; i < underlyingFits.size(); i++) {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->subs[i]);
    underlyingFits[i]->initialGuess(data, ds, 
                                    guess + s->firstParameterIndex[i]);
  }
}


QString CombinedFit::optionsString(FitData * data) const
{
  QStringList strs;
  Storage * s = storage<Storage>(data);
  for(int i = 0; i < underlyingFits.size(); i++) {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->subs[i]);
    strs << QString("y%1: %2").arg(i+1).
      arg(underlyingFits[i]->fitName(true, data));
  }
  return QString("%1 with %2").arg(formula.formula()).
    arg(strs.join(", "));
}

QList<ParameterDefinition> CombinedFit::parameters(FitData * data) const
{
  Storage * s = storage<Storage>(data);
  return s->overallParameters;
}


CombinedFit::~CombinedFit()
{
  
}

void CombinedFit::function(const double * parameters,
                           FitData * data, 
                           const DataSet * ds,
                           gsl_vector * target) const
{
  Storage * s = storage<Storage>(data);
  if(! s->cache.contains(ds))
    s->cache.insert(ds, BufferCache(ds, underlyingFits.size()));

  BufferCache & cache = s->cache[ds];

  for(int i = 0; i < underlyingFits.size(); i++) {
    if(cache.hasChanged(i, parameters, s->overallParameters.size(),
                        s->firstParameterIndex)) {
      if(data->debug > 0) {
        Debug::debug() << "Combined fits: recomputing fit #" << i
                       << endl;
      }
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->subs[i]);
      gsl_vector_view v = 
        gsl_vector_view_array(cache.fitValues[i].data(),
                              cache.fitValues[i].size());
      underlyingFits[i]->function(parameters + s->firstParameterIndex[i], data, 
                                  ds, &v.vector);
    }
    else {
      if(data->debug > 0) {
        Debug::debug() << "Combined fits: not recomputing fit #" << i
                       << endl;
      }
    }
  }
  cache.usedParameters(parameters, s->overallParameters.size());

  // Now, combine everything...
  QVarLengthArray<double, 100> args(1 + underlyingFits.size() + 
                                    s->overallParameters.size());
  for(int i = 0; i < ds->x().size(); i++) {
    args[0] = ds->x()[i];
    int base = 1;
    for(int j = 0; j < underlyingFits.size(); j++, base++)
      args[base] = cache.fitValues[j][i];
    for(int j = 0; j < s->firstParameterIndex[0]; j++, base++)
      args[base] = parameters[j];
    gsl_vector_set(target, i, formula.evaluate(args.data()));
  }
  
}

CombinedFit::CombinedFit(const QString & name, const QString & f, 
                         QList<PerDatasetFit *> fits, DuplicatesMode dup) :
  PerDatasetFit(name, 
                "Combined fit",
                "Combined fit", 1, -1, false),
  defaultMode(dup),
  underlyingFits(fits), formula(f)

{

  for(PerDatasetFit * fit : fits) {
    if(fit->fitArguments().size() > 0)
      throw InternalError("Cannot use CombinedFit with fits taking arguments, here '%1'").arg(fit->fitName(false));
  }

  QStringList params;
  params << "x";
  for(int i = 0; i < fits.size(); i++)
    params << QString("y%1").arg(i+1);

  params << formula.naturalVariables();
  Utils::makeUnique(params);

  formula.setVariables(params);
  ownParameters = params.mid(1 + fits.size());


  for(int i = 0; i < underlyingFits.size(); i++) {
    PerDatasetFit *f = underlyingFits[i];
    Command * cmd = CommandContext::globalContext()->
      namedCommand("fit-" + f->fitName(false));
    softOptions.mergeOptions(f->fitSoftOptions());
    hardOptions.mergeOptions(f->fitHardOptions());
  }

  // the choice of the duplication mode
  hardOptions << new TemplateChoiceArgument<DuplicatesMode>
    (::duplicatesNames, "duplicates", "Duplicates",
     "how duplicate parameters are handled");


  makeCommands();
}

ArgumentList CombinedFit::fitHardOptions() const
{
  return hardOptions;
}

ArgumentList CombinedFit::fitSoftOptions() const
{
  return softOptions;
}

void CombinedFit::processSoftOptions(const CommandOptions & opts,
                                     FitData * data) const
{
  Storage * s = storage<Storage>(data);

  for(int i = 0; i < underlyingFits.size(); i++) {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->subs[i]);
    underlyingFits[i]->processSoftOptions(opts, data);
  }
}


CommandOptions CombinedFit::currentSoftOptions(FitData * data) const
{
  CommandOptions opts;
  Storage * s = storage<Storage>(data);

  for(int i = 0; i < underlyingFits.size(); i++) {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->subs[i]);
    opts.unite(underlyingFits[i]->currentSoftOptions(data));
  }
  return opts;
}



//////////////////////////////////////////////////////////////////////
// Now, the command !

static void combineFits(const QString &, QString newName,
                        QString formula,
                        QStringList fits,
                        const CommandOptions & opts)
{
  
  QList<PerDatasetFit *> fts;
  bool overwrite  = false;
  updateFromOptions(opts, "redefine", overwrite);
  Fit::safelyRedefineFit(newName, overwrite);

  CombinedFit::DuplicatesMode duplicates= CombinedFit::Suffix;
  updateFromOptions(opts, "duplicates", duplicates);

  for(int i = 0; i < fits.size(); i++) {
      QString fitName = fits[i];
      PerDatasetFit * fit = 
        dynamic_cast<PerDatasetFit *>(Fit::namedFit(fitName));
      if(! fit)
        throw RuntimeError("The fit " + fitName + " isn't working "
                           "buffer-by-buffer: impossible to combine "
                           "it with others");
      if(fit->fitArguments().size() > 0)
        throw RuntimeError("Cannot use combine-fits with fits taking arguments, here '%1'").arg(fit->fitName(false));
      fts << fit;
  }
  new CombinedFit(newName, formula, fts, duplicates);
}


static ArgumentList 
cfA(QList<Argument *>() 
    << new StringArgument("name", "Name",
                          "name of the new fit")
    << new StringArgument("formula", "Formula",
                          "how to combine the various fits")
    << new SeveralFitNamesArgument("fits", "Fits",
                                   "the fits to combine together"));

static ArgumentList 
cfO(QList<Argument *>() 
    << new BoolArgument("redefine", 
                        "Redefine",
                        "If the new fit already exists, redefines it")
    << new TemplateChoiceArgument<CombinedFit::DuplicatesMode>
    (::duplicatesNames,
     "duplicates", "Duplicates",
     "how duplicate parameters are handled")
    );

static Command 
cf("combine-fits",              // command name
    effector(combineFits),      // action
    "fits",                     // group name
    &cfA,                       // arguments
    &cfO,                       // options
    "Combine fits",
    "Combine multiple fits together");
