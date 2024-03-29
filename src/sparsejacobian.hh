/**
   \file sparsejacobian.hh
   The SparseJacobian class, a memory- and time- efficient sparse jacobian handling for massive multifits
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
#ifndef __SPARSEJACOBIAN_HH
#define __SPARSEJACOBIAN_HH


class FitData;
class FreeParameter;
class ABDMatrix;

/// SparseJacobian is a sparse matrix for the handling of massive
/// multifit derivatives.
///
/// For fit with n datasets, m data points, i global parameters and j
/// local parameters, the size of the jacobian is m * (i + n*j), but
/// most of the numbers of the jacobian will be zero. (all the
/// parameters local to another buffer). In fact, only m*(i+j) is
/// necessary to store the data.
///
/// The SparseJacobian handles the compact storing of the Jacobian. It
/// interfaces between FitData::df and the fitengines (at least, the
/// ones that use FitData-provided derivatives).
///
/// SparseJacobian can optionally be not sparse, which is necessary
/// for the GSL-based fit engines that can't deal with sparse matrices
class SparseJacobian {

  /// The underlying fit data
  const FitData * fitData;

  bool ownMatrix;

  /// Whether the matrix is truly sparse or not
  bool sparse;

  /// A double array with the parameters, as in
  /// FitData::parameterByDefinition.
  QList<QList<FreeParameter *> > parameters;

  /// A correspondance [index in parameters -> index in the matrix]
  /// (not necessarily the same, if a parameter is completely fixed)
  QVector<int> matrixIndex;

  /// The effective number of parameters (i.e. parameters.size() -
  /// those that are fixed). This is the dimension of the matrix if it
  /// is sparse.
  int effectiveParameters;

  /// The underlying matrix
  gsl_matrix * matrix;

  /// buffers for the row/column view
  gsl_vector_view v1, v2;

  /// The number of datasets
  int datasets;

  /// A correspondance col * datasets + dataset -> gsl_index
  QVector<int> fitIndices;

  /// The first index of each parameter
  QVector<int> firstIndex;

public:
  /// Constructs a sparse jacobian from the given FitData
  ///
  /// If @a matrix is provided the jacobian uses it directly.
  explicit SparseJacobian(const FitData * data,  bool sparse = true,
                          gsl_matrix * matrix = NULL);

  ~SparseJacobian();

  /// Returns the vector for the given parameter (indexed as in
  /// FitData::parameterByDefinition), or returns NULL if there is no
  /// free parameters of the given index.
  ///
  /// @warning The pointer stays valid only until the next call to
  /// any parameterVector().
  gsl_vector * parameterVector(int index);


  /// Returns the vector corresponding to the given parameter and the
  /// given dataset, or NULL if there isn't one.
  gsl_vector * parameterVector(int index, int dataset);

  /// Splices the elements for the given parameter back into place if
  /// the matrix isn't sparse, or do nothing if the matrix is truly sparse
  void spliceParameter(int index);

  /// Computes the value of the J^T J matrix into target.
  void computejTj(gsl_matrix * target);

  /// Computes the value of the J^T J matrix into target.
  void computejTj(ABDMatrix * target);

  /// Computes the value of J^T func.  Optionnally scaled with fact.
  void computeGradient(gsl_vector * target, const gsl_vector * func,
                       double fact = 1);

  /// Applies the jacobian matrix to the given vector, i.e. predicts
  /// the change resulting from changing the parameters by delta_p,
  /// and stores it into delta_f.
  ///
  /// @todo an appliedNorm function that computes directly the norm ?
  ///
  /// @todo This should be const.
  void apply(const gsl_vector * delta_p, gsl_vector * delta_f);

  /// Adds the given other sparse jacobian to this one. @a factor is a
  /// factor by which this matrix is multiplied before the addition. Thus:
  ///
  /// j2.addJacobian(j1, -1) results in j1 - j2
  void addJacobian(const SparseJacobian & other, double factor);

};



#endif
