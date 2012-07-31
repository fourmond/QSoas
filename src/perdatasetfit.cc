/*
  perdatasetfit.cc: implementation of the PerDatasetFit class
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
#include <perdatasetfit.hh>
#include <vector.hh>
#include <dataset.hh>
#include <fitdata.hh>


void PerDatasetFit::function(const double * parameters,
                             FitData * data, gsl_vector * target)
{
  int nb_ds_params = data->parametersPerDataset();
  for(int i = 0; i < data->datasets.size(); i++) {
    gsl_vector_view dsView = data->viewForDataset(i, target);
    function(parameters + nb_ds_params * i, data,
             data->datasets[i], &dsView.vector);
  }
}

void PerDatasetFit::functionForDataset(const double * parameters,
                                       FitData * data, gsl_vector * target,
                                       int i)
{
  int nb_ds_params = data->parametersPerDataset();
  gsl_vector_view dsView = data->viewForDataset(i, target);
  function(parameters + nb_ds_params * i, data,
           data->datasets[i], &dsView.vector);
}

void PerDatasetFit::initialGuess(FitData * data, double * guess)
{
  int nb_ds_params = data->parametersPerDataset();
  for(int i = 0; i < data->datasets.size(); i++)
    initialGuess(data, data->datasets[i], 
                 guess + nb_ds_params * i);
}


PerDatasetFit::~PerDatasetFit()
{
}

int PerDatasetFit::parametersCheck(const double * parameters,
                                   FitData * data)
{
  int nb_ds_params = data->parametersPerDataset();
  for(int i = 0; i < data->datasets.size(); i++) {
    int status = parametersCheck(parameters + nb_ds_params * i,
                                 data, data->datasets[i]);
    if(status)
      return status;
  }
  return GSL_SUCCESS;
}

int PerDatasetFit::parametersCheck(const double * parameters,
                                   FitData * data,
                                   const DataSet * ds)
{
  return GSL_SUCCESS;
}


void FunctionFit::function(const double * parameters,
                           FitData * data, 
                           const DataSet * ds,
                           gsl_vector * target)
{
  const Vector & xvalues = ds->x();
  prepare(parameters, data, ds);
  for(int i = 0; i < target->size; i++)
    gsl_vector_set(target, i, function(parameters, data, 
                                       xvalues[i]));
}

void FunctionFit::prepare(const double * /*parameters*/, 
                          FitData * /*data*/, 
                          const DataSet * /*ds*/)
{
}
