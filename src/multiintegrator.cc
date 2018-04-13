/*
  multiintegrator.cc: implementation of the MultiIntegrator class
  Copyright 2015 by CNRS/AMU

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
#include <multiintegrator.hh>

#include <exceptions.hh>

#include <general-arguments.hh>
#include <factoryargument.hh>

MultiIntegrator::MultiIntegrator(Function fnc, int dim, double rel, double abs, int maxc) :
  absolutePrec(abs), relativePrec(rel), maxfuncalls(maxc), function(fnc),
  dimension(dim)
{
  funcalls = 0;
}

MultiIntegrator * MultiIntegrator::createNamedIntegrator(const QString & name,
                                                         Function fnc,
                                                         int dimension,
                                                         double relative,
                                                         double abs,
                                                         int maxc)
{
  return MultiIntegratorFactory::createObject(name, fnc, dimension, relative, abs, maxc);
}

uint qHash(const WrappedDouble & val)
{
  const quint64 * ptr = reinterpret_cast<const quint64 *>(&val.value);
  return qHash(*ptr);
}


MultiIntegrator::~MultiIntegrator()
{
  clearEvaluations();
}



void MultiIntegrator::internalReset()
{
}

void MultiIntegrator::clearEvaluations()
{
  for(gsl_vector * v : evaluations)
    gsl_vector_free(v);
  evaluations.clear();
  funcalls = 0;
}

void MultiIntegrator::reset(MultiIntegrator::Function fnc, int dim)
{
  clearEvaluations();
  function = fnc;
  dimension = dim;
}


gsl_vector * MultiIntegrator::functionForValue(double value)
{
  WrappedDouble v(value);
  if(! evaluations.contains(v)) {
    if(maxfuncalls > 0 && funcalls >= maxfuncalls)
      throw RuntimeError("Maximum of values of X reached during integration, aborting (nb = %1, x = %2)").arg(funcalls).arg(value);

    gsl_vector * vct = gsl_vector_alloc(dimension);
    function(value, vct);
    funcalls += 1;
    evaluations[v] = vct;
  }
  return evaluations[v];
}

QList<Argument *> MultiIntegrator::integratorOptions()
{
  QList<Argument *> args;
  args << new FactoryArgument<MultiIntegratorFactory>
    ("integrator", 
     "Integrator",
     "The algorithm used for integration")
       << new NumberArgument("prec-relative", "Relative integration precision",
                             "Relative precision required for integration")
       << new NumberArgument("prec-absolute", "Absolute integration precision",
                             "Absolute precision required for integration")
       << new IntegerArgument("max-evaluations", "Maximum evaluations",
                              "Maximum number of function evaluations");
  return args;
}

MultiIntegrator * MultiIntegrator::fromOptions(const CommandOptions & opts,
                                               MultiIntegrator::Function fcn, int dimension)
{
  MultiIntegratorFactory * c = MultiIntegratorFactory::namedItem("gk41");
  
  updateFromOptions(opts, "integrator", c);

  double relPrec = 1e-4;
  updateFromOptions(opts, "prec-relative", relPrec);
  double absPrec = 0;
  updateFromOptions(opts, "prec-absolute", absPrec);
  int maxc = 0;

  MultiIntegrator * integrator =
    c->creator(fcn, dimension, relPrec, absPrec, maxc);
  updateFromOptions(opts, "max-evaluations", integrator->maxfuncalls);
  return integrator;
}
