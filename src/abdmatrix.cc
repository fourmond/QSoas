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

void ABDMatrix::addToDiagonal(double value)
{
  for(int i = 0; i < sizes.size(); i++) {
    gsl_vector_view v = gsl_matrix_diagonal(diag[i]);
    gsl_vector_add_constant(&v.vector, value);
  }
}

/// Computes the i,j coefficient of src^T times src.
static inline double coeff(const gsl_matrix * src, int i, int j)
{
  double ret = 0;
  int di = src->tda;
  const double * d1 = src->data + i;
  const double * d2 = src->data + j;
  for(size_t k = 0; k < src->size1; k++, d1 += di, d2 += di)
    ret += (*d1) * (*d2);
  return ret;
}

void ABDMatrix::setFromProduct(const gsl_matrix * src)
{
  if(src->size2 != total)
    throw InternalError("matrix size mismatch");
  int cur = 0;
  for(int i = 0; i < sizes.size(); i++) {
    // First, the diagonal matrix:
    int sz = sizes[i];
    gsl_matrix * m = diag[i];
    for(int j = 0; j < sz; j++) {
      for(int k = j; k < sz; k++) {
        double val = coeff(src, cur + j, cur + k);
        gsl_matrix_set(m, j, k, val);
        if(k != j)
          gsl_matrix_set(m, k, j, val);
      }
    }
    
    // Then, the top matrices
    if(i > 0) {
      m = top[i-1];
      for(int j = 0; j < firstSize; j++) {
        for(int k = 0; k < sz; k++) {
          double val = coeff(src, j, cur + k);
          gsl_matrix_set(m, j, k, val);
        }
      }
      gsl_matrix_transpose_memcpy(left[i-1], m);
    }
    cur += sz;
  }
}


class TestStuff {
public:
  TestStuff();
};

// TestStuff a;

TestStuff::TestStuff() {
  QList<int> sizes;
  sizes << 3 << 2 << 4;
  QList<int> ps;
  ps << 50 << 100 << 50;

  int h = 0, w = 0;
  for(int i = 0; i < sizes.size(); i++) {
    h += sizes[i];
    w += ps[i];
  }

  gsl_matrix * m = gsl_matrix_alloc(w, h);

  // Now, initialize the matrix:
  gsl_matrix_set_zero(m);
  int curr = 0, curc = 0;
  for(int i = 0; i < sizes.size(); i++) {
    int max = (i == 0 ? w : curr + ps[i]);
    for(int j = 0; j < sizes[i]; j++) {
      for(int k = curr; k < max; k++)
        gsl_matrix_set(m, k, curc + j, drand48());
    }
    curc += sizes[i];
    curr += ps[i];
  }

  gsl_matrix * t1 = gsl_matrix_alloc(h, h);
  gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1, 
                 m, m, 0, t1);
  gsl_matrix * t2 = gsl_matrix_alloc(h, h);
  ABDMatrix mt(sizes);
  mt.setFromProduct(m);
  mt.expandToFullMatrix(t2);
  QString str; // = Utils::matrixString(m);
  // fprintf(stderr, "Orig:\n%s\n", str.toLocal8Bit().data());
  str = Utils::matrixString(t1);
  fprintf(stderr, "Direct:\n%s\n", str.toLocal8Bit().data());
  str = Utils::matrixString(t2);
  fprintf(stderr, "ABD:\n%s\n", str.toLocal8Bit().data());

  gsl_matrix_sub(t2, t1);

  str = Utils::matrixString(t2);
  fprintf(stderr, "Diff:\n%s\n", str.toLocal8Bit().data());
  exit(0);
}
