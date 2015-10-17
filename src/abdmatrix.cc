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
  for(int i = sizes.size() - 1; i >= 0; i--) {
    int sz = sizes[i];
    base -= sz;
    for(int j = sz-1; j >= 0; j--) {
      // @todo: swapping rows
      double val = gsl_matrix_get(diag[i], j, j);
      cd = gsl_matrix_row(diag[i], j);
      cdv = &cd.vector;
      if(i > 0) {
        cl = gsl_matrix_row(left[i-1], j);
        clv = &cl.vector;
      }
      else
        clv = NULL;

      if(val == 0.0)
        throw RuntimeError("Singular matrix -- or simply one needed permutations...");
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
