/*
  abdmatrix.cc: implementation of the ABDMatrix class
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
#include <abdmatrix.hh>
#include <exceptions.hh>


#include <commandlineparser.hh>

#include <utils.hh>

// Seems necessary:
#include <gsl/gsl_permute_vector_double.h>

#include <gsl-types.hh>

ABDMatrix::ABDMatrix(const QList<int> & sz) : sizes(sz)
{
  diag = new gsl_matrix *[sizes.size()];
  top = new gsl_matrix *[sizes.size() - 1];
  left = new gsl_matrix *[sizes.size() - 1];

  total = 0;
  for(int i = 0; i < sizes.size(); i++) {
    int s = sizes[i];
    total += s;
    if(! i)
      firstSize = s;
    diag[i] = gsl_matrix_alloc(s, s);
    if(i > 0) {
      top[i-1] = gsl_matrix_alloc(firstSize, s);
      left[i-1] = gsl_matrix_alloc(s, firstSize);
    }
  }

  permutation = gsl_permutation_alloc(total);
}

int ABDMatrix::totalSize() const
{
  return total;
}

ABDMatrix::~ABDMatrix()
{
  for(int i = 0; i < sizes.size(); i++) {
    gsl_matrix_free(diag[i]);
    if(i > 0) {
      gsl_matrix_free(left[i-1]);
      gsl_matrix_free(top[i-1]);
    }
  }
  delete[] diag;
  delete[] top;
  delete[] left;
  gsl_permutation_free(permutation);
}

void ABDMatrix::expandToFullMatrix(gsl_matrix * tg) const
{
  if(tg->size1 != total || tg->size2 != total)
    throw InternalError("Invalid matrix size");

  gsl_matrix_set_zero(tg);
  int cur = 0;
  for(int i = 0; i < sizes.size(); i++) {
    int sz = sizes[i];
    gsl_matrix_view m = gsl_matrix_submatrix(tg, cur, cur, sz, sz);
    gsl_matrix_memcpy(&m.matrix, diag[i]);
    if(i > 0) {
      m = gsl_matrix_submatrix(tg, 0, cur, firstSize, sz);
      gsl_matrix_memcpy(&m.matrix, top[i-1]);
      m = gsl_matrix_submatrix(tg, cur, 0, sz, firstSize);
      gsl_matrix_memcpy(&m.matrix, left[i-1]);
    }
    cur += sz;
  }
}

void ABDMatrix::invert(gsl_matrix * invert) const
{
  ABDMatrix tmp(sizes);
  if(invert->size1 != total || invert->size2 != total)
    throw InternalError("Matrix to store inverse is not of correct size");

  gsl_matrix_set_identity(invert);

  for(int i = 0; i < total;i++) {
    gsl_vector_view v = gsl_matrix_column(invert, i);
    tmp.copyFrom(*this);
    tmp.solve(&v.vector);
  }
}

void ABDMatrix::almostInvert(ABDMatrix * target) const
{
  ABDMatrix tmp(sizes);
  if(target->sizes != sizes)
    throw InternalError("ABDMatrix to store inverse is not of correct size");

  GSLVector v(total);
  for(int i = 0; i < total;i++) {
    gsl_vector_set_basis(v, i);
    tmp.copyFrom(*this);
    tmp.solve(v);
    // Completely inefficient, but it should not matter much
    for(int j = 0; j < total; j++)
      target->set(i, j, v[j]);
  }
}


void ABDMatrix::addToDiagonal(double value)
{
  for(int i = 0; i < sizes.size(); i++) {
    gsl_vector_view v = gsl_matrix_diagonal(diag[i]);
    gsl_vector_add_constant(&v.vector, value);
  }
}

void ABDMatrix::setFromProduct(const gsl_matrix * src)
{
  if(src->size2 != total)
    throw InternalError("matrix size mismatch");
  int cur = 0;
  gsl_matrix_const_view v1 = gsl_matrix_const_submatrix(src, 0, cur, src->size1, sizes[0]);
  for(int i = 0; i < sizes.size(); i++) {
    // First, the diagonal matrix:
    int sz = sizes[i];
    gsl_matrix * m = diag[i];
    gsl_matrix_const_view v = gsl_matrix_const_submatrix(src, 0, cur, src->size1, sz);
    gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1, 
                   &v.matrix, &v.matrix, 0, m);

    // Then, the top matrices
    if(i > 0) {
      m = top[i-1];
      gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1, 
                     &v1.matrix, &v.matrix, 0, m);
      gsl_matrix_transpose_memcpy(left[i-1], m);
    }
    cur += sz;
  }
}

void ABDMatrix::whereIndex(int overallIdx, int & mId, int & idx) const
{
  if(overallIdx >= 0) {
    mId = 0;
    idx = overallIdx;
    while(mId < sizes.size()) {
      if(idx < sizes[mId])
        return;
      idx -= sizes[mId];
      mId++;
    }
  }
  throw RuntimeError("Index out of bounds for ABDMatrix: %1").
    arg(overallIdx);
}

void ABDMatrix::permuteVariables(int i, int j)
{
  int mi, mj, li, lj;
  whereIndex(i, mi, li);
  whereIndex(j, mj, lj);
  if(mj != mi)
    throw RuntimeError("Permutation does not keep ABDMatrix structure: %1 <-> %2").arg(i).arg(j);

  // QTextStream o(stdout);


  // First, swap both the rows and colums of the diagonal matrix
  gsl_matrix * m = diag[mi];
  // o << "Permutating " << li << " and " << lj << " in block "
  //   << mj << " of size "  << m->size1 << "x" << m->size2 << endl;
  gsl_matrix_swap_rows(m, li, lj);
  gsl_matrix_swap_columns(m, li, lj);

  if(mi > 0) {
    // Swapping the rows of the corresponding top matrices and columns
    // of left matrices.
    m = top[mi-1];
    gsl_matrix_swap_columns(m, li, lj);

    m = left[mi-1];
    gsl_matrix_swap_rows(m, li, lj);
  }
  else {
    // Swapping rows of all the top matrices and the columns of all
    // the left matrices !
    for(int k = 0; k < sizes.size() - 1; k++) {
      m = left[k];
      gsl_matrix_swap_columns(m, i, j);
      
      m = top[k];
      gsl_matrix_swap_rows(m, i, j);
    }
  }
  
}

void ABDMatrix::solve(gsl_vector * sol)
{
  // To keep the structure of the matrix, we HAVE to go from the
  // bottom to top (i.e., the inverse of the standard gauss-jordan way
  // ?)

  // The index of the first row of the diag matrix we're interested in
  // for now.
  int base = total;
  int cur = total-1;
  // current diag, current left, other diag, other left
  gsl_vector_view cd, cl, od, ol;
  gsl_vector * cdv, * clv, * odv, * olv;

  gsl_permutation_init(permutation);
  
  // QTextStream o(stdout);
  for(int i = sizes.size() - 1; i >= 0; i--) {
    int sz = sizes[i];
    base -= sz;
    for(int j = sz-1; j >= 0; j--) {
      // @todo: swapping rows
      double val;
      int k = j+1;
      do {
        k--;
        val = gsl_matrix_get(diag[i], k, k);
      } while(val == 0 && k > 0);
      
      if(val == 0)
        throw RuntimeError("(most probably) singular matrix");
      
      if(k != j) {
        // o << "Permutating " << k << " and " << j << " || "
        //   << k+base << " and " << j+base << endl;
        // Permute the elements first:
        permuteVariables(j+base, k+base);
        // o << " -> ..." << endl;
        gsl_vector_swap_elements (sol, j+base, k+base);
        gsl_permutation_swap(permutation, j+base, k+base);
      }
      cd = gsl_matrix_row(diag[i], j);
      cdv = &cd.vector;
      if(i > 0) {
        cl = gsl_matrix_row(left[i-1], j);
        clv = &cl.vector;
      }
      else
        clv = NULL;

      val = 1/val;

      // First, scale the line we're dealing with:
      gsl_vector_scale(cdv, val);
      if(clv)
        gsl_vector_scale(clv, val);
      gsl_vector_set(sol, cur, val * gsl_vector_get(sol, cur));

      double rhs = gsl_vector_get(sol, cur);

      // Then, subtract to all lines above in the same matrix:
      for(int k = j-1; k >= 0; k--) {
        od = gsl_matrix_row(diag[i], k);
        odv = &od.vector;
        if(i > 0) {
          ol = gsl_matrix_row(left[i-1], k);
          olv = &ol.vector;
        }
        else
          olv = NULL;
        double fact = gsl_vector_get(odv, j);
        gsl_blas_daxpy(-fact, cdv, odv);
        if(olv)
          gsl_blas_daxpy(-fact, clv, olv);
        // And in the RHS
        gsl_vector_set(sol, base + k, 
                       gsl_vector_get(sol, base + k) - fact * rhs);
      }

      // Now, we do the same thing at the top
      if(i > 0) {
        for(int k = 0; k < firstSize; k++) {
          od = gsl_matrix_row(top[i-1], k);
          odv = &od.vector;
          ol = gsl_matrix_row(diag[0], k);
          olv = &ol.vector;

          double fact = gsl_vector_get(odv, j);
          gsl_blas_daxpy(-fact, cdv, odv);
          gsl_blas_daxpy(-fact, clv, olv);
          // And in the RHS
          gsl_vector_set(sol, k, 
                         gsl_vector_get(sol, k) - fact * rhs);
        }
      }
      cur -= 1;
    }
  }

  // Now, we do back-substitution
  base = 0;
  for(int i = 0; i < sizes.size(); i++) {
    int sz = sizes[i];
    for(int j = 0; j < sz; j++) {
      double val = gsl_vector_get(sol, base+j);
      
      // Back subs on the matrix:
      for(int k = j+1; k < sz; k++) {
        double coeff = gsl_matrix_get(diag[i], k, j);
        gsl_vector_set(sol, base + k, 
                       gsl_vector_get(sol, base + k) - coeff * val);
      }
      if(i == 0) {
        // This is the special case where we have to go through the
        // whole left to perform back substitution
        int c2 = firstSize;
        for(int i2 = 1; i2 < sizes.size(); i2++) {
          int sz2 = sizes[i2];
          for(int k = 0; k < sz2; k++, c2++) {
            double coeff = gsl_matrix_get(left[i2-1], k, j);
            gsl_vector_set(sol, c2, 
                           gsl_vector_get(sol, c2) - coeff * val);
          }
        }
      }  
    }
    base += sz;
  }

  gsl_permute_vector_inverse(permutation, sol);  

}

void ABDMatrix::copyFrom(const ABDMatrix & src)
{
  if(sizes != src.sizes)
    throw InternalError("Mismatched sizes while copying");
  for(int i = 0; i < sizes.size(); i++) {
    gsl_matrix_memcpy(diag[i], src.diag[i]);
    if(i > 0) {
      gsl_matrix_memcpy(left[i-1], src.left[i-1]);
      gsl_matrix_memcpy(top[i-1], src.top[i-1]);
    }
  }
}

void ABDMatrix::clear()
{
  for(int i = 0; i < sizes.size(); i++) {
    gsl_matrix_set_zero(diag[i]);
    if(i > 0) {
      gsl_matrix_set_zero(left[i-1]);
      gsl_matrix_set_zero(top[i-1]);
    }
  }
}

double ABDMatrix::get(int i, int j) const
{
  if(i > j)
    std::swap(i,j);
  // OK, now i is the lowest.

  int iblk, isub;
  whereIndex(i, iblk, isub);
  int jblk, jsub;
  whereIndex(j, jblk, jsub);
  if(iblk == jblk)
    return gsl_matrix_get(diag[iblk], isub, jsub);
  if(iblk == 0)
    return gsl_matrix_get(top[jblk-1], isub, jsub);
  return 0;                     // But should fail
}

void ABDMatrix::set(int i, int j, double v)
{
  if(i > j)
    std::swap(i,j);
  // OK, now i is the lowest.

  int iblk, isub;
  whereIndex(i, iblk, isub);
  int jblk, jsub;
  whereIndex(j, jblk, jsub);
  if(iblk == jblk) {
    gsl_matrix_set(diag[iblk], isub, jsub, v);
    if(jsub != isub)
      gsl_matrix_set(diag[iblk], jsub, isub, v);
    return;
  }
  if(iblk == 0) {
    gsl_matrix_set(top[jblk-1], isub, jsub, v);
    gsl_matrix_set(left[jblk-1], jsub, isub, v);
    return;
  }
  throw InternalError("Trying to set the %1,%2 element of an ABDMatrix, but this is a 0-only element").arg(i).arg(j);
}
