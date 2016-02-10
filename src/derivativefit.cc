/*
  derivativefit.cc: fit with automatic fitting of the derivative
  Copyright 2012, 2013, 2014, 2016 by CNRS/AMU

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
#include <derivativefit.hh>

#include <fitdata.hh>
#include <dataset.hh>
#include <perdatasetfit.hh>
#include <command.hh>
#include <argumentlist.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>
#include <possessive-containers.hh>

#include <idioms.hh>


DerivativeFit::Storage::~Storage()
{
  delete originalStorage;
  if(sameX)
    delete[] sameX;
  for(int i = 0; i < splittedDatasets.size(); i++)
    delete splittedDatasets[i];
}

DerivativeFit::Storage::Storage() :
  sameX(NULL), originalStorage(NULL)
{
}

DerivativeFit::Storage::Storage(const DerivativeFit::Storage & o) :
  originalParameters(o.originalParameters), buffers(o.buffers),
  sameX(NULL), originalStorage(NULL)
  // The copy constructor cannot copy the originalStorage !
{
  if(o.sameX) {
    sameX = new bool[buffers.size()];
    memcpy(sameX, o.sameX, buffers.size()*sizeof(bool));
  }
  for(int i = 0; i < o.splittedDatasets.size(); i++)
    splittedDatasets << new DataSet(*o.splittedDatasets[i]);
}

//////////////////////////////////////////////////////////////////////

bool DerivativeFit::threadSafe() const
{
  return true;                  // YES
}



/// Redirectors

void DerivativeFit::processOptions(const CommandOptions & opts,
                                   FitData * data) const
{
  Storage * s = storage<Storage>(data);
  if(mode == Combined) {
    s->sameX = new bool[data->datasets.size()];
    for(int i = 0; i < data->datasets.size(); i++) {
      const DataSet * ds = data->datasets[i];
      PossessiveList<DataSet> sub(ds->splitIntoMonotonic());
      if(sub.size() != 2)
        throw RuntimeError(QString("Dataset '%1' should be made of two "
                                   "monotonic parts !").arg(ds->name));
      sub.detach();
      s->splittedDatasets << sub[0] << sub[1];
      s->sameX[i] = sub[0]->x() == sub[1]->x();
    }
  }

  TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
  Fit::processOptions(underlyingFit, opts, data);
}


QString DerivativeFit::optionsString(FitData *  data) const
{
  QString n;
  switch(mode) {
  case DerivativeOnly:
    n = " only";break;
  case Separated:
    n = " (separated)"; break;
  case Combined:
    n = " (combined)"; break;
  default:
    ;
  }
  Storage * s = storage<Storage>(data);
  TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
  return Fit::optionsString(underlyingFit, data) + " -- derivative" + n;
}

ArgumentList * DerivativeFit::fitHardOptions() const
{
  return Fit::fitHardOptions(underlyingFit);
}

ArgumentList * DerivativeFit::fitSoftOptions() const
{
  return Fit::fitSoftOptions(underlyingFit);
}

CommandOptions DerivativeFit::currentSoftOptions(FitData * data) const
{
  Storage * s = storage<Storage>(data);
  TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
  return Fit::currentSoftOptions(underlyingFit, data);
}

void DerivativeFit::processSoftOptions(const CommandOptions & opts, FitData * data) const
{
  Storage * s = storage<Storage>(data);
  TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
  Fit::processSoftOptions(underlyingFit, opts, data);
}  

void DerivativeFit::checkDatasets(const FitData * data) const
{
  if(mode == Separated) {
    if(data->datasets.size() != 2)
      throw RuntimeError("Fit " + name + " needs exactly two buffers");
  }
  // No restriction in the other cases
}

QList<ParameterDefinition> DerivativeFit::parameters(FitData * data) const
{
  Storage * s = storage<Storage>(data);
  
  TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
  QList<ParameterDefinition> params = underlyingFit->parameters(data);

  s->originalParameters = params.size();
  for(int i = 0; i < s->originalParameters; i++)
    params[i].canBeBufferSpecific = (mode != Separated);
  if(mode == Combined)
    params << ParameterDefinition("deriv_scale", true); 
  /// @todo add index of switch ?
  return params;
}

void DerivativeFit::initialGuess(FitData * data, double * guess) const
{
  Storage * s = storage<Storage>(data);
  int tp = data->parametersPerDataset();
  TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
  for(int i = 0; i < data->datasets.size(); i++) {
    underlyingFit->initialGuess(data, data->datasets[i], guess + i*tp);
    if(mode == Combined)
      guess[i*tp + s->originalParameters] = 1; // Makes a good default scale !
  }
}

QString DerivativeFit::annotateDataSet(int idx, FitData *) const
{
  if(mode != Separated)
    return QString();
  if(idx == 0)
    return "function";
  return "derivative";
}

DerivativeFit::~DerivativeFit()
{
}

void DerivativeFit::reserveBuffers(FitData * data) const
{
  Storage * s = storage<Storage>(data);
  for(int i = 0; i < data->datasets.size(); i++) {
    if(i >= s->buffers.size())
      s->buffers << Vector();
    s->buffers[i].resize(data->datasets[i]->x().size());
  }
}

/// @bug There is a potential bug here, since we don't change the
/// datasets from FitData, so any PerDatasetFit that needs
/// FitData->datasets will fail. This has been here for a while,
/// though.
void DerivativeFit::function(const double * parameters,
                             FitData * data, gsl_vector * target) const
{
  reserveBuffers(data);
  Storage * s = storage<Storage>(data);

  TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);

  int i = 0;
  if(mode == Separated) {
    gsl_vector_view fnView = data->viewForDataset(0, target);
    const DataSet * baseDS = data->datasets[0];

    underlyingFit->function(parameters, data,
                            baseDS, &fnView.vector);
    ++i;
  }                      
  for(; i < data->datasets.size(); ++i) {
    gsl_vector_view derView = data->viewForDataset(i, target);
    gsl_vector_view bufView = s->buffers[i].vectorView(); 

    const DataSet * derDS = data->datasets[i];

    int pbase = i * data->parametersPerDataset();

    if(mode == Combined) {
      const DataSet * main = s->splittedDatasets[2*i];
      const DataSet * sub = s->splittedDatasets[2*i+1];

      // We create sub-views of the original view...
      int idx = main->x().size();
      gsl_vector_view sFnView = gsl_vector_subvector(&derView.vector, 0, idx);
      underlyingFit->function(parameters + pbase, 
                              data, main, &sFnView.vector);
      if(s->sameX[i]) {
        // Reuse the same data -- factor-of-two win...
        DataSet::firstDerivative(main->x().data(), 1, 
                                 sFnView.vector.data, 
                                 sFnView.vector.stride,
                                 derView.vector.data + 
                                 derView.vector.stride * idx, 
                                 derView.vector.stride,
                                 main->x().size());
      }
      else {
        underlyingFit->function(parameters + pbase, 
                                data, sub, &bufView.vector);
        // Now copying back 
        DataSet::firstDerivative(sub->x().data(), 1, 
                                 bufView.vector.data, 
                                 bufView.vector.stride,
                                 derView.vector.data + 
                                 derView.vector.stride * idx, 
                                 derView.vector.stride,
                                 sub->x().size());
      }
      // And scaling...
      gsl_vector_view sDerView = 
        gsl_vector_subvector(&derView.vector, idx, sub->x().size());

      // Do scaling of the derivative !
      gsl_vector_scale(&sDerView.vector, 
                       parameters[pbase + s->originalParameters]);
    }
    else {
      underlyingFit->function(parameters + i * 
                              data->parametersPerDataset(), 
                              data, derDS, &bufView.vector);
      DataSet::firstDerivative(derDS->x().data(), 1, 
                               bufView.vector.data, bufView.vector.stride,
                               derView.vector.data, derView.vector.stride,
                               derDS->x().size());
    }


  }
}

DerivativeFit::DerivativeFit(PerDatasetFit * source, DerivativeFit::Mode m) :
  Fit((QString((m == DerivativeFit::Separated ? "deriv-" : 
                (m == DerivativeFit::DerivativeOnly ? "deriv-only-" : 
                 "deriv-combined-"))) +
       source->fitName(false)).toLocal8Bit(), 
      "Derived fit",
      "(derived fit)",
      (m == DerivativeFit::Separated ? 2: 1) , 
      (m == DerivativeFit::Separated ? 2: -1) , false), 
  underlyingFit(source), mode(m)

{
  // How to remove the "parameters" argument ?
  Command * cmd = Command::namedCommand("fit-" + source->fitName(false));
  ArgumentList * opts = new ArgumentList(*cmd->commandOptions());

  /// @todo Add own options.

  makeCommands(NULL, NULL, NULL, opts);
}

FitInternalStorage * DerivativeFit::allocateStorage(FitData * data) const
{
  Storage * s = new Storage;
  s->originalStorage = underlyingFit->allocateStorage(data);
  return s;
}

FitInternalStorage * DerivativeFit::copyStorage(FitData * data,
                                                FitInternalStorage * source,
                                                int ds) const
{
  Storage * s = static_cast<Storage *>(source);
  Storage * s2 = new Storage(*s);
  
  {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                           s->originalStorage);
    s2->originalStorage = underlyingFit->copyStorage(data, s->originalStorage, ds);
  }
  return s2;
}


//////////////////////////////////////////////////////////////////////
// Now, the command !

static void defineDerivedFit(const QString &, QString fitName, 
                             const CommandOptions & opts)
{
  PerDatasetFit * fit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(fitName));

  if(! fit)
    throw RuntimeError("The fit " + fitName + " isn't working buffer-by-buffer: impossible to make a derived fit");

  
  DerivativeFit::Mode mode = DerivativeFit::Separated;
  if(testOption(opts, "mode", QString("deriv-only")))
    mode = DerivativeFit::DerivativeOnly;
  else if(testOption(opts, "mode", QString("combined")))
    mode = DerivativeFit::Combined;

  ArgumentList * lst = fit->fitArguments();
  if(lst && lst->size())
    throw RuntimeError("Cannot make derivatives of fits that take arguments -- use one of the define-*-fit commands, or custom-fit");
  new DerivativeFit(fit, mode);
}

static ArgumentList 
ddfA(QList<Argument *>() 
      << new FitNameArgument("existing-fit", "Fit",
                             "name of the fit to make a derived fit of"));

static ArgumentList 
ddfO(QList<Argument *>() 
     << new ChoiceArgument(QStringList() 
                           << "deriv-only" 
                           << "separated" 
                           << "combined", 
                           "mode", "Derivative mode",
                           "Whether one fits only the derivative, both the "
                           "derivative and the original data together or "
                           "separated"));


static Command 
ddf("define-derived-fit", // command name
    effector(defineDerivedFit), // action
    "fits",  // group name
    &ddfA, // arguments
    &ddfO, // arguments
    "Create a derived fit",
    "Create a derived fit to fit both the data and its derivative",
    "(...)");
