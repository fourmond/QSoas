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

#include <abdmatrix.hh>
#include <utils.hh>

SparseJacobian::SparseJacobian(const FitData * data,  bool sp,
                               gsl_matrix * mat) :
  fitData(data), sparse(sp)
{
  // total size
  int sz = data->dataPoints();
  datasets = fitData->datasets.size();

  parameters = fitData->parametersByDefinition;

  effectiveParameters = 0;
  fitIndices = QVector<int>(parameters.size() * datasets, -1);
  for(int i = 0; i < parameters.size(); i++) {
    const QList<FreeParameter *> lst = parameters[i];
    if(lst.size() > 0) {
      firstIndex << lst[0]->fitIndex;
      matrixIndex << effectiveParameters;
      for(int j = 0; j < lst.size(); j++) {
        int ds = lst[j]->dsIndex;
        if(ds >= 0)
          fitIndices[effectiveParameters * datasets + ds] = lst[j]->fitIndex;
        else {
          for(int k = 0; k < datasets; k++)
            fitIndices[effectiveParameters * datasets + k] = lst[j]->fitIndex;
        }
      }
      effectiveParameters++;
    }
    else {
      matrixIndex << -1;      // not found
      firstIndex << -1;
    }
  }

  if(mat) {
    matrix = mat;
    ownMatrix = false;
    if(! sparse)
      gsl_matrix_set_zero(matrix);
  }
  else {
    ownMatrix = true;
    if(sparse)
      matrix = gsl_matrix_alloc(sz, effectiveParameters);
    else {
      matrix = gsl_matrix_alloc(sz, fitData->freeParameters());
      gsl_matrix_set_zero(matrix);    // We need to initialize to 0
    }
  }
}

SparseJacobian::~SparseJacobian()
{
  if(ownMatrix)
    gsl_matrix_free(matrix);
}

gsl_vector * SparseJacobian::parameterVector(int index)
{
  int idx = sparse ? matrixIndex.value(index, -1) : firstIndex[index];
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
  QTextStream o(stdout);
  int idx = firstIndex[index];
  o << "Splicing parameters for index :" << index
    << " -> " << idx << endl;
  if(idx < 0)
    return;
  gsl_vector_view source_1 = gsl_matrix_column(matrix, idx);
  const QList<FreeParameter *> pms = parameters[index];
  for(int i = 1; i < pms.size(); i++) {
    int id2 = pms[i]->fitIndex;
    if(id2 < 0 || id2 == idx)
      continue;                  // Nothing to do here
    int dsi = pms[i]->dsIndex;
    o << "dsi:" << dsi << ", " << idx << ", " << id2 << endl;
    if(dsi < 0)
      continue;
    gsl_vector_view dest_1 = gsl_matrix_column(matrix, id2);
    gsl_vector_view source_2 = fitData->viewForDataset(dsi, &source_1.vector);
    gsl_vector_view dest_2 = fitData->viewForDataset(dsi, &dest_1.vector);
    gsl_vector_memcpy(&dest_2.vector, &source_2.vector);
    /// Clear the source
    gsl_vector_set_zero(&source_2.vector);
  }
}

void SparseJacobian::computejTj(gsl_matrix * target)
{
  if(! sparse) {
    gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1, 
                   matrix, matrix, 0, target);
    return;
  }

  gsl_matrix_set_zero(target);
  int ts = fitIndices.size();
  for(int i = 0; i < ts; i++) {
    int itgt = fitIndices[i];
    if(itgt < 0)
      continue;
    int ip = i / datasets;
    gsl_vector_view l1 = gsl_matrix_column(matrix, ip);
    int ids = i % datasets;
    gsl_vector_view l2 = fitData->viewForDataset(ids, &l1.vector);
    for(int j = i; j < ts; j+= datasets) {
      int jtgt = fitIndices[j];
      if(jtgt < 0)
        continue;
      int jp = j / datasets;
      gsl_vector_view r1 = gsl_matrix_column(matrix, jp);
      // ids and jds are the same
      gsl_vector_view r2 = fitData->viewForDataset(ids, &r1.vector);
      double val;
      gsl_blas_ddot(&l2.vector, &r2.vector, &val);
      val += gsl_matrix_get(target, itgt, jtgt);
      gsl_matrix_set(target, itgt, jtgt, val);
      if(itgt != jtgt)          // matrix is symmetric
        gsl_matrix_set(target, jtgt, itgt, val);
    }
  }
}

void SparseJacobian::computejTj(ABDMatrix * target)
{
  if(! sparse) {
    target->setFromProduct(matrix);
    return;
  }

  target->clear();
  int ts = fitIndices.size();
  for(int i = 0; i < ts; i++) {
    int itgt = fitIndices[i];
    if(itgt < 0)
      continue;
    int ip = i / datasets;
    gsl_vector_view l1 = gsl_matrix_column(matrix, ip);
    int ids = i % datasets;
    gsl_vector_view l2 = fitData->viewForDataset(ids, &l1.vector);
    for(int j = i; j < ts; j+= datasets) {
      int jtgt = fitIndices[j];
      if(jtgt < 0)
        continue;
      int jp = j / datasets;
      gsl_vector_view r1 = gsl_matrix_column(matrix, jp);
      // ids and jds are the same
      gsl_vector_view r2 = fitData->viewForDataset(ids, &r1.vector);
      double val;
      gsl_blas_ddot(&l2.vector, &r2.vector, &val);
      val += target->get(itgt, jtgt);
      target->set(itgt, jtgt, val);
    }
  }
}

void SparseJacobian::computeGradient(gsl_vector * target,
                                     const gsl_vector * func,
                                     double fact)
{
  if(! sparse) {
    gsl_blas_dgemv(CblasTrans, fact, 
                   matrix, func, 0, target);
    return;
  }

  gsl_vector_set_zero(target);
  int ts = fitIndices.size();
  for(int i = 0; i < ts; i++) {
    int itgt = fitIndices[i];
    if(itgt < 0)
      continue;
    int ip = i / datasets;
    gsl_vector_view l1 = gsl_matrix_column(matrix, ip);
    int ids = i % datasets;
    gsl_vector_view l2 = fitData->viewForDataset(ids, &l1.vector);
    gsl_vector_const_view r2 = fitData->viewForDataset(ids, func);
    double val = Utils::finiteProduct(&l2.vector, &r2.vector);
    // gsl_blas_ddot(&l2.vector, &r2.vector, &val);
    val *= fact;
    val += gsl_vector_get(target, itgt);
    gsl_vector_set(target, itgt, val);
  }
}
