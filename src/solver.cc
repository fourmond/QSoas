/*
  solver.cc: implementation of the Solver class
  Copyright 2013, 2016 by CNRS/AMU

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

#include <general-arguments.hh>

Solver::Solver(const gsl_root_fdfsolver_type * t) :
  fdfsolver(NULL), fsolver(NULL),
  absolutePrec(0), relativePrec(1e-6), maxIterations(35), type(t)
{
}

Solver::Solver(const Solver & o) :
  fdfsolver(NULL), fsolver(NULL),
  absolutePrec(o.absolutePrec),
  relativePrec(o.relativePrec),
  maxIterations(o.maxIterations),
  type(o.type)
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
  double xr = x + step;
  step = xr - x;
  double fr = s->f(xr);
  double fl = s->f(x);

  return (fr - fl)/step;
}

void Solver::fdf(double x, void * params, double * f, double * df)
{
  Solver * s = reinterpret_cast<Solver *>(params);
  double step = 1e-7 * x;
  if(fabs(step) < 1e-13)
    step = 1e-13;
  double xr = x + step;
  step = xr - x;
  double fr = s->f(xr);
  double fl = s->f(x);
  *f = fl;
  *df = (fr - fl)/step;
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
      status = gsl_root_test_interval(gsl_root_fsolver_x_lower(fsolver),
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



QList<Argument*> Solver::commandOptions()
{
  QList<Argument*> args;
  args << new NumberArgument("prec-relative", "Relative precision",
                             "relative precision required")
       << new NumberArgument("prec-absolute", "Absolute precision",
                             "absolute precision required")
       << new IntegerArgument("iterations", "Maximum number of iterations",
                              "Maximum number of iterations before giving up")
    ;
  return args;
}

void Solver::parseOptions(const CommandOptions & opts)
{
  updateFromOptions(opts, "prec-absolute", absolutePrec);
  updateFromOptions(opts, "prec-relative", relativePrec);
  updateFromOptions(opts, "iterations", maxIterations);
}

CommandOptions Solver::currentOptions() const
{
  CommandOptions opts;
  updateOptions(opts, "prec-absolute", absolutePrec);
  updateOptions(opts, "prec-relative", relativePrec);
  updateOptions(opts, "iterations", maxIterations);

  return opts;
}

//////////////////////////////////////////////////////////////////////

LambdaSolver::LambdaSolver(const std::function<double (double)> & f, 
                           const gsl_root_fdfsolver_type * type) :
  Solver(type), function(f)
{
  
}

double LambdaSolver::f(double x)
{
  return function(x);
}

void LambdaSolver::setFunction(const std::function<double (double)> & f)
{
  function = f;
}
