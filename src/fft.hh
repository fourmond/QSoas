/**
   \file fft.hh
   The FFT class, providing GSL-based fourier transforms
   Copyright 2012 by Vincent Fourmond

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

  /// The first X value.
  double firstX;
  

  /// Data (either real of FFTed)
  Vector data;


  /// @name GSL interface
  /// 
  /// They are handled as shared pointers, for the sake of using copy
  /// constructors.
  ///
  /// @{
  QSharedPointer<gsl_fft_real_wavetable> realWT;
  QSharedPointer<gsl_fft_halfcomplex_wavetable> hcWT;
  QSharedPointer<gsl_fft_real_workspace> fftWS;
  /// @}


  /// setup the shared pointers
  void setup();

  
  
public:

  /// Whether we use a cubic baseline or not
  bool useCubicBaseline;

  /// Coefficients of a cubic baseline.
  double baseLine[4];

  FFT(const Vector & x, const Vector & y, bool autoBL = true);
  FFT(double dx, double fx, const Vector & y);

  /// Performs the (destructive) forward transform, after subtracting
  /// baseline.
  void forward(bool useBaseline = true); 

  /// Performs the reverse transform and add baseline back.
  void reverse(bool useBaseline = true);

  /// Computes a cubic baseline based on the given fraction of either
  /// side of the dataset.
  void computeBaseline(const Vector & x, const Vector & y, 
                       double alpha = 0.05);

  /// @name Frequency domain manipulations
  ///
  /// @{

  /// The magnitude of the ith frequency (0 <= i < n)
  double magnitude(int i) const;

  /// Scales the given frequency element
  double scaleFrequency(int i, double fact);
  /// @}

  
};

#endif
