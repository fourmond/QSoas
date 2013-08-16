/*
  solver.cc: implementation of the Solver class
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
#include <solver.hh>

#include <exceptions.hh>

Solver::Solver(const gsl_root_fdfsolver_type * type) :
  absolutePrec(0), relativePrec(1e-4), maxIterations(15)
{
  fdfsolver = gsl_root_fdfsolver_alloc(type);
}

Solver::~Solver()
{
  gsl_root_fdfsolver_free(fdfsolver);
}

double Solver::f(double x, void * params)
{
  Solver * s = reinterpret_cast<Solver *>(params);
  return s->f(x);
}

double Solver::df(double x, void * params)
{
  Solver * s = reinterpret_cast<Solver *>(params);
  double step = 1e-7 * x;
  if(fabs(step) < 1e-13)
    step = 1e-13;

  return (s->f(x + step) - s->f(x))/step;
}

void Solver::fdf(double x, void * params, double * f, double * df)
{
  Solver * s = reinterpret_cast<Solver *>(params);
  double step = 1e-7 * x;
  if(fabs(step) < 1e-13)
    step = 1e-13;
  *f = s->f(x);
  *df = (s->f(x + step) - *f)/step;
}

double Solver::currentValue() const
{
  return gsl_root_fdfsolver_root(fdfsolver);
}

int Solver::iterate()
{
  return gsl_root_fdfsolver_iterate(fdfsolver);
}

void Solver::initialize(double x0)
{
  function.f = &Solver::f;
  function.df = &Solver::df;
  function.fdf = &Solver::fdf;
  function.params = this;

  /// @todo Check return value ?
  gsl_root_fdfsolver_set(fdfsolver, &function, x0);
}

double Solver::solve(double x0)
{
  initialize(x0);
  return solve();
}

double Solver::solve()
{
  int nb = 0;
  double xp = currentValue();
  while(true) {
    iterate();
    double xn = currentValue();
    int status = gsl_root_test_delta (xn, xp, absolutePrec, relativePrec);

    if (status == GSL_SUCCESS)
      return xn;
    nb++;
    xp = xn;
    if(nb >= maxIterations)
      throw RuntimeError("Maximum number of iterations reached, giving up !");
  }
  return xp;
}
