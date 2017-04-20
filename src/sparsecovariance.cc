/*
  sparsecovariance.cc: implementation of the SparseCovariance class
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
#include <sparsecovariance.hh>
#include <exceptions.hh>

#include <fitdata.hh>
#include <fitparameter.hh>

#include <abdmatrix.hh>

SparseCovariance::SparseCovariance(const FitData * data,  bool sp) :
  fitData(data), matrix(NULL), sparseMatrix(NULL)
{
  int datasets = fitData->datasets.size();

  const QList<QList<FreeParameter *> > & parameters =
    fitData->parametersByDefinition;
  
  fitIndices = QVector<int>(parameters.size() * datasets, -1);
  for(int i = 0; i < parameters.size(); i++) {
    const QList<FreeParameter *> lst = parameters[i];
    for(int j = 0; j < lst.size(); j++) {
      int ds = lst[j]->dsIndex;
      if(ds >= 0)
        fitIndices[i * datasets + ds] = lst[j]->fitIndex;
      else {
        for(int k = 0; k < datasets; k++)
          fitIndices[i * datasets + k] = lst[j]->fitIndex;
      }
    }
  }
  

  int n = fitData->freeParameters();
  if(sp) {
    for(int i = -1; i < fitData->datasets.size(); i++) {
      int sz = fitData->parametersByDataset[i].size();
      if(sz > 0)
        sizes << sz;
    }
    sparseMatrix = new ABDMatrix(sizes);
  }
  else
    matrix = gsl_matrix_alloc(n, n);
}

SparseCovariance::~SparseCovariance()
{
  if(matrix)
    gsl_matrix_free(matrix);
  delete sparseMatrix;
}

double SparseCovariance::get(int i, int j) const
{
  i = fitIndices[i];
  if(i < 0)
    return 0;
  j = fitIndices[j];
  if(j < 0)
    return 0;
  if(matrix)
    return gsl_matrix_get(matrix, i, j);
  return sparseMatrix->get(i, j);
}
  
