/*
  distribfit.cc: fit with automatic fitting of the distrib
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
#include <distribfit.hh>

#include <fitdata.hh>
#include <dataset.hh>
#include <perdatasetfit.hh>
#include <command.hh>
#include <argumentlist.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>

#include <multiintegrator.hh>

#include <idioms.hh>

DistribFit::Storage::~Storage()
{
  delete sub;
}


FitInternalStorage * DistribFit::allocateStorage(FitData * data) const
{
  Storage * s = new Storage;
  s->sub = underlyingFit->allocateStorage(data);
  return s;
}

FitInternalStorage * DistribFit::copyStorage(FitData * data,
                                             FitInternalStorage * source,
                                             int ds) const
{
  Storage * s = static_cast<Storage *>(source);
  Storage * s2 = new Storage(*s);
  s2->sub = NULL;
  {
    TemporaryChange<FitInternalStorage*> d(data->fitStorage,
                                           s->sub);
    s2->sub = underlyingFit->copyStorage(data, s->sub, ds);
  }
  throw InternalError("Cannot copy integrators");
  return s2;
}


void DistribFit::processOptions(const CommandOptions & opts, FitData * data) const
{
  Storage * s = storage<Storage>(data);
  
  {
    TemporaryChange<FitInternalStorage*> d(data->fitStorage,
                                           s->sub);
    Fit::processOptions(underlyingFit, opts, data);
    s->parametersCache = underlyingFit->parameters(data);

    s->parameterIndex = -1;
    for(int i = 0; i < s->parametersCache.size(); i++) {
      if(s->parametersCache[i].name == parameterName) {
        s->parameterIndex = i;
        break;
      }
    }
    if(s->parameterIndex < 0)
      throw RuntimeError("Parameter %1 is missing from the fit %2").
        arg(parameterName).arg(underlyingFit->fitName(true, data));

  }

  s->parametersCache.takeAt(s->parameterIndex);
  s->distribIndex = s->parametersCache.size();

  /// @todo Process own options
  s->dist = defaultDistribution;

  s->parametersCache << s->dist->parameters(parameterName);

  processSoftOptions(opts, data);
}

void DistribFit::processSoftOptions(const CommandOptions & opts, FitData * data) const
{
  Storage * s = storage<Storage>(data);
  {
    TemporaryChange<FitInternalStorage*> d(data->fitStorage,
                                           s->sub);
    Fit::processOptions(underlyingFit, opts, data);
  }
  if(s->integrator)
    delete s->integrator;
  s->integrator = MultiIntegrator::fromOptions(opts,
                                               MultiIntegrator::Function(), 0);
}

void DistribFit::initialGuess(FitData * data, 
                              const DataSet * ds,
                              double * guess) const
{
  Storage * s = storage<Storage>(data);
  {
    TemporaryChange<FitInternalStorage*> d(data->fitStorage,
                                           s->sub);
    underlyingFit->initialGuess(data, ds, guess);
  }
  double value = guess[s->parameterIndex];
  for(int i = s->parameterIndex; i < s->distribIndex; i++)
    guess[i] = guess[i+1];
  s->dist->initialGuess(guess + s->distribIndex, value);
}

ArgumentList * DistribFit::fitHardOptions() const
{
  ArgumentList * rv = new ArgumentList(*underlyingFit->fitHardOptions());
  return rv;
}


QString DistribFit::optionsString(FitData * data) const
{
  Storage * s = storage<Storage>(data);
  QString sub;
  {
    TemporaryChange<FitInternalStorage*> d(data->fitStorage, s->sub);
    sub = underlyingFit->fitName(true, data);
  }
  return QString("%1 with %2 %3").arg(sub).arg(s->dist->name).arg(parameterName);
}

QList<ParameterDefinition> DistribFit::parameters(FitData * data) const
{
  Storage * s = storage<Storage>(data);
  return s->parametersCache;
}


DistribFit::~DistribFit()
{
  
}


void DistribFit::function(const double * parameters,
                            FitData * data, 
                            const DataSet * ds,
                            gsl_vector * target) const
{
  Storage * s = storage<Storage>(data);
  TemporaryChange<FitInternalStorage*> dd(data->fitStorage,
                                          s->sub);

  // Splice parameters
  double d[s->distribIndex + 1];
  const double * distParams = parameters + s->distribIndex;
  for(int i = 0; i < s->parameterIndex; i++)
    d[i] = parameters[i];
  for(int i = s->parameterIndex; i < s->distribIndex; i++)
    d[i+1] = parameters[i];
  MultiIntegrator::Function fcn = [data, s, &d, ds, distParams, this](double x, gsl_vector * tgt) {
    d[s->parameterIndex] = x;
    underlyingFit->function(d, data, ds, tgt);
    gsl_vector_scale(tgt, s->dist->weight(distParams, x));
  };

  s->integrator->reset(fcn, target->size);
  double left, right;
  s->dist->range(distParams, &left, &right);
  s->integrator->integrate(target, left, right);
  gsl_vector_scale(target, 1/s->dist->rangeWeight(left, right));
}


ArgumentList * DistribFit::fitSoftOptions() const
{
  ArgumentList * rv = new ArgumentList(*underlyingFit->fitHardOptions());
  (*rv) << MultiIntegrator::integratorOptions();
  return rv;
}

DistribFit::DistribFit(const QString & name, const QString & param, 
                       PerDatasetFit *fit) :
  PerDatasetFit(name.toLocal8Bit(), 
                "Distrib fit",
                "Distrib fit", 1, -1, false), 
  underlyingFit(fit)

{

  // QStringList params;
  // params << "x";
  // for(int i = 0; i < fits.size(); i++)
  //   params << QString("y%1").arg(i+1);

  // params << formula.naturalVariables();
  // Utils::makeUnique(params);

  // formula.setVariables(params);
  // ownParameters = params.mid(1 + fits.size());

  // ArgumentList * opts = new ArgumentList();

  // for(int i = 0; i < underlyingFits.size(); i++) {
  //   PerDatasetFit *f = underlyingFits[i];
  //   Command * cmd = Command::namedCommand("fit-" + f->fitName(false));
  //   *opts << *cmd->commandOptions();
  // }

  // /// @todo Global register of options for fits...

  // makeCommands(NULL, NULL, NULL, opts);
}

// //////////////////////////////////////////////////////////////////////
// // Now, the command !

// static void combineFits(const QString &, QString newName,
//                         QString formula,
//                         QStringList fits,
//                         const CommandOptions & opts)
// {
  
//   QList<PerDatasetFit *> fts;
//   if(Fit::namedFit(newName))
//     throw RuntimeError("Fit '%1' already exists").arg(newName);
//   for(int i = 0; i < fits.size(); i++) {
//       QString fitName = fits[i];
//       PerDatasetFit * fit = 
//         dynamic_cast<PerDatasetFit *>(Fit::namedFit(fitName));
      
//       if(! fit)
//         throw RuntimeError("The fit " + fitName + " isn't working "
//                            "buffer-by-buffer: impossible to combine "
//                            "it with others");
//       fts << fit;
//       }
//   new DistribFit(newName, formula, fts);
// }

// static ArgumentList 
// cfA(QList<Argument *>() 
//     << new StringArgument("name", "Name",
//                           "The name of the new fit")
//     << new StringArgument("formula", "Formula",
//                           "How to combine the various fits")
//     << new SeveralChoicesArgument(&Fit::availableFits,
//                                   "fits", "Fits",
//                                   "The fit to combine together"));

// static Command 
// cf("combine-fits", // command name
//     effector(combineFits), // action
//     "fits",  // group name
//     &cfA, // arguments
//     NULL, // options
//     "Combine fits",
//     "Combine different fits together",
//     "(...)");
