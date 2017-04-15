/*
  fitdata.cc: implementation of the FitData helper class
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014 by CNRS/AMU

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
#include <fitparameter.hh>
#include <fitdata.hh>
#include <dataset.hh>

#include <soas.hh>
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>

#include <gsl/gsl_cdf.h>

#include <fitengine.hh>
#include <debug.hh>


// first, the implementation of the queue

DFComputationQueue::DFComputationQueue(FitData * d) :
  terminate(false), runningComputations(0), data(d)
{
}

DFComputationQueue::~DFComputationQueue()
{
}

void DFComputationQueue::enqueue(const DerivativeJob & job)
{
  QMutexLocker l(&mutex);
  queue.enqueue(job);
  condition.wakeAll();
}

void DFComputationQueue::enqueue(int i, const gsl_vector * parameters,
                                 gsl_matrix * target,
                                 const gsl_vector * current)
{
  enqueue(DerivativeJob(i, parameters, target, current));
}

void DFComputationQueue::signalTermination()
{
  QMutexLocker l(&mutex);
  terminate = true;
  condition.wakeAll();
}

DFComputationQueue::DerivativeJob DFComputationQueue::nextJob()
{
  while(true) {
    QMutexLocker l(&mutex);
    // We wake up every half second, to avoid threads just waiting
    // forever. Shouldn't be too long in the end.
    if(data->debug > 0) {
      QMutexLocker l(Debug::debug().mutex());
      Debug::debug() << "Thread " << QThread::currentThread()
                     << " about to wait for a job" << endl;
    }
    condition.wait(&mutex, 500);
    if(queue.size() > 0) {
      if(data->debug > 0) {
        QMutexLocker l(Debug::debug().mutex());
        Debug::debug() << "Thread " << QThread::currentThread()
                       << " got a job" << endl;
      }
      DerivativeJob nxt = queue.dequeue();
      runningComputations++;
      return nxt;
    }
    if(terminate) {
      if(data->debug > 0) {
        QMutexLocker l(Debug::debug().mutex());
        Debug::debug() << "Thread " << QThread::currentThread()
                       << " got signaled for termination" << endl;
      }
      throw TerminateException();
    }
    if(data->debug > 0) {
      QMutexLocker l(Debug::debug().mutex());
      Debug::debug() << "Thread " << QThread::currentThread()
                     << " did not get a job" << endl;
    }
  }
  // This should never happen, but the compiler doesn't know it.
  return DerivativeJob(0, 0, 0, 0);
}

void DFComputationQueue::doneJob()
{
  QMutexLocker l(&mutex);
  runningComputations--;        // Should be positive
  condition.wakeAll();
}

void DFComputationQueue::waitForJobsDone()
{
  while(true) {
    QMutexLocker l(&mutex);
    condition.wait(&mutex, 100);
    int nb = queue.size() + runningComputations;
    if(data->debug > 0) {
      QMutexLocker l(Debug::debug().mutex());
      Debug::debug() << "Waiting for " << nb 
                     << " jobs to finish" << endl;
    }
    if(nb == 0)
      return;
  }
}

int DFComputationQueue::remainingJobs()
{
  QMutexLocker l(&mutex);
  return queue.size() + runningComputations;
}

//////////////////////////////////////////////////////////////////////

class DerivativeComputationThread : public QThread {
protected:
  FitData * data;

  FitInternalStorage * stg;

  QMutex storageMutex;

  int idx;
public:
  DerivativeComputationThread(FitData * d, int id) :
    data(d), stg(NULL), idx(id) {
  };

  void setStorage(FitInternalStorage * local) {
    QMutexLocker l(&storageMutex);
    stg = local;
  }
  
  void run() {
    try {
      while(true) {
        DFComputationQueue::DerivativeJob job = data->workersQueue->nextJob();
        if(stg) {
          QMutexLocker l(&storageMutex);
          data->fitStorage.setLocalData(stg);
          if(data->debug > 0) {
            QMutexLocker l(Debug::debug().mutex());
            Debug::debug() << "Setting up thread " << this
                           << " (#" << idx << ") to work with storage "
                           << stg << endl;
          }
          stg = NULL;
        }
        if(data->debug > 0) {
          QMutexLocker l(Debug::debug().mutex());
          Debug::debug()
            << QString("Thread #%1 derives parameter %2").arg(idx).
            arg(job.idx)
            << " -- current options: " << data->fit->optionsString(data)
            << endl;
        }
        data->deriveParameter(job.idx, job.params, job.target, job.current);
        if(data->debug > 0) {
          QMutexLocker l(Debug::debug().mutex());
          Debug::debug() << QString("Thread #%1 done deriving parameter %2").
            arg(idx).arg(job.idx) << endl;
        }
        data->workersQueue->doneJob();
      }
    }
    catch(const DFComputationQueue::TerminateException) {
    }
  }
};



//////////////////////////////////////////////////////////////////////

FitData::FitData(const Fit * f, const QList<const DataSet *> & ds, int d, 
                 const QStringList & ex) : 
  totalSize(0), covarStorage(NULL), covarIsOK(false),
  engine(NULL), engineOK(false), extra(ex),
  evaluationNumber(0), 
  fit(f), debug(d), datasets(ds),
  standardYErrors(NULL), pointWeights(NULL),
  nbIterations(0), storage(0), parametersStorage(0)
{
  for(int i = 0; i < datasets.size(); i++) {
    totalSize += datasets[i]->nbRows();
    weightsPerBuffer << 1;      // By default 1
  }

  storage = gsl_vector_alloc(totalSize);

  computeWeights();

  workersQueue = NULL;
  fitStorage.setLocalData(f->allocateStorage(this));

  engineFactory = FitEngine::defaultFactoryItem(datasets.size());
}

FitInternalStorage * FitData::getStorage()
{
  return fitStorage.localData();
}

void FitData::setupThreads(int nb)
{
  if((! fit->threadSafe()) || nb == 1)         // Nothing to do !
    return;
  // #ifndef Q_OS_LINUX
  // Debug::debug() << "Threads are disabled on platforms other than Linux as of now" << endl;
  // return;
  // #endif
  if(nb <= 0)
    nb += QThread::idealThreadCount();
  if(nb <= 0)
    return;                     // Still no threads ?

  if(debug > 0)
    Debug::debug() << "Setting up fit data " << this
                   << " for working with " << nb << " threads" << endl;

  workersQueue = new DFComputationQueue(this);
  for(int i = 0; i < nb; i++) {
    workers << new DerivativeComputationThread(this, i+1);
    workers.last()->start();
  }
  
}

void FitData::finishInitialization()
{
  parameterDefinitions = fit->parameters(this);
  for(int i = 0; i < extra.size(); i++)
    parameterDefinitions << ParameterDefinition(extra[i]);
  parametersStorage = gsl_vector_alloc(fullParameterNumber());
}


bool FitData::checkWeightsConsistency() const
{
  int nb = 0;
  for(int i = 0; i < datasets.size(); i++) {
    if(datasets[i]->options.hasYErrors(datasets[i]))
      nb++;
  }
  if(nb < datasets.size() && nb > 0)
    return false;
  return true;
}

void FitData::computeWeights()
{
  // first, we loop through all the datasets to check that all have
  // errors
  int nb = 0;
  for(int i = 0; i < datasets.size(); i++) {
    if(datasets[i]->options.hasYErrors(datasets[i]))
      nb++;
  }
  if(nb < datasets.size()) {
    if(nb > 0) {
      ;                         /// @todo Emit warning...
    }
    // Nothing to do
  }
  else {
    // All datasets have error information
    standardYErrors = gsl_vector_alloc(totalSize);
    pointWeights = gsl_vector_alloc(totalSize);

    for(int i = 0; i < datasets.size(); i++) {
      gsl_vector_view ve = viewForDataset(i, standardYErrors);
      gsl_vector_view vp = viewForDataset(i, pointWeights);

      const DataSet * ds = datasets[i];
      int sz = ds->nbRows();
      for(int j = 0; j < sz; j++) {
        double e = ds->yError(j);
        gsl_vector_set(&ve.vector, j, e);
        gsl_vector_set(&vp.vector, j, 1/(e*e));
      }
    }
  }
}

void FitData::freeSolver()
{
  if(engineOK)
    delete engine;              /// @bug we lose memory upon failed
                                /// engine creation.
  engineOK = false;
  engine = NULL;
  
  for(int i = 0; i < subordinates.size(); i++)
    delete subordinates[i];
  subordinates.clear();
}

FitData::~FitData()
{
  // free up some resources
  gsl_vector_free(storage);
  gsl_vector_free(parametersStorage);
  if(covarStorage)
    gsl_matrix_free(covarStorage);
  if(standardYErrors) {
    gsl_vector_free(standardYErrors);
    gsl_vector_free(pointWeights);
  }
  freeSolver();

  if(workersQueue) {
    workersQueue->signalTermination();
    for(int i = 0; i < workers.size(); i++) {
      // try that ?
      workersQueue->signalTermination();
      DerivativeComputationThread * d = workers[i];
      d->wait();
      d->setStorage(NULL);    
      delete d;
    }
    delete workersQueue;
  }
      
  // This calls apparently deletes the data !
  fitStorage.setLocalData(NULL);

}

void FitData::weightVector(gsl_vector * tg)
{
  for(int i = 0; i < datasets.size(); i++) {
    gsl_vector_view v = viewForDataset(i, tg);
    gsl_vector_scale(&v.vector, weightsPerBuffer[i]);
  }
  if(standardYErrors && engine && !engine->handlesWeights()) {
    /// @todo Would be faster if I multiply ;-)...
    gsl_vector_div(tg, standardYErrors);
  }
}

double FitData::weightedSquareSumForDataset(int ds, const gsl_vector * vect, 
                                            bool subtract)
{
  double ret = 0;
  double w = weightsPerBuffer[ds];
  gsl_vector_view wv;

  const Vector & y = datasets[ds]->y();
  gsl_vector * weight = NULL;
  if(standardYErrors) {
    wv = viewForDataset(ds, standardYErrors);
    weight = &(wv.vector);
  }


  int sz = datasets[ds]->x().size();
  for(int j = 0; j < sz; j++) {
    double v = w;
    if(weight)
      v *= gsl_vector_get(weight, j);
    v = 1/v;
    double val = (vect ? gsl_vector_get(vect, j) : 0);
    if(vect || subtract)
      v *= (subtract ? val - y[j] : val);
    ret += v*v;
  }
  return ret;
}


double FitData::weightedSquareSum(const gsl_vector * src, 
                                  bool subtract)
{
  double ret = 0;
  for(int i = 0; i < datasets.size(); i++) {
    gsl_vector_const_view vv = (src ? 
                                viewForDataset(i, src) :
                                viewForDataset(i, (const gsl_vector*) storage) );
    const gsl_vector * vect = NULL;
    if(src)
      vect = &(vv.vector);
    ret += weightedSquareSumForDataset(i, vect, subtract);
  }
  return ret;
}

bool FitData::usesPointWeights() const
{
  return standardYErrors != NULL;
}

int FitData::computeFunction(const double * params, gsl_vector * f,
                             bool doSubtract, bool doWeight)
{
  int nb = freeParameters();
  QVarLengthArray<double, 1024> dt(nb);
  gsl_vector_view v = gsl_vector_view_array(dt.data(), nb);
  packParameters(params, &v.vector);

  return this->f(&v.vector, f, doSubtract, doWeight);
}

int FitData::f(const gsl_vector * x, gsl_vector * f,
               bool doSubtract, bool doWeights)
{
  QVarLengthArray<double, 1024> params(fullParameterNumber());
  unpackParameters(x, params.data());

  /// @question It may be desirable to add range checking simply here
  /// by checking all parameters.

  if(debug > 0) {
    dumpString("Entering f computation");
    dumpGSLParameters(x);
    dumpFitParameters(params.data());

    dumpString("Reprocessed parameters:");
    QVarLengthArray<double, 1024> p2(gslParameters);
    for(int i = 0; i < gslParameters; i++)
      p2[i] = -100;
    gsl_vector_view v = gsl_vector_view_array(p2.data(), gslParameters);
    packParameters(params.data(), &v.vector);
    dumpGSLParameters(&v.vector);

    QVarLengthArray<double, 1024> p3(fullParameterNumber());
    for(int i = 0; i < fullParameterNumber(); i++)
      p3[i] = -100;
    unpackParameters(&v.vector, p3.data());
    dumpFitParameters(p3.data());
  }

  // First, compute the value in place

  fit->function(params.data(), this, f);
  evaluationNumber++;
  // Then, subtract data.
  if(doSubtract)
    subtractData(f);
  if(doWeights)
    weightVector(f);
  /// @todo Data weighting ?

  if(debug > 0)
    dumpString("Finished f computation");

  return GSL_SUCCESS;
}

void FitData::deriveParameter(int i, const gsl_vector * params,
                              gsl_matrix * target, const gsl_vector * current)
{
  QVarLengthArray<double, 1024> gslParams(gslParameters);
  QVarLengthArray<double, 1024> unpackedParams(fullParameterNumber());

  gsl_vector_view v = gsl_vector_view_array(gslParams.data(), gslParameters);


  gsl_vector_memcpy(&v.vector, params); // reset...
  const QList<FreeParameter *> lst = parametersByDefinition[i];
  if(lst.size() == 0)
    return;                 // Happens: when all parameters are fixed.
    
  int tg = lst[0]->fitIndex;
  gsl_vector_view col = gsl_matrix_column(target, tg);

  // Now, modify all parameters:
  for(int j = 0; j < lst.size(); j++) {
    FreeParameter * param = lst[j];
    double value = gslParams[param->fitIndex];
    double step = (value == 0 ? 1e-6 : param->derivationStep(value));
    if(debug > 0)
      dumpString(QString("Step %1 for param %2 (value: %3)").
                 arg(step).arg(param->fitIndex).arg(value));
    gslParams[param->fitIndex] += step;
  }

  unpackParameters(&v.vector, unpackedParams.data());

  if(debug > 0) {
    QMutexLocker l(Debug::debug().mutex());
    dumpGSLParameters(&v.vector);
    dumpFitParameters(unpackedParams.data());
  }

  fit->function(unpackedParams.data(), this, &col.vector);
  {
    QMutexLocker m(&evaluationsMutex);
    evaluationNumber++;
  }

  if(debug > 1) {
    QMutexLocker l(Debug::debug().mutex());
    dumpString(QString("Independent parameters %1").arg(i));
    dumpString(QString("f:   ") + Utils::vectorString(&col.vector));
  }

  gsl_vector_sub(&col.vector, current);

  if(debug > 1) {
    dumpString(QString("df:  ") + Utils::vectorString(&col.vector));
  }
    
  weightVector(&col.vector);

  if(debug > 1) {
    dumpString(QString("wdf: ") + Utils::vectorString(&col.vector));
  }

  // Splicing the buffer-specific stuff back into place
  for(int j = 1; j < lst.size(); j++) {
    const FreeParameter * p = lst[j];
    gsl_vector_view col2 = gsl_matrix_column(target, p->fitIndex);
    gsl_vector_set_zero(&col2.vector);
    gsl_vector_view c1 = viewForDataset(p->dsIndex, &col.vector);
    gsl_vector_view c2 = viewForDataset(p->dsIndex, &col2.vector);
    gsl_vector_memcpy(&c2.vector, &c1.vector);
    gsl_vector_set_zero(&c1.vector);
  }
    
  for(int j = 0; j < lst.size(); j++) {
    const FreeParameter * p = lst[j];
    double step = gslParams[p->fitIndex] - gsl_vector_get(params, p->fitIndex);
    if(debug > 0)
      dumpString(QString(" -> actual step for parameter %1: %2").
                 arg(p->fitIndex).
                 arg(step));
    col = gsl_matrix_column(target, p->fitIndex);
    gsl_vector_scale(&col.vector, 1/step);
  }
}


int FitData::df(const gsl_vector * x, gsl_matrix * df)
{
  if(x->size != gslParameters)
    throw InternalError("Size mismatch between GSL parameters "
                        "and FitData data");

  QVarLengthArray<double, 1024> unpackedParams(fullParameterNumber());
  unpackParameters(x, unpackedParams.data());

  if(debug > 0)
    dumpString("Entering df computation");

  
  // First, compute the common value, and store it in... the storage
  // place.
  fit->function(unpackedParams.data(), this, storage);
  evaluationNumber++;

  if(workersQueue) {
    for(int i = 0; i < parametersByDefinition.size(); i++)
      workersQueue->enqueue(i, x, df, storage);
    workersQueue->waitForJobsDone();
  }
  else {
    for(int i = 0; i < parametersByDefinition.size(); i++)
      deriveParameter(i, x, df, storage);
  }

  
  if(debug > 0)
    dumpString("Finished df computation");
  if(debug > 1) {
    dumpString("Computed jacobian:");
    dumpString(Utils::matrixString(df));
  }
  return GSL_SUCCESS;
}

int FitData::fdf(const gsl_vector * x, gsl_vector * f, gsl_matrix * df)
{
  this->f(x, f);
  this->df(x, df);
  return GSL_SUCCESS;
}

void FitData::packParameters(const double * unpacked, 
                             gsl_vector * packed) const
{
  int nb_ds_params = parameterDefinitions.size();
  int nb_datasets = datasets.size();
  for(int i = 0; i < parameters.size(); i++)
    parameters[i]->copyToPacked(packed, unpacked, nb_datasets, nb_ds_params);
}

int FitData::namedParameterIndex(const QString & name) const
{
  for(int i = 0; i < parameterDefinitions.size(); i++)
    if(parameterDefinitions[i].name == name)
      return i;
  return -1;
}

/// @todo This function seriously lacks validation
void FitData::unpackParameters(const gsl_vector * packed, 
                               double * unpacked) const
{
  // the number of parameters per dataset
  int nb_ds_params = parameterDefinitions.size();
  int nb_datasets = datasets.size();

  int nb_second_pass = 0;

  for(int i = 0; i < parameters.size(); i++) {
    parameters[i]->copyToUnpacked(unpacked, packed, 
                                 nb_datasets, nb_ds_params);
    if(parameters[i]->needSecondPass())
      nb_second_pass++;
  }


  // We make as many second passes as there are parameters needing it,
  // so that if there is a linear chain of parameters, the whole
  // dependency chain is complete

  /// @todo Smarter interdependency detection, to compute the right
  /// parameters in the right order only once. Only the order in which
  /// the parameters have to be evaluated should be stored

  while(nb_second_pass > 0) {
    for(int i = 0; i < parameters.size(); i++)
      if(parameters[i]->needSecondPass())
        parameters[i]->copyToUnpacked(unpacked, packed, 
                                      nb_datasets, nb_ds_params);
    --nb_second_pass;
  }

}

gsl_vector_view FitData::viewForDataset(int ds, gsl_vector * vect)
{
  int total = 0;
  for(int i = 0; i < ds; i++)
    total += datasets[i]->nbRows();
  return gsl_vector_subvector(vect, total, datasets[ds]->nbRows());
}

gsl_vector_const_view FitData::viewForDataset(int ds, const gsl_vector * vect)
{
  int total = 0;
  for(int i = 0; i < ds; i++)
    total += datasets[i]->nbRows();
  return gsl_vector_const_subvector(vect, total, datasets[ds]->nbRows());
}

void FitData::subtractData(gsl_vector * target)
{
  // Then, subtract the data.
  int nbds = datasets.size();
  int total = 0;
  for(int i = 0; i < nbds; i++) {
    const DataSet * ds = datasets[i];
    gsl_vector_const_view sv = ds->y().vectorView();
    gsl_vector_view tv = gsl_vector_subvector(target, total, ds->nbRows());
    gsl_vector_sub(&tv.vector, &sv.vector);
    total += ds->nbRows();
  }
}


void FitData::initializeParameters()
{
  gslParameters = 0;
  covarIsOK = false;

  // We make sure the parameters are in the right order
  qSort(parameters.begin(), parameters.end(), FitParameter::parameterIsLower);

  parametersByDataset.clear();
  parametersByDefinition.clear();
  for(int i = 0; i < parameterDefinitions.size(); i++)
    parametersByDefinition << QList<FreeParameter *>();

  // Update the free parameters index
  for(int i = 0; i < parameters.size(); i++) {
    FitParameter * param = parameters[i];
    if(! param->fixed()) {
      param->fitIndex = gslParameters++;
      FreeParameter * fp = dynamic_cast<FreeParameter*>(param);
      if(! fp)
        throw InternalError("Free parameter, but not FreeParameter ?");
      // Do cross-linking
      parametersByDataset[param->dsIndex] << fp;
      parametersByDefinition[param->paramIndex] << fp;
    }
    else
      param->fitIndex = -1;     // Should already be the case
    if(param->needsInit())
      param->initialize(this);
  }
}


bool FitData::isFixed(int id, int ds) const
{
  const QList<FreeParameter *> & lst = parametersByDefinition[id];
  for(int i = 0; i < lst.size(); i++)
    if(lst[i]->dsIndex == -1 ||
       lst[i]->dsIndex == ds)
      return false;
  return true;
}


void FitData::initializeSolver(const double * initialGuess,
                               CommandOptions * opts)
{
  nbIterations = 0;
  evaluationNumber = 0;
  freeSolver();

  if(! parametersStorage)
    throw InternalError("Using a FitData for which finishInitialization() was not called");

  initializeParameters();

  if(debug > 0)
    dumpFitParameterStructure();

  if(independentDataSets()) {
    for(int i = 0; i < datasets.size(); i++) {
      if(debug > 0)
        dumpString(QString("Preparing sub-fit %1").arg(i));
      QList<const DataSet * > dss;
      dss << datasets[i];
      FitData * d = new FitData(fit, dss, debug, extra);
      subordinates.append(d);

      for(int j = 0; j < parameters.size(); j++) {
        FitParameter * param = parameters[j];
        if(param->dsIndex == i || (param->fixed() && param->dsIndex == -1)) {
          FitParameter * p2 = param->dup();
          p2->dsIndex = 0;
          d->parameters << p2;
        }
      }
      // delete d->fitStorage.localData();
      d->fitStorage.setLocalData(fit->copyStorage(this, getStorage(), i));

      // Make sure the initialization is finished and done after
      // copying the internal storage
      d->finishInitialization();
      if(workers.size() > 0)
        d->setupThreads(workers.size());

      d->engineFactory = engineFactory;
      d->initializeSolver(initialGuess + 
                          (i * parameterDefinitions.size()), opts);
    }
  }
  else {
    if(! engineFactory)
      engineFactory = FitEngine::defaultFactoryItem();
    engine = engineFactory->creator(this);

    FitInternalStorage * master = getStorage();
    for(int i = 0; i < workers.size(); i++) {
      FitInternalStorage * c = fit->copyStorage(this, master);
      workers[i]->setStorage(c);
    }
    
    if(debug > 0) {
      QTextStream o(stdout);
      o << "Initialized data " << this << " with engine " << engine
        << " (" << engineFactory->name << ")" << endl;
    }
    if(opts)
      engine->setEngineParameters(*opts);
    engine->initialize(initialGuess);
    /// @todo Maybe the handling using engineOK should be transformed
    /// into a try/catch block around the engine creation ?
    engineOK = true;            // Now should be OK
  }
  // And this should be fine.
}

void FitData::unpackCurrentParameters(double * target)
{
  if(subordinates.size() > 0) {
    for(int i = 0; i < subordinates.size(); i++)
      subordinates[i]->
        unpackCurrentParameters(target + i * parameterDefinitions.size());
  }
  else
    unpackParameters(engine->currentParameters(), target);
}

int FitData::iterate()
{
  nbIterations++;
  covarIsOK = false;
  if(debug > 0) {
    dumpString(QString("Fit iteration: #%1").arg(nbIterations));
    dumpFitParameterStructure();
  }
  if(subordinates.size() > 0) {
    int nbGoingOn = 0;
    for(int i = 0; i < subordinates.size(); i++) {

      if(subordinates[i]->nbIterations >= 0) {
        if(debug > 0)
          dumpString(QString("Passing to subordinate %1").arg(i));
        int status = subordinates[i]->iterate();
        if(debug > 0)
          dumpString(QString(" -> subordinate %1 return code %2").
                     arg(i).arg(status));
        if(status != GSL_CONTINUE) 
          subordinates[i]->nbIterations = -1; // Special sign to
                                              // finish iterations
        else
          nbGoingOn++;
      }
      else
        if(debug > 0)
          dumpString(QString("skipping subordinate %1 (finished)").arg(i));

    }
    if(nbGoingOn)
      return GSL_CONTINUE;
    else
      return GSL_SUCCESS;
  }
  else {
    int status = engine->iterate();
    if(debug > 0)
      dumpString(QString("engine 0x%1 returned status %2").
                 arg((long)engine,0,16).arg(status));
    return status;
  }
}

double FitData::residuals()
{
  if(subordinates.size() > 0) {
    double res = 0;
    for(int i = 0; i < subordinates.size(); i++)
      res += subordinates[i]->residuals();
    return res;
  }
  if(! engine)
    return 0.0/0.0;
  return engine->residuals();
}


bool FitData::independentDataSets() const
{
  if(datasets.size() <= 1)
    return false;               // For sure, we don't want
                                // subordinates in that case...

  for(int i = 0; i < parameters.size(); i++) {
    if(! parameters[i]->fixed() && parameters[i]->dsIndex < 0)
      return false;
  }
  return true;
}

void FitData::recomputeJacobian()
{
  covarIsOK = false;
  if(subordinates.size() > 0) {
    for(int i = 0; i < subordinates.size(); i++)
      subordinates[i]->recomputeJacobian();
  }
  else {
    if(debug > 0) {
      Debug::debug()
        << "Recomputing errors in data " <<  this
        << " with engine: " <<  engine << endl;
    }
    if(! engine)
      throw InternalError("Must have an engine for computing the errors");
    engine->recomputeJacobian();
  }
}

const gsl_matrix * FitData::covarianceMatrix()
{
  if(covarIsOK)
    return covarStorage;
  /// @todo provide a function with a gsl_matrix target as argument.
  if(! covarStorage) {
    int sz = fullParameterNumber();
    if(sz == 0)
      sz = 1;                   /// @hack Work around GSL failing on
                                /// empty matrix
    covarStorage = gsl_matrix_alloc(sz, sz);
  }

  gsl_matrix_set_zero(covarStorage);

  if(subordinates.size() > 0) {
    for(int i = 0; i < subordinates.size(); i++) {
      const gsl_matrix * sub = subordinates[i]->covarianceMatrix();
      int nb = parameterDefinitions.size();
      gsl_matrix_view m = gsl_matrix_submatrix(covarStorage, nb*i, nb*i, 
                                               nb, nb);
      gsl_matrix_memcpy(&m.matrix, sub);
    }
  }
  else {

    // We write the covariance matrix in the top-left corner of the
    // storage space, and then move all the columns in place using
    // permutations.

    int nbparams = (gslParameters > 0 ? gslParameters : 1); 
    /// @hack Work around the GSL limitation on having empty matrices
    gsl_matrix_view m = gsl_matrix_submatrix(covarStorage, 0, 0, 
                                             nbparams, nbparams);
    if(engine && engineOK) {
      engine->computeCovarianceMatrix(&m.matrix);

      // Now, we perform permutations to place all the elements where they
      // should be.
      //
      // We start from the top.
      for(int i = parameters.size() - 1; i >= 0; i--) {
        FitParameter * param = parameters[i];

        if(param->fitIndex < 0)
          continue;

        /// @warning This function assumes a certain layout in the fit
        /// parameters part of the fit vector, but as they are all free
        /// parameters, things should be fine ?
        int target = param->paramIndex + 
          parameterDefinitions.size() * ( param->dsIndex >= 0 ? 
                                          param->dsIndex : 0);
        gsl_matrix_swap_rows(covarStorage, param->fitIndex, target);
        gsl_matrix_swap_columns(covarStorage, param->fitIndex, target);

        /// @todo add scaling to columns when applicable. (bijections)
      }

      // Scaling factor coming from the gsl documentation
      double res = residuals();
      /// @bug This is not accurate for weighted fits !!!!
      gsl_matrix_scale(covarStorage, res*res/doF());
    } 
    else
      /// @todo Maybe this should be signaled in a different way --
      /// using an exception of some kind ?
      gsl_matrix_set_zero(&m.matrix); // in the case when no engine
                                      // has been setup yet.
  }
  
  covarIsOK = true;
  return covarStorage;
}

int FitData::doF() const
{
  return totalSize - gslParameters;
}

double FitData::confidenceLimitFactor(double conf) const
{
  return gsl_cdf_tdist_Pinv(conf, doF());
}


// Debug-related functions
void FitData::dumpString(const QString & str) const
{
  QMutexLocker l(Debug::debug().mutex());
  Debug::debug() << str << endl;
}

void FitData::dumpGSLParameters(const gsl_vector * params) const
{
  QString s("GSL:");
  for(int i = 0; i < gslParameters; i++)
    s += QString("\t%1").arg(gsl_vector_get(params, i));
  dumpString(s);
}

void FitData::dumpFitParameters(const double * params) const
{
  // WW at 4 params ?
  QString s("Fit:");
  for(int i = 0; i < parametersPerDataset() * datasets.size(); i++) {
    if((i+1) % 5 == 0)
      s += "\n";
    QString name = QString("%1[#%2]").
      arg(parameterDefinitions[i % parameterDefinitions.size()].name).
      arg(i / parameterDefinitions.size());
    s += QString("\t%1: %2").arg(name, -13).arg(params[i],-6);
  }
  dumpString(s);
}

void FitData::dumpFitParameterStructure() const
{
  QString s("Parameters:\n");
  for(int i = 0; i < parameters.size(); i++) {
    s += QString(" * #%1 %2[%3]:\t%4\t%5\n").
      arg(parameters[i]->paramIndex).
      arg(parameterDefinitions[parameters[i]->paramIndex].name).
      arg(parameters[i]->dsIndex).
      arg(parameters[i]->fitIndex).
      arg(parameters[i]->fixed() ? "fixed" : "FREE");
  }
  dumpString(s);
}

