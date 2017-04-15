/*
  sparsejacobian.cc: implementation of the SparseJacobian class
  Copyright 2017 by CNRS/AMU

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
#include <sparsejacobian.hh>
#include <exceptions.hh>

#include <fitdata.hh>
#include <fitparameter.hh>

SparseJacobian::SparseJacobian(const FitData * data,  bool sp) :
  fitData(data), sparse(sp)
{
  // total size
  int sz = data->dataPoints();

  parameters = fitData->parametersByDefinition;

  effectiveParameters = 0;
  for(int i = 0; i < parameters.size(); i++) {
    if(parameters[i].size() > 0) {
      matrixIndex[i] = effectiveParameters;
      effectiveParameters++;
    }
    else
      matrixIndex[i] = -1;      // not found
  }

  if(sparse)
    matrix = gsl_matrix_alloc(sz, effectiveParameters);
  else {
    matrix = gsl_matrix_alloc(sz, fitData->freeParameters());
    gsl_matrix_set_zero(matrix);    // We need to initialize to 0
  }
}

SparseJacobian::~SparseJacobian()
{
  gsl_matrix_free(matrix);
}

gsl_vector * SparseJacobian::parameterVector(int index)
{
  int idx = matrixIndex.value(index, -1);
  if(idx >= 0) {
    v1 = gsl_matrix_column(matrix, idx);
    return &v1.vector;
  }
  return NULL;
}

gsl_vector * SparseJacobian::parameterVector(int index, int dataset)
{
  gsl_vector * v = parameterVector(index);
  if(dataset < 0)
    return v;
  if(v) {
    v2 = fitData->viewForDataset(dataset, v);
    return &v2.vector;
  }
  return NULL;
}

void SparseJacobian::spliceParameter(int index)
{
  if(sparse)
    return;
  int idx = matrixIndex.value(index, -1);
  if(idx < 0)
    return;
  gsl_vector_view source_1 = gsl_matrix_column(matrix, idx);
  const QList<FreeParameter *> pms = parameters[index];
  for(int i = 0; i < pms.size(); i++) {
    int id2 = pms[i]->fitIndex;
    int dsi = pms[i]->dsIndex;
    gsl_vector_view dest_1 = gsl_matrix_column(matrix, id2);
    gsl_vector_view source_2 = fitData->viewForDataset(dsi, &source_1.vector);
    gsl_vector_view dest_2 = fitData->viewForDataset(dsi, &dest_1.vector);
    gsl_vector_memcpy(&dest_2.vector, &source_2.vector);
    /// Clear the source
    gsl_vector_set_zero(&source_2.vector);
  }
}
