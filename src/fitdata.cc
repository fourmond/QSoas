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

FitData::FitData(Fit * f, const QList<const DataSet *> & ds) : 
  totalSize(0),
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
}

FitData::~FitData()
{
  // free up some resources
  gsl_vector_free(storage);
  gsl_vector_free(parametersStorage);
  freeSolver();
}

int FitData::staticF(const gsl_vector * x, void * params, gsl_vector * f)
{
  FitData * data = reinterpret_cast<FitData *>(params);
  return data->fit->f(x, data, f);
}

int FitData::staticDf(const gsl_vector * x, void * params, gsl_matrix * df)
{
  FitData * data = reinterpret_cast<FitData *>(params);
  return data->fit->df(x, data, df);
}

int FitData::staticFdf(const gsl_vector * x, void * params, gsl_vector * f,
                       gsl_matrix * df)
{
  FitData * data = reinterpret_cast<FitData *>(params);
  return data->fit->fdf(x, data, f, df);
}

int FitData::packedToUnpackedIndex(int i) const
{
  // the number of parameters per dataset
  int nb_ds_params = parameterDefinitions.size();
  const ActualParameter & param = parameters[i];
  int idx = (param.dsIndex < 0 ? 
             param.paramIndex : 
             param.paramIndex + nb_ds_params * param.dsIndex);
  return idx;
}

void FitData::packParameters(const double * unpacked, 
                             gsl_vector * packed) const
{
  for(int i = 0; i < parameters.size(); i++)
    gsl_vector_set(packed, i, unpacked[packedToUnpackedIndex(i)]);
}

/// @todo This function seriously lacks validation
void FitData::unpackParameters(const gsl_vector * packed, 
                               double * unpacked) const
{
  // the number of parameters per dataset
  int nb_ds_params = parameterDefinitions.size();
  int nb_datasets = datasets.size();

  for(int i = 0; i < parameters.size(); i++) {
    const ActualParameter & param = parameters[i];
    double value = gsl_vector_get(packed, i);
    if(param.dsIndex >= 0)
      unpacked[param.paramIndex + param.dsIndex * nb_ds_params] = value;
    else {
      for(int j = 0; j < nb_datasets; j++)
        unpacked[param.paramIndex + j * nb_ds_params] = value;
    }
  }

  // Now, fixed parameters.
  for(int i = 0; i < fixedParameters.size(); i++) {
    const FixedParameter & param = fixedParameters[i];
    if(param.dsIndex >= 0)
      unpacked[param.paramIndex + param.dsIndex * nb_ds_params] = param.value;
    else {
      for(int j = 0; j < nb_datasets; j++)
        unpacked[param.paramIndex + j * nb_ds_params] = param.value;
    }
  }
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
  solver = gsl_multifit_fdfsolver_alloc(gsl_multifit_fdfsolver_lmsder,
                                        totalSize, parameters.size());
  function.f = &FitData::staticF;
  function.df = &FitData::staticDf;
  function.fdf = &FitData::staticFdf;
  function.n = totalSize;
  function.p = parameters.size();
  function.params = this;

  gsl_vector_view v = gsl_vector_subvector(parametersStorage, 0, 
                                           function.p);
  packParameters(initialGuess, &v.vector);
  gsl_multifit_fdfsolver_set(solver, &function, &v.vector);

  // And this should be fine.
}

void FitData::unpackCurrentParameters(double * target)
{
  unpackParameters(solver->x, target);
}

int FitData::iterate()
{
  nbIterations++;
  int status = gsl_multifit_fdfsolver_iterate(solver);
  if(status)
    return status;

  /// @todo customize this !
  return gsl_multifit_test_delta(solver->dx, solver->x,
                                 1e-4, 1e-4);
}

double FitData::residuals()
{
  return gsl_blas_dnrm2(solver->f);
}
