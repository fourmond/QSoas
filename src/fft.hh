/**
   \file fft.hh
   The FFT class, providing GSL-based fourier transforms
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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
#ifndef __FFT_HH
#define __FFT_HH

#include <vector.hh>

#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>

/// This class provides fourier transforms facilities (including
/// filtering)
///
/// @todo Implement auto padding !
class FFT {

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

  /// A dummy value to be used as a returned reference for the last
  /// frequency on even numbers
  double dummy;
  
  
public:

  /// The distance between two points from the normal space
  double deltaX;

  /// The first X value.
  double firstX;


  /// Data (either real of FFTed)
  Vector data;


  /// Whether we use a cubic baseline or not
  bool useCubicBaseline;

  /// Coefficients of a cubic baseline.
  ///
  /// @todo Turn that into a general polynomial baseline ?
  double baseLine[4];

  FFT(const Vector & x, const Vector & y, bool autoBL = true, 
      double alpha = 0.05);
  FFT(double dx, double fx, const Vector & y);

  /// Performs the (destructive) forward transform, after subtracting
  /// baseline.
  void forward(bool useBaseline = true); 

  /// (re)-initializes the object using the given X and Y values
  /// (recomputing the baseline if needed).
  void initialize(const Vector & x, const Vector & y, bool autoBL = true, 
                  double alpha = 0.05);

  /// Performs the backward (unscaled) transform.
  void backward();

  /// Performs the reverse transform and add baseline back.
  void reverse(bool useBaseline = true);

  /// Computes a cubic baseline based on the given fraction of either
  /// side of the dataset.
  void computeBaseline(const Vector & x, const Vector & y, 
                       double alpha = 0.05);

  /// @name Frequency domain manipulations
  ///
  /// @{

  /// The magnitude of the ith frequency (0 <= i < n/2)
  double magnitude(int i) const;

  /// The amplitude of all the frequencies
  Vector spectrum() const;

  /// Scales the given frequency element (0 <= i < (n+1)/2)
  void scaleFrequency(int i, double fact);

  /// Returns the number of frequencies that can be tweaked using the
  /// other functions.
  int frequencies() const;

  /// Returns a reference to the real part of the i-th frequency element
  double & real(int i);
  // double real(int i) const;

  /// Returns a reference to the imaginary part of the i-th frequency element
  double & imag(int i);
  // double imag(int i) const;


  /// @}


  /// Returns the value of the baseline at the given X location
  double baseline(double x) const;

  /// Computes the baseline into the given vector
  void baseline(Vector * y) const;

  /// Computes the baseline
  Vector baseline() const;

  /// Returns the list of factors used by the mix-radix routines
  QList<int> factors() const;

  /// Differentiates the baseline
  void deriveBaseLine();

  /// Differentiates the data (assuming it is in Fourier space)
  void differentiate();

  /// @name Filtering operations
  ///
  /// Filtering operations that apply only in the Fourier space
  /// @{

  /// Apply a Gaussian filter with the given cutoff (frequencies range
  /// from 0 to 1)
  void applyGaussianFilter(double cutoff);

  /// @}

};

#endif
