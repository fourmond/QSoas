/*
  bsplines.cc: implementation of the BSplines class
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

#include <headers.hh>
#include <exceptions.hh>
#include <bsplines.hh>
#include <dataset.hh>

#include <terminal.hh>

#include <gsl/gsl_blas.h>

BSplines::BSplines(const Vector & xvalues, 
                   const Vector & yvalues, int o, int mo) :
  splinesWS(NULL), order(o), maxOrder(mo), x(xvalues), y(yvalues)
{
}

BSplines::BSplines(const DataSet * ds, int o, int mo) :
  splinesWS(NULL), order(o), maxOrder(mo), x(ds->x()), y(ds->y())
{
}

BSplines::~BSplines()
{
  freeWS();
}

void BSplines::allocateWS()
{
  if(splinesWS)
    throw InternalError("Should not allocate twice B-splines workspace");
  if(breakPoints.size() < 1)
    throw InternalError("Should have at least one breakpoint");
  

  splinesWS = gsl_bspline_alloc(order, breakPoints.size());
  derivWS = gsl_bspline_deriv_alloc(order);
  nbCoeffs = gsl_bspline_ncoeffs(splinesWS);

  nb = x.size();
  if(nb != y.size())
    throw InternalError("X and Y size differ");


  for(int i = 0; i <= maxOrder; i++)
    splines << gsl_matrix_alloc(nb, nbCoeffs);

  storage = gsl_matrix_alloc(nbCoeffs, maxOrder + 1);

  fitWS = gsl_multifit_linear_alloc(nb, nbCoeffs);

  coeffs = gsl_vector_alloc(nbCoeffs);
  cov = gsl_matrix_alloc(nbCoeffs, nbCoeffs);
}

void BSplines::freeWS()
{
  if(! splinesWS)
    return;
  for(int i = 0; i <= maxOrder; i++)
    gsl_matrix_free(splines[i]);
  splines.clear();
  gsl_matrix_free(cov);
  gsl_matrix_free(storage);
  gsl_vector_free(coeffs);

  
  gsl_multifit_linear_free(fitWS);
  gsl_bspline_free(splinesWS);
  gsl_bspline_deriv_free(derivWS);


  splinesWS = NULL;
}

void BSplines::setBreakPoints(const Vector & bps)
{
  int oldsize = breakPoints.size();
  breakPoints = bps;

  qSort(breakPoints);
  if(breakPoints.min() > x.min())
    breakPoints.prepend(x.min());
  if(breakPoints.max() < x.max())
    breakPoints.append(x.max());

  if(breakPoints.size() != oldsize) {
    freeWS();                   // Reallocate everything
    allocateWS();
  }

  gsl_vector_view v = gsl_vector_view_array(breakPoints.data(), 
                                            breakPoints.size());
  gsl_bspline_knots(&v.vector, splinesWS);


  const double * xvals = x.data();

  for(int i = 0; i < nb; i++) {
    gsl_bspline_deriv_eval(xvals[i], maxOrder, storage, 
                           splinesWS, derivWS);
    for(int j = 0; j <= maxOrder; j++) {
      gsl_vector_view s = gsl_matrix_column(storage, j);
      gsl_vector_view v = gsl_matrix_row(splines[j], i);
      gsl_vector_memcpy(&v.vector, &s.vector);
    }
  }

}

void BSplines::setOrder(int no)
{
  if(no == order)
    return;
  freeWS();
  order = no;
  Vector bps = breakPoints;
  allocateWS();
  setBreakPoints(bps);
}

void BSplines::autoBreakPoints(int nbPoints)
{
  Vector bps;
  double xmin = x.min();
  double xmax = x.max();
  for(int i = 0; i < nbPoints; i++)
    bps << xmin + (xmax - xmin)/(nbPoints + 1)*(i+1);
  setBreakPoints(bps);
}


double BSplines::computeCoefficients()
{
  double chisq;
  gsl_vector_const_view yv = 
    gsl_vector_const_view_array(y.data(), nb);
  
  gsl_multifit_linear(splines[0], &yv.vector,
                      coeffs, cov, &chisq, fitWS);
  return chisq;
}

void BSplines::computeValues(gsl_vector * target, int order) const
{
  gsl_blas_dgemv(CblasNoTrans, 1, splines[order], coeffs, 0, target);
}

Vector BSplines::computeValues(int order) const
{
  Vector v(nb, 0);
  gsl_vector_view target = 
    gsl_vector_view_array(v.data(), nb);
  computeValues(&target.vector, order);
  return v;
}


static int f(const gsl_vector * x, void * data, gsl_vector * f)
{
  BSplines * bs = (BSplines*) data;
  Vector bps = bs->getBreakPoints();
  gsl_vector_view v = gsl_vector_view_array(bps.data() + 1, x->size);
  gsl_vector_memcpy(&v.vector, x);
  bs->setBreakPoints(bps);
  bs->computeCoefficients();
  bs->computeValues(f);

  const Vector & yd = bs->yData();
  
  gsl_vector_const_view v2 = 
    gsl_vector_const_view_array(yd.data(), yd.size());
  gsl_vector_sub(f, &v2.vector);
  return GSL_SUCCESS;
}

static int df(const gsl_vector * x, void * data, gsl_matrix * J)
{
  BSplines * bs = (BSplines*) data;
  // I need a temporary storage vector
  gsl_vector * tmpStorage = gsl_vector_alloc(bs->yData().size());

  Vector bps = bs->getBreakPoints();
  gsl_vector_view v = gsl_vector_view_array(bps.data() + 1, x->size);
  gsl_vector_memcpy(&v.vector, x);

  bs->setBreakPoints(bps);
  bs->computeCoefficients();
  bs->computeValues(tmpStorage);


  for(int i = 0; i < x->size; i++) {
    // Pick up a reasonable dx: 1/10 of the distance between here and
    // next step
    double dx = 0.1*(bps[i+2] - bps[i+1]);
    double oldx = bps[i+1];
    bps[i+1] += dx;

    bs->setBreakPoints(bps);
    bs->computeCoefficients();
    gsl_vector_view s = gsl_matrix_column(J, i);
    bs->computeValues(&s.vector);
    gsl_vector_sub(&s.vector, tmpStorage);
    gsl_vector_scale(&s.vector,1/dx);

    bps[i+1] = oldx;
  }

  gsl_vector_free(tmpStorage);
  return GSL_SUCCESS;
}

static int fdf(const gsl_vector * x, void * data, gsl_vector * fs,
               gsl_matrix * J)
{
  f(x,data,fs);
  return df(x,data,J);
}

void BSplines::optimize(int maxIterations, bool silent)
{
  Vector bps = breakPoints;
  gsl_vector_view params = gsl_vector_view_array(bps.data() + 1, 
                                                 bps.size() - 2);
  gsl_multifit_function_fdf f;
  f.f = &::f;
  f.df = &::df;
  f.fdf = &::fdf;
  f.n = nb;
  f.p = bps.size() - 2;
  f.params = this;

  gsl_multifit_fdfsolver *s = 
    gsl_multifit_fdfsolver_alloc(gsl_multifit_fdfsolver_lmsder, nb, 
                                 f.p);

  gsl_multifit_fdfsolver_set(s, &f, &params.vector);

  int status;
  int i = 0;
  do {
    maxIterations--;
    status = gsl_multifit_fdfsolver_iterate(s);
    if (status)
      break;
    status = gsl_multifit_test_delta(s->dx, s->x,
                                     1e-4, 1e-4);
    if(! silent) {
      Terminal::out << "Iteration: " << i++ 
                    << " -- residuals: " << gsl_blas_dnrm2(s->f)
                    << endl;

      // Hmmm, this isn't really good, but I don't see any other way
      // to do
      QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
  }
  while(status == GSL_CONTINUE && maxIterations >0);

  gsl_vector_memcpy(&params.vector, s->x);
  setBreakPoints(bps);

  gsl_multifit_fdfsolver_free(s);
}
