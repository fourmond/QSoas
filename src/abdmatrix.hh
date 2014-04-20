/**
   \file abdmatrix.hh
   The ABDMatrix class, an almost block diagonal matrix
   Copyright 2014 by CNRS/AMU

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
#ifndef __ABDMATRIX_HH
#define __ABDMATRIX_HH

/// ABDMatrix is an (almost) block diagonal square matrix, ie a matrix
/// in the form:
///
///     D_1 A_2 A_3 A_4 .... A_n
///     B_2 D_2 0   ...      0
///     B_3 0   D_3 0 ...    0
///     ......................
///     B_n 0 ....        0  D_n
///
/// The matrix is assumed to be symmetric, too.
///
/// This class provides essentially facilities to initialize
/// efficiently enough this matrix from a tJ times J product, and to
/// solve the A x = B problem (destructively).
class ABDMatrix {
  /// Sizes of the blocks.
  ///
  /// The first size is special, since it is the size of the
  /// non-diagonal stuff.
  QList<int> sizes;

  /// The D matrices
  gsl_matrix ** diag;

  /// The A matrices
  gsl_matrix ** top;

  /// The B matrices
  gsl_matrix ** left;

  /// The total size of the matrix
  int total;

  /// The size of the first block
  int firstSize;
public:
  ABDMatrix(const QList<int> & sizes);
  ~ABDMatrix();

  /// Returns the total size of the square matrix
  int totalSize() const;

  /// Expands
  void expandToFullMatrix(gsl_matrix * tg) const;

  /// Adds the given number to the diagonal
  void addToDiagonal(double value);

  /// Sets the contents of the matrix from the product tJ times J. All
  /// the elements that don't fall on the correct place are SILENTLY
  /// IGNORED !
  ///
  /// The matrix must have totalSize() columns.
  ///
  /// This is a Gram matrix !
  void setFromProduct(const gsl_matrix * src);
};



#endif
