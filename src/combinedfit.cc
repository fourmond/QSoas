/*
  combinedfit.cc: fit with automatic fitting of the combined
  Copyright 2012, 2013 by CNRS/AMU

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
#include <argumentlist.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>

#include <idioms.hh>

CombinedFit::Storage::~Storage()
{
  int sz = subs.size();
  for(int i = 0; i < sz; i++)
    delete subs[i];
}


FitInternalStorage * CombinedFit::allocateStorage(FitData * data) const
{
  Storage * s = new Storage;
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
  // Now, handle the parameters
  for(int i = 0; i < ownParameters.size(); i++)
    s->overallParameters << ParameterDefinition(ownParameters[i]);
  
  for(int i = 0; i < underlyingFits.size(); i++) {
    PerDatasetFit * f = underlyingFits[i];
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->subs[i]);
    QList<ParameterDefinition> params = f->parameters(data);
    s->firstParameterIndex << s->overallParameters.size();
    for(int j = 0; j < params.size(); j++) {
      ParameterDefinition def = params[j];
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

void CombinedFit::reserveBuffers(const DataSet * ds, FitData * data) const
{
  Storage * s = storage<Storage>(data);
  for(int i = 0; i < underlyingFits.size(); i++) {
    if(i >= s->buffers.size())
      s->buffers << Vector();
    s->buffers[i].resize(ds->x().size());
  }
}

void CombinedFit::function(const double * parameters,
                           FitData * data, 
                           const DataSet * ds,
                           gsl_vector * target) const
{
  Storage * s = storage<Storage>(data);
  reserveBuffers(ds, data);

  for(int i = 0; i < underlyingFits.size(); i++) {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                           s->subs[i]);
    gsl_vector_view v = 
      gsl_vector_view_array(s->buffers[i].data(), ds->x().size());
    underlyingFits[i]->function(parameters + s->firstParameterIndex[i], data, 
                                ds, &v.vector);
  }

  // Now, combine everything...
  QVarLengthArray<double, 100> args(1 + underlyingFits.size() + 
                                    s->overallParameters.size());
  for(int i = 0; i < ds->x().size(); i++) {
    args[0] = ds->x()[i];
    int base = 1;
    for(int j = 0; j < underlyingFits.size(); j++, base++)
      args[base] = s->buffers[j][i];
    for(int j = 0; j < s->firstParameterIndex[0]; j++, base++)
      args[base] = parameters[j];
    gsl_vector_set(target, i, formula.evaluate(args.data()));
  }
  
}

CombinedFit::CombinedFit(const QString & name, const QString & f, 
                         QList<PerDatasetFit *> fits) :
  PerDatasetFit(name.toLocal8Bit(), 
                "Combined fit",
                "Combined fit", 1, -1, false), 
  underlyingFits(fits), formula(f)

{

  QStringList params;
  params << "x";
  for(int i = 0; i < fits.size(); i++)
    params << QString("y%1").arg(i+1);

  params << formula.naturalVariables();
  Utils::makeUnique(params);

  formula.setVariables(params);
  ownParameters = params.mid(1 + fits.size());

  ArgumentList * opts = new ArgumentList();

  for(int i = 0; i < underlyingFits.size(); i++) {
    PerDatasetFit *f = underlyingFits[i];
    Command * cmd = Command::namedCommand("fit-" + f->fitName(false));
    *opts << *cmd->commandOptions();
  }

  /// @todo Global register of options for fits...

  makeCommands(NULL, NULL, NULL, opts);
}


// int CombinedFit::splicedIndex(FitData * data, int fit) const
// {
//   return firstParameterIndex[fit] * data->datasets.size();
// }

// void CombinedFit::spliceParameters(FitData * data, double * target, 
//                                    const double * source)
// {
//   int nbds = data->datasets.size();
//   int tparams = overallParameters.size();

//   for(int i = 0; i < underlyingFits.size(); i++) {
//     int base = firstParameterIndex[i];
//     int nbparams = firstParameterIndex.value(i+1, tparams) - base;
//     for(int j = 0; j < nbds; j++) {
//       for(int k = 0; k < nbparams; k++)
//         target[base * nbds + nbparams * j + k] = 
//           source[tparams * j + base + k];
//     }
//   }

// }

// void CombinedFit::unspliceParameters(FitData * data, double * target, 
//                                      const double * source)
// {
//   int nbds = data->datasets.size();
//   int tparams = overallParameters.size();

//   /// @todo rewrite both functions using lambdas !
//   for(int i = 0; i < underlyingFits.size(); i++) {
//     int base = firstParameterIndex[i];
//     int nbparams = firstParameterIndex.value(i+1, tparams) - base;
//     for(int j = 0; j < nbds; j++) {
//       for(int k = 0; k < nbparams; k++)
//         target[tparams * j + base + k] = 
//           source[base * nbds + nbparams * j + k];
//     }
//   }
// }

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

  for(int i = 0; i < fits.size(); i++) {
      QString fitName = fits[i];
      PerDatasetFit * fit = 
        dynamic_cast<PerDatasetFit *>(Fit::namedFit(fitName));
      
      if(! fit)
        throw RuntimeError("The fit " + fitName + " isn't working "
                           "buffer-by-buffer: impossible to combine "
                           "it with others");
      fts << fit;
      }
  new CombinedFit(newName, formula, fts);
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
    );

static Command 
cf("combine-fits",              // command name
    effector(combineFits),      // action
    "fits",                     // group name
    &cfA,                       // arguments
    &cfO,                       // options
    "Combine fits",
    "Combine multiple fits together");
