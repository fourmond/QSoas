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

Solver::Solver(const gsl_root_fdfsolver_type * t) :
  fdfsolver(NULL), fsolver(NULL),
  absolutePrec(0), relativePrec(1e-4), maxIterations(25), type(t)
{
}

void Solver::freeSolvers()
{
  if(fdfsolver)
    gsl_root_fdfsolver_free(fdfsolver);
  fdfsolver = NULL;
  if(fsolver)
    gsl_root_fsolver_free(fsolver);
  fsolver = NULL;
}

Solver::~Solver()
{
  freeSolvers();
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
  if(fdfsolver)
    return gsl_root_fdfsolver_root(fdfsolver);
  else
    return gsl_root_fsolver_root(fsolver);
}

int Solver::iterate()
{
  if(fdfsolver)
    return gsl_root_fdfsolver_iterate(fdfsolver);
  else
    return gsl_root_fsolver_iterate(fsolver);
}

void Solver::initialize(double x0)
{
  freeSolvers();
  fdfsolver = gsl_root_fdfsolver_alloc(type);
  function.f = &Solver::f;
  function.df = &Solver::df;
  function.fdf = &Solver::fdf;
  function.params = this;

  /// @todo Check return value ?
  gsl_root_fdfsolver_set(fdfsolver, &function, x0);
}

void Solver::initialize(double min, double max)
{
  freeSolvers();
  fsolver = gsl_root_fsolver_alloc(gsl_root_fsolver_brent);

  if(min > max)
    std::swap(min, max);
  fnc.function = &Solver::f;
  fnc.params = this;

  gsl_root_fsolver_set(fsolver, &fnc, min, max);
}

double Solver::solve(double x0)
{
  initialize(x0);
  return solve();
}

double Solver::solve(double min, double max)
{
  initialize(min, max);
  return solve();
}

double Solver::solve()
{
  int nb = 0;
  double xp = currentValue();
  while(true) {
    iterate();
    double xn = currentValue();

    int status = 0;
    if(fsolver)
      gsl_root_test_interval(gsl_root_fsolver_x_lower(fsolver),
                             gsl_root_fsolver_x_upper(fsolver),
                             absolutePrec, relativePrec);
    else 
      status = gsl_root_test_delta (xn, xp, absolutePrec, relativePrec);

    if (status == GSL_SUCCESS)
      return xn;
    nb++;
    xp = xn;
    if(nb >= maxIterations)
      throw RuntimeError("Maximum number of iterations reached, giving up !");
  }
  return xp;
}


//////////////////////////////////////////////////////////////////////

LambdaSolver::LambdaSolver(const std::function<double (double)> & f, 
                           const gsl_root_fdfsolver_type * type) :
  Solver(type), fnc(f)
{
  
}

double LambdaSolver::f(double x)
{
  return fnc(x);
}

