/*
  fitdata.cc: implementation of the FitData helper class
  Copyright 2011 by Vincent Fourmond

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

#include <fitdialog.hh>
#include <fitengine.hh>


FitData::FitData(Fit * f, const QList<const DataSet *> & ds, bool d, 
                 const QStringList & ex) : 
  totalSize(0), covarStorage(NULL), engine(NULL), extra(ex),
  evaluationNumber(0), 
  fit(f), debug(d), datasets(ds),
  standardYErrors(NULL), pointWeights(NULL),
  parameterDefinitions(f->parameters()), nbIterations(0), storage(0)
{
  for(int i = 0; i < extra.size(); i++)
    parameterDefinitions << ParameterDefinition(extra[i]);
  for(int i = 0; i < datasets.size(); i++) {
    totalSize += datasets[i]->nbRows();
    weightsPerBuffer << 1;      // By default 1
  }

  storage = gsl_vector_alloc(totalSize);
  parametersStorage = gsl_vector_alloc(fullParameterNumber());

  computeWeights();
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
    if(nb > 0)
      ;                         /// @todo Emit warning...
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
  delete engine;
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

bool FitData::usesPointWeights() const
{
  return standardYErrors != NULL;
}

int FitData::f(const gsl_vector * x, gsl_vector * f, bool doSubtract)
{
  QVarLengthArray<double, 1024> params(fullParameterNumber());
  unpackParameters(x, params.data());

  /// @todo It may be possible to add range checking simply here by
  /// checking all parameters.

  if(debug) {
    dumpString("Entering f computation");
    dumpGSLParameters(x);
    dumpFitParameters(params.data());
  }

  // First, compute the value in place

  fit->function(params.data(), this, f);
  evaluationNumber++;
  // Then, subtract data.
  if(doSubtract)
    subtractData(f);
  weightVector(f);
  /// @todo Data weighting ?

  if(debug)
    dumpString("Entering f computation");

  return GSL_SUCCESS;
}


int FitData::df(const gsl_vector * x, gsl_matrix * df)
{
  if(x->size != gslParameters)
    throw InternalError("Size mismatch between GSL parameters "
                        "and FitData data");

  QVarLengthArray<double, 1024> gslParams(gslParameters);
  QVarLengthArray<double, 1024> unpackedParams(fullParameterNumber());

  gsl_vector_view v = gsl_vector_view_array(gslParams.data(), gslParameters);


  gsl_vector_memcpy(&v.vector, x);
  unpackParameters(&v.vector, unpackedParams.data());

  if(debug)
    dumpString("Entering df computation");

  
  // First, compute the common value, and store it in... the storage
  // place.
  fit->function(unpackedParams.data(), this, storage);
  evaluationNumber++;

  /// @todo This would greatly benefit from using multiple threads, if
  /// possible. The downside is of course that it would be a real pain
  /// to program ;-)... It would require fits to enable a flag saying
  /// their function is thread-safe (which is almost always the case)
  for(int i = 0; i < parameters.size(); i++) {
    FreeParameter * param = dynamic_cast<FreeParameter*>(parameters[i]);
    if(! param)
      continue;
    double value = gslParams[param->fitIndex];

    /// @todo It is crucial to come up with a much better way to
    /// handle the choice of the step. Look at
    /// http://en.wikipedia.org/wiki/Numerical_differentiation
    double step = param->derivationStep(value);


    gslParams[param->fitIndex] += step;
    unpackParameters(&v.vector, unpackedParams.data());

    if(debug) {
      dumpGSLParameters(&v.vector);
      dumpFitParameters(unpackedParams.data());
    }
    
    gsl_vector_view col = gsl_matrix_column(df, param->fitIndex);

    /// @todo turn back on the optimization with functionForDataSet if
    /// I find the time and motivation (though improvement isn't that
    /// great)

    try {
      fit->function(unpackedParams.data(), this, &col.vector);
      evaluationNumber++;
    }
    catch (const RuntimeError & e) {
      // If forward step fails, we try backwards...
      step = -step;
      gslParams[param->fitIndex] = value + step;
      unpackParameters(&v.vector, unpackedParams.data());

      if(debug) {
        dumpGSLParameters(&v.vector);
        dumpFitParameters(unpackedParams.data());
      }
      fit->function(unpackedParams.data(), this, &col.vector);
      evaluationNumber++;
    }

    gsl_vector_sub(&col.vector, storage);
    // Improve precision again:
    step = gslParams[param->fitIndex] - value;
    gsl_vector_scale(&col.vector, 1/step);
    weightVector(&col.vector);
    gslParams[param->fitIndex] = value;
  }
  if(debug)
    dumpString("Finished df computation");
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


  for(int i = 0; i < parameters.size(); i++)
    parameters[i]->copyToUnpacked(unpacked, packed, 
                                 nb_datasets, nb_ds_params);

  /// @todo Make that much more subtle to handle interdependence ?
  /// Lets say that for now, interdependence isn't an option !
  for(int i = 0; i < parameters.size(); i++)
    if(parameters[i]->needSecondPass())
      parameters[i]->copyToUnpacked(unpacked, packed, 
                                    nb_datasets, nb_ds_params);

}

gsl_vector_view FitData::viewForDataset(int ds, gsl_vector * vect)
{
  int total = 0;
  for(int i = 0; i < ds; i++)
    total += datasets[i]->nbRows();
  return gsl_vector_subvector(vect, total, datasets[ds]->nbRows());
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

  // Update the free parameters index
  for(int i = 0; i < parameters.size(); i++) {
    FitParameter * param = parameters[i];
    if(! param->fixed())
      param->fitIndex = gslParameters++;
    else
      param->fitIndex = -1;     // Should already be the case
    if(param->needsInit())
      param->initialize(this);
  }
}


void FitData::initializeSolver(const double * initialGuess, 
                               FitEngineFactoryItem * feit)
{
  nbIterations = 0;
  evaluationNumber = 0;
  freeSolver();

  initializeParameters();

  if(independentDataSets()) {
    for(int i = 0; i < datasets.size(); i++) {
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

      d->initializeSolver(initialGuess + 
                          (i * parameterDefinitions.size()));
    }
  }
  else {
    if(! feit)
      feit = FitEngine::defaultFactoryItem();
    engine = feit->creator(this);
    engine->initialize(initialGuess);
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
  if(subordinates.size() > 0) {
    int nbGoingOn = 0;
    for(int i = 0; i < subordinates.size(); i++) {
      if(subordinates[i]->nbIterations >= 0) {
        int status = subordinates[i]->iterate();
        if(status != GSL_CONTINUE) 
          subordinates[i]->nbIterations = -1; // Special sign to
                                              // finish iterations
          else
            nbGoingOn++;
      }
    }
    if(nbGoingOn)
      return GSL_CONTINUE;
    else
      return GSL_SUCCESS;
  }
  else
    return engine->iterate();
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

double FitData::relativeResiduals()
{
  double res = residuals();
  double norm = 0;
  for(int i = 0; i < datasets.size(); i++)
    norm += datasets[i]->y().norm() * weightsPerBuffer[i];
  return res/norm;
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

const gsl_matrix * FitData::covarianceMatrix()
{
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
    if(engine) {
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
      gsl_matrix_scale(covarStorage, res*res/doF());
    } 
    else
      /// @todo Maybe this should be signaled in a different way --
      /// using an exception of some kind ?
      gsl_matrix_set_zero(&m.matrix); // in the case when no engine
                                      // has been setup yet.
  }
  
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
  QTextStream o(stdout);
  o << str << endl;
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
  QString s;
  for(int i = 0; i < parameterDefinitions.size() * datasets.size(); i++) {
    if((i+1) % 5 == 0)
      s += "\n";
    QString name = QString("%1[#%2]").
      arg(parameterDefinitions[i % parameterDefinitions.size()].name).
      arg(i / parameterDefinitions.size());
    s += QString("\t%1: %2").arg(name).arg(params[i]);
  }
  dumpString(s);
}

