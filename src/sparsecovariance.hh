/**
   \file sparsecovariance.hh
   The SparseCovariance class, a memory- and time- efficient sparse covariance handling for massive multifits
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
#ifndef __SPARSECOVARIANCE_HH
#define __SPARSECOVARIANCE_HH


class FitData;
class FreeParameter;
class ABDMatrix;

/// SparseCovariance is a sparse matrix for the handling of massive
/// multifit derivatives.
///
/// For fit with n datasets, m data points, i global parameters and j
/// local parameters, the size of the covariance is m * (i + n*j), but
/// most of the numbers of the covariance will be zero. (all the
/// parameters local to another buffer). In fact, only m*(i+j) is
/// necessary to store the data.
///
/// The SparseCovariance handles the compact storing of the Covariance. It
/// interfaces between FitData::df and the fitengines (at least, the
/// ones that use FitData-provided derivatives).
///
/// SparseCovariance can optionally be not sparse, which is necessary
/// for the GSL-based fit engines that can't deal with sparse matrices
class SparseCovariance {

  /// The underlying fit data
  const FitData * fitData;

  /// Storage if not sparse
  gsl_matrix * matrix;

  /// Storage if sparse
  ABDMatrix * sparseMatrix;

  /// A correspondance col * datasets + dataset -> gsl_index
  QVector<int> fitIndices;

  /// THe sizes of the blocks if the matrix is sparse
  QList<int> sizes;

public:
  /// Constructs a sparse covariance from the given FitData
  SparseCovariance(const FitData * data,  bool sparse);
  ~SparseCovariance();

  /// Gets the element at i,j (i and j are referenced over the full
  /// index of parameters)
  double get(int i, int j) const;

};



#endif
