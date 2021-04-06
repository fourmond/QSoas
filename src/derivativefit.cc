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
#include <commandcontext.hh>
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
  for(DataSet * ds : mainDS.values())
    delete ds;
  for(DataSet * ds : subDS.values())
    delete ds;
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
  underlyingFit = o.underlyingFit;
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


  TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
  Fit::processOptions(s->underlyingFit, opts, data);
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
  return Fit::optionsString(s->underlyingFit, data) + " -- derivative" + n;
}

ArgumentList DerivativeFit::fitHardOptions() const
{
  PerDatasetFit * fit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(underlyingFitName));
  if(fit)
    return Fit::fitHardOptions(fit);
  return ArgumentList();
}

ArgumentList DerivativeFit::fitSoftOptions() const
{
  PerDatasetFit * fit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(underlyingFitName));
  if(fit)
    return Fit::fitSoftOptions(fit);
  return ArgumentList();
}

CommandOptions DerivativeFit::currentSoftOptions(FitData * data) const
{
  Storage * s = storage<Storage>(data);
  TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
  return Fit::currentSoftOptions(s->underlyingFit, data);
}

void DerivativeFit::processSoftOptions(const CommandOptions & opts, FitData * data) const
{
  Storage * s = storage<Storage>(data);
  TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
  Fit::processSoftOptions(s->underlyingFit, opts, data);
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
  QList<ParameterDefinition> params = s->underlyingFit->parameters(data);

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
    s->underlyingFit->initialGuess(data, data->datasets[i], guess + i*tp);
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

  // Prepare the splitted buffers the first time we use them
  if(mode == Combined && !s->sameX) {
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

  int i = 0;
  if(mode == Separated) {
    gsl_vector_view fnView = data->viewForDataset(0, target);
    const DataSet * baseDS = data->datasets[0];

    s->underlyingFit->function(parameters, data,
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
      s->underlyingFit->function(parameters + pbase, 
                              data, main, &sFnView.vector);
      if(s->sameX[i]) {
        // Reuse the same data -- factor-of-two win...
        DataSet::firstDerivative(main->x().data(), 1, 
                                 sFnView.vector.data, 
                                 sFnView.vector.stride,
                                 derView.vector.data + 
                                 derView.vector.stride * idx, 
                                 derView.vector.stride,
                                 main->x().size(), false);
      }
      else {
        s->underlyingFit->function(parameters + pbase, 
                                   data, sub, &bufView.vector);
        // Now copying back 
        DataSet::firstDerivative(sub->x().data(), 1, 
                                 bufView.vector.data, 
                                 bufView.vector.stride,
                                 derView.vector.data + 
                                 derView.vector.stride * idx, 
                                 derView.vector.stride,
                                 sub->x().size(), false);
      }
      // And scaling...
      gsl_vector_view sDerView = 
        gsl_vector_subvector(&derView.vector, idx, sub->x().size());

      // Do scaling of the derivative !
      gsl_vector_scale(&sDerView.vector, 
                       parameters[pbase + s->originalParameters]);
    }
    else {
      s->underlyingFit->function(parameters + i * 
                              data->parametersPerDataset(), 
                              data, derDS, &bufView.vector);
      DataSet::firstDerivative(derDS->x().data(), 1, 
                               bufView.vector.data, bufView.vector.stride,
                               derView.vector.data, derView.vector.stride,
                               derDS->x().size());
    }


  }
}

QString DerivativeFit::derivativeFitName(PerDatasetFit * source,
                                         DerivativeFit::Mode mode)
{
  QString pref;
  switch(mode) {
  case Separated:
    pref = "deriv-";
    break;
  case DerivativeOnly:
    pref = "deriv-only-";
    break;
  default:
    pref = "deriv-combined-";
  }
  return pref + source->fitName(false);
}


DerivativeFit::DerivativeFit(PerDatasetFit * source, DerivativeFit::Mode m) :
  Fit(DerivativeFit::derivativeFitName(source, m).toLocal8Bit(), 
      "Derived fit",
      "(derived fit)",
      (m == DerivativeFit::Separated ? 2: 1) , 
      (m == DerivativeFit::Separated ? 2: -1) , false), 
  mode(m)

{
  underlyingFitName = source->fitName(false);
  // How to remove the "parameters" argument ?
  Command * cmd = CommandContext::globalContext()->
    namedCommand("fit-" + underlyingFitName );

  /// @hack But shouldn't this simply be fitSoftOptions + fitHardOptions();
  ArgumentList opts = *cmd->commandOptions();

  /// @todo Add own options.

  makeCommands(ArgumentList(), NULL, NULL, opts);
}

FitInternalStorage * DerivativeFit::allocateStorage(FitData * data) const
{
  Storage * s = new Storage;
  s->underlyingFit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(underlyingFitName));
  if(! s->underlyingFit)
    throw RuntimeError("The fit " + underlyingFitName + " either does not exist or it isn't working buffer-by-buffer: impossible to make a derived fit");
  
  s->originalStorage = s->underlyingFit->allocateStorage(data);
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
    s2->originalStorage = s->underlyingFit->copyStorage(data, s->originalStorage, ds);
  }
  return s2;
}

