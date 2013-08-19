/*
  msolver.cc: implementation of the MSolver class
  Copyright 2013 by Vincent Fourmond

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
#include <msolver.hh>

#include <exceptions.hh>

MSolver::MSolver(const gsl_multiroot_fdfsolver_type * t) :
  solver(NULL), type(t), absolutePrec(0), 
  relativePrec(1e-5), maxIterations(40)
{
}

MSolver::~MSolver()
{
  if(solver)
    gsl_multiroot_fdfsolver_free(solver);
}

void MSolver::initialize(const gsl_vector * init)
{
  solver = gsl_multiroot_fdfsolver_alloc(type, dimension());
  function.f = &MSolver::f;
  function.df = &MSolver::df;
  function.fdf = &MSolver::fdf;
  function.n = dimension();
  function.params = this;

  gsl_multiroot_fdfsolver_set(solver, &function, init);
}

int MSolver::f(const gsl_vector * x, void * params, gsl_vector * f)
{
  MSolver * slv = reinterpret_cast<MSolver*>(params);
  return slv->f(x, f);
}

int MSolver::fdf(const gsl_vector * x, void * params, 
                 gsl_vector * f, gsl_matrix * J)
{
  MSolver * slv = reinterpret_cast<MSolver*>(params);

  slv->f(x, f);


  int n = slv->dimension();
  double tmp[n];
  gsl_vector_view v = gsl_vector_view_array(tmp, n);
  gsl_vector_memcpy(&v.vector, x);

  for(int i = 0; i < n; i++) {
    double old = tmp[i];

    double step = old * 1e-7;
    if(fabs(step) <= 1e-12)
      step = 1e-12;

    tmp[i] = old + step;
    gsl_vector_view col = gsl_matrix_column(J, i);

    slv->f(&v.vector, &col.vector);
    gsl_vector_sub(&col.vector, f);
    gsl_vector_scale(&col.vector, 1/step);
    
    tmp[i] = old;
  }

  return GSL_SUCCESS;
}

int MSolver::df(const gsl_vector * x, void * params, gsl_matrix * J)
{
  int n = x->size;
  double tmp[n];
  gsl_vector_view v = gsl_vector_view_array(tmp, n);
  return fdf(x, params, &v.vector, J);
}

int MSolver::iterate()
{
  if(! solver)
    throw InternalError("Trying to use iterate() on an uninitialized solver");
  return gsl_multiroot_fdfsolver_iterate(solver);
}

const gsl_vector * MSolver::currentValue() const
{
  if(! solver)
    throw InternalError("Trying to use currentValue() "
                        "on an uninitialized solver");
  return gsl_multiroot_fdfsolver_root(solver);
}

void MSolver::solve()
{
  if(! solver)
    throw InternalError("Trying to use solve() "
                        "on an uninitialized solver");
  int nb = 0;
  while(true) {
    iterate();
    int status = 
      gsl_multiroot_test_delta(gsl_multiroot_fdfsolver_dx(solver), 
                               gsl_multiroot_fdfsolver_root(solver), 
                               absolutePrec, relativePrec);

    if (status == GSL_SUCCESS)
      return;
    nb++;
    if(nb >= maxIterations)
      throw RuntimeError("Maximum number of iterations reached, giving up !");
  }
}


