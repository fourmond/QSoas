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
#include <fitdata.hh>
#include <dataset.hh>

#include <soas.hh>
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>

#include <fitdialog.hh>

#include <bijection.hh>

#include <ruby.hh>

void FitParameter::initialize(FitData * /*data*/)
{
  
}

void FitParameter::copyToPacked(gsl_vector * /*fit*/, 
                                const double * /*unpacked*/,
                                int /*nbdatasets*/, 
                                int /*nb_per_dataset*/) const
{
  // Nothing to do !
}


QString FitParameter::saveExtraInfo() const
{
  return QString();
}

void FitParameter::loadExtraInfo(const QString & /*str*/)
{
  // Nothing to do by default
}

QString FitParameter::saveAsString(double value) const
{
  QString str = QString("%1\t!\t%2").
    arg(textValue(value)).arg(fixed() ? "0" : "1");
  QString extra = saveExtraInfo();
  if(! extra.isEmpty()) 
    str += "\t!\t" + extra;
  return str;
}

FitParameter * FitParameter::loadFromString(const QString & str, 
                                            double * target, 
                                            int paramIndex, int dsIndex)
{
  QStringList lst = str.split("\t!\t");
  bool fixed = (lst[1] == "0");

  FitParameter * p;
  
  if(fixed) {
    FixedParameter * pm = new FixedParameter(paramIndex, dsIndex, 0);
    pm->setValue(target, lst[0]);
    pm->value = *target;        /// @todo This should move to setValue
    p = pm;
  }
  else {
    FreeParameter * pm = new FreeParameter(paramIndex, dsIndex);
    pm->setValue(target, lst[0]);
    p =  pm;
  }
  p->loadExtraInfo(lst.value(2, QString()));
  return p;
}

FitParameter::~FitParameter()
{
}

//////////////////////////////////////////////////////////////////////



void FreeParameter::copyToUnpacked(double * target, const gsl_vector * fit, 
                                  int nb_datasets, int nb_per_dataset) const
{
  /// @todo check that fitIndex is positive ??
  double value = gsl_vector_get(fit, fitIndex);

  if(bijection)
    value = bijection->forward(value);
  if(dsIndex >= 0)
    target[paramIndex + dsIndex * nb_per_dataset] = value;
  else
    for(int j = 0; j < nb_datasets; j++)
      target[paramIndex + j * nb_per_dataset] = value;
}

void FreeParameter::copyToPacked(gsl_vector * target, const double * unpacked,
                                 int /*nbdatasets*/, int nb_per_dataset) const
{
  double value = unpacked[paramIndex + (dsIndex < 0 ? 0 : dsIndex) * 
                          nb_per_dataset];
  if(bijection)
    value = bijection->backward(value);

  gsl_vector_set(target, fitIndex, value);
}

QString FreeParameter::saveExtraInfo() const
{
  if(bijection)
    return bijection->saveAsText();
  return QString();
}

void FreeParameter::loadExtraInfo(const QString & str)
{
  delete bijection;
  bijection = Bijection::loadFromText(str); // Can return NULL, that
                                            // isn't a problem.
}

FreeParameter::~FreeParameter()
{
  delete bijection;             // This is where the fun starts...
}

FitParameter * FreeParameter::dup() const
{
  FreeParameter *p = new FreeParameter(*this);
  if(bijection)
    p->bijection = bijection->dup();
  return p;
}

//////////////////////////////////////////////////////////////////////

void FixedParameter::copyToUnpacked(double * target, const gsl_vector * fit, 
                                   int nb_datasets, int nb_per_dataset) const
{
  /// @todo Formula-based stuff
  if(dsIndex >= 0)
    target[paramIndex + dsIndex * nb_per_dataset] = value;
  else
    for(int j = 0; j < nb_datasets; j++)
      target[paramIndex + j * nb_per_dataset] = value;
}

void FixedParameter::copyToPacked(gsl_vector * target, const double * unpacked,
                                  int /*nbdatasets*/, int nb_per_dataset) const
{
  // We retrieve the value from the unpacked parameters
  value = unpacked[paramIndex + (dsIndex < 0 ? 0 : dsIndex) * 
                   nb_per_dataset];
}

void FixedParameter::initialize(FitData * data)
{
  if(formula.isEmpty())
    return;

  makeBlock();

  depsIndex.clear();
  for(int j = 0; j < dependencies.size(); j++) {
    int idx = data->namedParameterIndex(dependencies[j]);
    if(idx < 0)
      throw RuntimeError(QString("In definition of parameter %1: "
                                 "'%2' isn't a parameter name !").
                         arg(data->parameterDefinitions[paramIndex].name).
                         arg(dependencies[j]));
    depsIndex << idx;
  }
}




void FixedParameter::makeBlock()
{
  block = Ruby::run<QStringList *, 
                    const QByteArray &>(&Ruby::makeBlock, 
                                        &dependencies, formula.toLocal8Bit());
}

double FixedParameter::compute(const double * unpacked) const
{
  int np = depsIndex.size();
  VALUE args[np];
  for(int i = 0; i < np; i++)
    args[i] = rb_float_new(unpacked[depsIndex[i]]);
  return NUM2DBL(Ruby::run(&rb_funcall2, block, 
                           rb_intern("call"), np, 
                           (const VALUE *) args));

}


//////////////////////////////////////////////////////////////////////


FitData::FitData(Fit * f, const QList<const DataSet *> & ds) : 
  totalSize(0), covarStorage(NULL),
  fit(f), datasets(ds),
  parameterDefinitions(f->parameters()),
  solver(0), nbIterations(0), storage(0)
  
