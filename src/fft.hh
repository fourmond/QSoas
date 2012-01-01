/**
   \file fft.hh
   The FFT class, providing GSL-based fourier transforms
   Copyright 2011 by Vincent Fourmond

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

#ifndef __FFT_HH
#define __FFT_HH

#include <vector.hh>

#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>

/// This class provides fourier transforms facilities (including
/// filtering)
class FFT {

  /// The distance between two points from the normal space
  double deltaX;

  /// Original Y data
  Vector normal;

  /// Transformed data
  Vector ffted;

  /// Whether we have normal data
  bool unFftDone;


  gsl_fft_real_wavetable * realWT;
  gsl_fft_halfcomplex_wavetable * hcWT;
  gsl_fft_real_workspace * fftWS;

  
  /// Whether we use a cubic baseline or not
  bool useCubicBase;
  
  /// Coefficients of a cubic baseline.
  double bl[4];

  /// Sets up the mem allocation + computes the transform
  void setup(double dx, const Vector & y, bool cub);

  /// Frees all associated memory
  void free();

public:

  /// Builds a FFT object from X and Y things.
  ///
  /// Raises an exception if the points are not equally spaced.
  FFT(const Vector & x, const Vector & y, bool cub = true);
  FFT(double dx, const Vector & y, bool cub = true);
};

#endif
