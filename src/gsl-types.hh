/**
   \file gsl-types.hh
   A serie of thin wrappers around GSL types to avoid managing memory
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
#ifndef __GSL_TYPES_HH
#define __GSL_TYPES_HH

/// Thin wrapper around gsl_vector
class GSLVector {
  /// Managed vector
  gsl_vector * data;
public:
  GSLVector(size_t size) {
    data = gsl_vector_alloc(size);
  };

  ~GSLVector() {
    gsl_vector_free(data);
  };

  gsl_vector* operator->() {
    return data;
  };

  double operator[](int j) const {
    return gsl_vector_get(data, j);
  };

  double & operator[](int j) {
    return *gsl_vector_ptr(data, j);
  };

  operator gsl_vector*() {
    return data;
  };

  operator const gsl_vector*() const {
    return data;
  };
};

/// Thin wrapper around gsl_matrix
class GSLMatrix {
  /// Managed matrix
  gsl_matrix * data;
public:
  GSLMatrix(size_t s1, size_t s2) {
    data = gsl_matrix_alloc(s1, s2);
  };

  ~GSLMatrix() {
    gsl_matrix_free(data);
  };

  gsl_matrix* operator->() {
    return data;
  };

  operator gsl_matrix*() {
    return data;
  };
};

/// This class is here to hold temporary matrices for computations.
///
/// Just reserve and use as a matrix
class ScratchPadMatrix {
  double * data;
  gsl_matrix_view view;
  int size;
public:

  ScratchPadMatrix() : data(NULL), size(0) {;};
  ~ScratchPadMatrix() {
    delete data;
  };

  void reserve(int s1, int s2) {
    if(size < s1*s2) {
      delete[] data;
      size = s1 * s2;
      data = new double[size];
    }
    view = gsl_matrix_view_array(data, s1, s2);
  };

  gsl_matrix* operator->() {
    return &view.matrix;
  };

  const gsl_matrix* operator->() const {
    return &view.matrix;
  };

  operator gsl_matrix*() {
    return &view.matrix;
  };

  operator const gsl_matrix*() const {
    return &view.matrix;
  };

};


#endif
