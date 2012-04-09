/*
  fft.cc: implementation of the FFT class
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

#include <headers.hh>
#include <fft.hh>

void FFT::setup()
{
  realWT = QSharedPointer<gsl_fft_real_wavetable>
    (gsl_fft_real_wavetable_alloc(data.size()), 
     gsl_fft_real_wavetable_free);
  hcWT = QSharedPointer<gsl_fft_halfcomplex_wavetable> 
    (gsl_fft_halfcomplex_wavetable_alloc(data.size()), 
     gsl_fft_halfcomplex_wavetable_free);
  fftWS = QSharedPointer<gsl_fft_real_workspace>
    (gsl_fft_real_workspace_alloc(data.size()),
     gsl_fft_real_workspace_free);
}


FFT::FFT(const Vector & x, const Vector & y, bool autoBL) :
  data(y), useCubicBaseline(false)
{
  deltaX = x[1] - x[0];
  firstX = x[0];
  if(autoBL)
    computeBaseline(x,y);
  setup();
}

FFT::FFT(double dx, double fx, const Vector & y) : 
  deltaX(dx), firstX(fx), data(y), useCubicBaseline(false)
{
  setup();
}

void FFT::computeBaseline(const Vector & x, const Vector & y, 
                          double alpha )
{
  /// @todo !
}

void FFT::forward(bool useBaseline)
{
  
  if(useBaseline) {
    // Subtract the baseline
    for(int i = 0; i < data.size(); i++) {
      double x = firstX + i * deltaX;
      double val = 0;
      double coeff = 1;
      for(int j = 0; j < 4; j++) {
        val += baseLine[j] * coeff;
        coeff *= x;
      }
      data[i] -= val;
    }
  }
  gsl_fft_real_transform(data.data(), 1, data.size(), realWT.data(), 
                         fftWS.data());
  
}

void FFT::reverse(bool useBaseline)
{
  
  gsl_fft_halfcomplex_transform(data.data(), 1, data.size(), hcWT.data(), 
                                fftWS.data());

  if(useBaseline) {
    // add back the baseline
    for(int i = 0; i < data.size(); i++) {
      double x = firstX + i * deltaX;
      double val = 0;
      double coeff = 1;
      for(int j = 0; j < 4; j++) {
        val += baseLine[j] * coeff;
        coeff *= x;
      }
      data[i] += val;
    }
  }
  
}
