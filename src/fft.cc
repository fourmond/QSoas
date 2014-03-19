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
#include <dataset.hh>

#include <exceptions.hh>

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


FFT::FFT(const Vector & x, const Vector & y, bool autoBL, double alpha) :
  useCubicBaseline(false)
{
  initialize(x, y, autoBL, alpha);
  setup();
}

void FFT::initialize(const Vector & x, const Vector & y, bool autoBL, 
                     double alpha)
{
  data = y;
  deltaX = (x.last() - x.first())/(x.size() - 1);
  firstX = x[0];
  if(autoBL)
    computeBaseline(x,y, alpha);
}



FFT::FFT(double dx, double fx, const Vector & y) : 
  deltaX(dx), firstX(fx), data(y), useCubicBaseline(false)
{
  setup();
}

void FFT::computeBaseline(const Vector & x, const Vector & y, 
                          double alpha)
{
  DataSet ds(x,y);
  int nb = x.size();
  int pa = alpha * nb;
  QPair<double, double> beg = ds.reglin(0, pa);
  QPair<double, double> end = ds.reglin(nb - pa -1,nb -1);

  double x1 = x.first();
  double sl1 = beg.first;
  double y1 = beg.first * x1 + beg.second; 

  double x2 = x.last();
  double sl2 = end.first;
  double y2 = end.first * x2 + end.second; 

  double delta = (-pow(x1,3)+3*x2*pow(x1,2)+pow(x2,3)-3*x1*pow(x2,2));
  if(delta != 0) {
    // Is this correct ?

    /// @todo use a matrix ?
    baseLine[0] = -(-y1*pow(x2,3) + 3*y1*x1*pow(x2,2) + 
                    pow(x1,2)*pow(x2,2)*sl2 - pow(x1,2)*pow(x2,2)*sl1 - 
                    3*pow(x1,2)*x2*y2-pow(x1,3)*x2*sl2 + 
                    pow(x1,3)*y2+x1*sl1*pow(x2,3))/delta;
    baseLine[1] = (2*pow(x2,2)*x1*sl2 + pow(x2,2)*x1*sl1+6*x1*x2*y1 - 
                   6*x1*x2*y2-2*pow(x1,2)*x2*sl1 - 
                   pow(x1,2)*x2*sl2-pow(x1,3)*sl2+pow(x2,3)*sl1)/delta;
    baseLine[2] = -(pow(x2,2)*sl2+2*pow(x2,2)*sl1+3*x2*y1 - 
                    3*x2*y2-x2*x1*sl1+x2*x1*sl2-2*pow(x1,2)*sl2 + 
                    3*x1*y1-3*x1*y2-pow(x1,2)*sl1)/delta;
    baseLine[3] = (2*y1-x1*sl2-x1*sl1+x2*sl2+x2*sl1-2*y2)/delta;
  }
  else {
    baseLine[0] = beg.second;
    baseLine[1] = beg.first;
    baseLine[2] = 0;
    baseLine[3] = 0;
  }
}

double FFT::baseline(double x) const
{
  double val = 0;
  double coeff = 1;
  for(int j = 0; j < 4; j++) {
    val += baseLine[j] * coeff;
    coeff *= x;
  }
  return val;
}

void FFT::forward(bool useBaseline)
{
  
  if(useBaseline) {
    // Subtract the baseline
    for(int i = 0; i < data.size(); i++)
      data[i] -= baseline(firstX + i * deltaX);
  }
  gsl_fft_real_transform(data.data(), 1, data.size(), realWT.data(), 
                         fftWS.data());
  
}

void FFT::backward()
{
  
  gsl_fft_halfcomplex_transform(data.data(), 1, data.size(), hcWT.data(), 
                                fftWS.data());
}

void FFT::reverse(bool useBaseline)
{
  backward();
  data /= data.size();
  if(useBaseline) {
    // Subtract the baseline
    for(int i = 0; i < data.size(); i++)
      data[i] += baseline(firstX + i * deltaX);
  }
  
}

void FFT::baseline(Vector * y) const
{
  /// @todo will crash if used with a vector smaller ?
  for(int i = 0; i < data.size(); i++)
    (*y)[i] = baseline(firstX + i * deltaX);
}

Vector FFT::baseline() const
{
  Vector y(data.size(), 0);
  baseline(&y);
  return y;
}

double FFT::magnitude(int i) const
{
  if(i == 0)
    return data[0];
  if(i >= frequencies() || i < 0)
    return 0.0/0.0;             /// @todo raise an exception ?
  int sz = data.size();
  if(sz % 2 == 0 && i == sz/2)
    return data[sz-1];
  return sqrt(data[2*i-1]*data[2*i-1] + data[2*i]*data[2*i]);
}

double & FFT::real(int i)
{
  if(i >= frequencies() || i < 0)
    throw InternalError("FFT:real(): invalid frequency: %1").arg(i);
  if(i == 0)
    return data[0];
  int sz = data.size();
  if(sz % 2 == 0 && i == sz/2)
    return data[sz-1];
  return data[2*i-1];
}

double & FFT::imag(int i)
{
  if(i >= frequencies() || i < 0)
    throw InternalError("FFT:real(): invalid frequency: %1").arg(i);
  int sz = data.size();
  if((i == 0) || (sz % 2 == 0 && i == sz/2)) {
    dummy = 0;
    return dummy;               // Never used !
  }
  return data[2*i];
}



int FFT::frequencies() const
{
  return data.size()/2 + 1;
}

Vector FFT::spectrum() const
{
  Vector s;
  int nb = frequencies();
  for(int i = 0; i < nb; i++)
    s << magnitude(i);
  return s;
}

void FFT::scaleFrequency(int i, double scale)
{
  if(i >= frequencies() || i < 0)
    return;
  if(i == 0)
    data[0] *= scale;
  else {
    int sz = data.size();
    if(i > sz/2)
      return;             /// @todo raise an exception ?
    if(sz % 2 == 0 && i == sz/2)
      data[sz-1] *= scale;
    else {
      data[2*i-1] *= scale;
      data[2*i] *= scale;
    }
  }
}

QList<int> FFT::factors() const
{
  gsl_fft_real_wavetable * wt = realWT.data();
  QList<int> lst;
  for(size_t i = 0; i < wt->nf; i++)
    lst << wt->factor[i];
  return lst;
}

void FFT::deriveBaseLine()
{
  for(int i = 0; i < 3; i++)
    baseLine[i] = (i+1) * baseLine[i+1];
  baseLine[3] = 0;
}


void FFT::differentiate()
{
  data[0] = 0;
  if(data.size() % 2 == 0)
    data[data.size() - 1] = 0;
  for(int i = 1; i < (data.size()+1)/2; i++) {
    double freq = 2 * M_PI * i/(data.size()*deltaX);
    double re = data[2*i-1];
    double im = data[2*i];
    data[2*i - 1] = - freq * im;
    data[2*i] = freq * re;
  }
  deriveBaseLine();
}

void FFT::applyGaussianFilter(double cutoff)
{
  int nb = frequencies();
  for(int i = 0; i < nb; i++) { 
    double freq = i/(nb*1.0);
    double xx = freq*freq;
    double fact = exp(-1*xx*cutoff*cutoff/2.);
    scaleFrequency(i, fact);
  }
}