//////////////////////////////////////////////////////////////////////

/// A special class for combined fits...
///
/// @todo There may be ways to do code sharing with the other derivative fits ?
class CombinedDerivativeFit : public PerDatasetFit {

  /// Name of the underlying fit.
  QString underlyingFitName;

  /// @name Redirectors
  ///
  /// The functions here just redirect to the wrapped fit
  /// 
  /// @{
  virtual void processOptions(const CommandOptions & opts,
                              FitData * data) const  override {
    DerivativeFit::Storage * s = storage<DerivativeFit::Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
    Fit::processOptions(s->underlyingFit, opts, data);
  };
  
  virtual QString optionsString(FitData * data) const override {
    DerivativeFit::Storage * s = storage<DerivativeFit::Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
    return Fit::optionsString(s->underlyingFit, data) + " -- derivative (combined)";

  };
  
  virtual ArgumentList fitHardOptions() const override {
    PerDatasetFit * fit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(underlyingFitName));
    if(fit)
      return Fit::fitHardOptions(fit);
    return ArgumentList();
  };
  
  virtual ArgumentList fitSoftOptions() const override {
    PerDatasetFit * fit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(underlyingFitName));
    if(fit)
      return Fit::fitSoftOptions(fit);
    return ArgumentList();
  };
  
  virtual CommandOptions currentSoftOptions(FitData * data) const override {
    DerivativeFit::Storage * s = storage<DerivativeFit::Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
    return Fit::currentSoftOptions(s->underlyingFit, data);
  };
  
  virtual void processSoftOptions(const CommandOptions & opts,
                                  FitData * data) const override {
    DerivativeFit::Storage * s = storage<DerivativeFit::Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
    Fit::processSoftOptions(s->underlyingFit, opts, data);
  };
  /// @}


  /// Make sure the buffers are the right size.
  void reserveBuffers(FitData * data) const {
    DerivativeFit::Storage * s = storage<DerivativeFit::Storage>(data);
    for(int i = 0; i < data->datasets.size(); i++) {
      int sz = data->datasets[i]->x().size();
      if(s->buffer.size() < sz)
        s->buffer.resize(sz);
    }
  };

protected:
  
  virtual FitInternalStorage * allocateStorage(FitData * data) const override {
    DerivativeFit::Storage * s = new DerivativeFit::Storage;
    s->underlyingFit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(underlyingFitName));
    if(! s->underlyingFit)
      throw RuntimeError("The fit " + underlyingFitName + " either does not exist or it isn't working buffer-by-buffer: impossible to make a derived fit");
    
    s->originalStorage = s->underlyingFit->allocateStorage(data);
    return s;
  };

  virtual FitInternalStorage * copyStorage(FitData * data,
                                           FitInternalStorage * source,
                                           int ds = -1) const override {
    DerivativeFit::Storage * s = static_cast<DerivativeFit::Storage *>(source);
    DerivativeFit::Storage * s2 = new DerivativeFit::Storage(*s);
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->originalStorage);
      s2->originalStorage = s->underlyingFit->copyStorage(data, s->originalStorage, ds);
    }
    return s2;
  };