{
  for(int i = 0; i < datasets.size(); i++) 
    totalSize += datasets[i]->nbRows();

  storage = gsl_vector_alloc(totalSize);
  parametersStorage = gsl_vector_alloc(fullParameterNumber());
}

void FitData::freeSolver()
{
  if(solver)
    gsl_multifit_fdfsolver_free(solver);
  solver = NULL;
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
  freeSolver();
}

int FitData::staticF(const gsl_vector * x, void * params, gsl_vector * f)
{
  FitData * data = reinterpret_cast<FitData *>(params);
  return data->f(x, f);
}

int FitData::staticDf(const gsl_vector * x, void * params, gsl_matrix * df)
{
  FitData * data = reinterpret_cast<FitData *>(params);
  return data->df(x, df);
}

int FitData::staticFdf(const gsl_vector * x, void * params, gsl_vector * f,
                       gsl_matrix * df)
{
  FitData * data = reinterpret_cast<FitData *>(params);
  return data->fdf(x, f, df);
}

int FitData::f(const gsl_vector * x, gsl_vector * f)
{
  QVarLengthArray<double, 1024> params(fullParameterNumber());
  unpackParameters(x, params.data());

  // First, compute the value in place
  fit->function(params.data(), this, f);
  // Then, subtract data.
  subtractData(f);
  /// @todo Data weighting ?

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

  
  // First, compute the common value, and store it in... the storage
  // place.
  fit->function(unpackedParams.data(), this, storage);

  /// @todo This would greatly benefit from using multiple threads, if
  /// possible. The downside is of course that it would be a real pain
  /// to program ;-)... It would require fits to enable a flag saying
  /// their function is thread-safe (which is almost always the case)
  for(int i = 0; i < parameters.size(); i++) {
    FreeParameter * param = dynamic_cast<FreeParameter*>(parameters[i]);
    if(! param)
      continue;
    double value = gslParams[param->fitIndex];
    double step = param->derivationFactor * value;
    if(fabs(step) < param->minDerivationStep)
      step = param->minDerivationStep;

    gslParams[param->fitIndex] += step;
    unpackParameters(&v.vector, unpackedParams.data());
    
    gsl_vector_view col = gsl_matrix_column(df, param->fitIndex);

    /// @todo turn back on the optimization with functionForDataSet if
    /// I find the time and motivation (though improvement isn't that
    /// great)
    fit->function(unpackedParams.data(), this, &col.vector);

    gsl_vector_sub(&col.vector, storage);
    gsl_vector_scale(&col.vector, 1/step);
    gslParams[param->fitIndex] = value;
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

  for(int i = 0; i < parameters.size(); i++)
    parameters[i]->copyToUnpacked(unpacked, packed, 
                                 nb_datasets, nb_ds_params);

  /// @todo Second pass if required !
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

void FitData::initializeSolver(const double * initialGuess)
{
  nbIterations = 0;
  freeSolver();
  gslParameters = 0;

  // Update the free parameters index
  for(int i = 0; i < parameters.size(); i++) {
    FitParameter * param = parameters[i];
    if(! param->fixed())
      param->fitIndex = gslParameters++;
    else
      param->fitIndex = -1;     // Should already be the case
    param->initialize(this);
  }

  if(independentDataSets()) {
    for(int i = 0; i < datasets.size(); i++) {
      QList<const DataSet * > dss;
      dss << datasets[i];
      FitData * d = new FitData(fit, dss);
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
  
    solver = gsl_multifit_fdfsolver_alloc(gsl_multifit_fdfsolver_lmsder,
                                          totalSize, gslParameters);
    function.f = &FitData::staticF;
    function.df = &FitData::staticDf;
    function.fdf = &FitData::staticFdf;
    function.n = totalSize;
    function.p = gslParameters;
    function.params = this;

    gsl_vector_view v = gsl_vector_subvector(parametersStorage, 0, 
                                             function.p);
    packParameters(initialGuess, &v.vector);
    gsl_multifit_fdfsolver_set(solver, &function, &v.vector);
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
    unpackParameters(solver->x, target);
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
  else {
    int status = gsl_multifit_fdfsolver_iterate(solver);
    if(status)
      return status;

    QVarLengthArray<double, 1024> params(fullParameterNumber());
    unpackParameters(solver->x, params.data());
    status = fit->parametersCheck(params.data(), this);
    if(status)
      return status;
    
    /// @todo customize this !
    return gsl_multifit_test_delta(solver->dx, solver->x,
                                   1e-4, 1e-4);
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
  if(! solver)
    return 0.0/0.0;
  return gsl_blas_dnrm2(solver->f);
}

double FitData::relativeResiduals()
{
  double res = residuals();
  double norm = 0;
  for(int i = 0; i < datasets.size(); i++)
    norm += datasets[i]->y().norm();
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
  if(! covarStorage)
    covarStorage = gsl_matrix_alloc(fullParameterNumber(),
                                    fullParameterNumber());

  gsl_matrix_set_zero(covarStorage);

  // We write the covariance matrix in the top-left corner of the
  // storage space, and then move all the columns in place using
  // permutations.
  gsl_matrix_view m = gsl_matrix_submatrix(covarStorage, 0, 0, 
                                           gslParameters, gslParameters);
  gsl_multifit_covar(solver->J, 0, &m.matrix);

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
      parameterDefinitions.size() * ( param->dsIndex >= 0 ? param->dsIndex : 0);
    gsl_matrix_swap_rows(covarStorage, param->fitIndex, target);
    gsl_matrix_swap_columns(covarStorage, param->fitIndex, target);

    /// @todo add scaling to columns when applicable. (bijections)
  }

  // Scaling factor coming from the gsl documentation
  double res = residuals();
  gsl_matrix_scale(covarStorage, res*res/(totalSize - gslParameters));

  
  return covarStorage;
}