public:

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    DerivativeFit::Storage * s = storage<DerivativeFit::Storage>(data);
      
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
    QList<ParameterDefinition> params = s->underlyingFit->parameters(data);

    s->originalParameters = params.size();
    params << ParameterDefinition("deriv_scale", true); 
    return params;
  };
  
  
  virtual void initialGuess(FitData * data, const DataSet * ds, double * guess) const override {
    DerivativeFit::Storage * s = storage<DerivativeFit::Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);
    s->underlyingFit->initialGuess(data, ds, guess);
    guess[s->originalParameters] = 1; // Makes a good default scale !
  };

  void prepareSameX(FitData * data) const {
    DerivativeFit::Storage * s = storage<DerivativeFit::Storage>(data);
    if(s->sameXH.size() > 0)
      return;

    for(int i = 0; i < data->datasets.size(); i++) {
      const DataSet * ds = data->datasets[i];
      PossessiveList<DataSet> sub(ds->splitIntoMonotonic());
      if(sub.size() != 2)
        throw RuntimeError(QString("Dataset '%1' should be made of two "
                                   "monotonic parts !").arg(ds->name));
      sub.detach();
      s->mainDS[ds] = sub[0];
      s->subDS[ds] = sub[1];
      s->sameXH[ds] = sub[0]->x() == sub[1]->x();
    }
  };

  virtual void function(const double * parameters,
                        FitData * data, const DataSet * ds,
                        gsl_vector * target) const override {
    reserveBuffers(data);
    DerivativeFit::Storage * s = storage<DerivativeFit::Storage>(data);

    prepareSameX(data);

    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);

    const DataSet * main = s->mainDS[ds];
    const DataSet * sub = s->subDS[ds];
      
    // We create sub-views of the original view...
    int idx = main->x().size();
    gsl_vector_view sFnView = gsl_vector_subvector(target, 0, idx);
    s->underlyingFit->function(parameters, 
                               data, main, &sFnView.vector);
    if(s->sameXH[ds]) {
      // Reuse the same data -- factor-of-two win...
      DataSet::firstDerivative(main->x().data(), 1, 
                               sFnView.vector.data, 
                               sFnView.vector.stride,
                               target->data + 
                               target->stride * idx, 
                               target->stride,
                               main->x().size());
    }
    else {
      gsl_vector_view bufView = gsl_vector_view_array(s->buffer.data(),
                                                      ds->nbRows());
      s->underlyingFit->function(parameters, 
                                 data, sub, &bufView.vector);
      // Now copying back 
      DataSet::firstDerivative(sub->x().data(), 1, 
                               bufView.vector.data, 
                               bufView.vector.stride,
                               target->data + 
                               target->stride * idx, 
                               target->stride,
                               sub->x().size());
    }
    // And scaling...
    gsl_vector_view sDerView = 
      gsl_vector_subvector(target, idx, sub->x().size());
    
    // Do scaling of the derivative !
    gsl_vector_scale(&sDerView.vector, 
                     parameters[s->originalParameters]);
  };

  virtual bool hasSubFunctions(FitData * data) const override {
    DerivativeFit::Storage * s = storage<DerivativeFit::Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->originalStorage);
    return s->underlyingFit->hasSubFunctions(data);
  };
  virtual bool displaySubFunctions(FitData * data) const override {
    DerivativeFit::Storage * s = storage<DerivativeFit::Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->originalStorage);
    return s->underlyingFit->displaySubFunctions(data);
  }

  virtual void computeSubFunctions(const double * parameters,
                                   FitData * data, 
                                   const DataSet * ds,
                                   QList<Vector> * targetData,
                                   QStringList * targetAnnotations) const override {
    reserveBuffers(data);
    DerivativeFit::Storage * s = storage<DerivativeFit::Storage>(data);

    prepareSameX(data);

    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->originalStorage);

    const DataSet * main = s->mainDS[ds];
    const DataSet * sub = s->subDS[ds];
    
    s->underlyingFit->computeSubFunctions(parameters, data,
                                          main,
                                          targetData,
                                          targetAnnotations);
    if(s->sameXH[ds]) {
      for(Vector & v : *targetData) {
        Vector nv = v;
        DataSet::firstDerivative(main->x().data(), 1, 
                                 v.data(), 1,
                                 nv.data(), 1, v.size());
        nv *= parameters[s->originalParameters];
        v << nv;
      }
    }
    else {
      QList<Vector> subs;
      QStringList dummy;
      s->underlyingFit->computeSubFunctions(parameters, data,
                                            sub,
                                            &subs,
                                            &dummy);
      if(subs.size() != targetData->size())
        throw RuntimeError("Mismatch in the number of subfunctions");
      for(int i = 0; i < targetData->size(); i++) {
        Vector & v = (*targetData)[i];
        Vector nv = subs[i];
        DataSet::firstDerivative(sub->x().data(), 1, 
                                 subs[i].data(), 1,
                                 nv.data(), 1, nv.size());
        nv *= parameters[s->originalParameters];
        v << nv;
      }
    }
  };


  /// Creates (and registers) the derivative fit based on the given
  /// fit
  CombinedDerivativeFit(PerDatasetFit * source) :
      PerDatasetFit(DerivativeFit::derivativeFitName(source, DerivativeFit::Combined).
          toLocal8Bit(), 
          "Derived fit",
          "(derived fit)", 1 , -1, false) {
    underlyingFitName = source->fitName(false);
    // How to remove the "parameters" argument ?
    Command * cmd = CommandContext::globalContext()->
      namedCommand("fit-" + underlyingFitName );
    ArgumentList opts = ArgumentList(*cmd->commandOptions());

    /// @todo Add own options.

    makeCommands(ArgumentList(), NULL, NULL, opts);
  };

};


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

  bool overwrite = false;
  updateFromOptions(opts, "redefine", overwrite);

  ArgumentList lst = fit->fitArguments();
  if(lst.size() > 0)
    throw RuntimeError("Cannot make derivatives of fits that take arguments -- use one of the define-*-fit commands, or custom-fit");

  QString tn = DerivativeFit::derivativeFitName(fit, mode);
  if(Fit::namedFit(tn)) {
    if(! overwrite)
      throw RuntimeError("Attempting to redefine fit '%1', use /redefine=true to ignore").arg(tn);
    return;
  }
  else {
    if(mode == DerivativeFit::Combined)
      new CombinedDerivativeFit(fit);
    else
      new DerivativeFit(fit, mode);
  }
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
                           "separated")
    << new BoolArgument("redefine", 
                        "Redefine",
                        "Does not error out if the fit already exists")
     );


static Command 
ddf("define-derived-fit", // command name
    effector(defineDerivedFit), // action
    "fits",  // group name
    &ddfA, // arguments
    &ddfO, // arguments
    "Create a derived fit",
    "Create a derived fit to fit both the data and its derivative");
