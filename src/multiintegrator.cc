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


MultiIntegrator::~MultiIntegrator()
{
  for(int i = 0; i < evaluations.size(); i++)
    gsl_vector_free(evaluations[i]);
}


gsl_vector * MultiIntegrator::functionForValue(double value)
{
  int idx = evaluationsAt.indexOf(value);
  if(idx < 0) {
    gsl_vector * vct = gsl_vector_alloc(dimension);
    function(value, vct);
    funcalls += 1;
    idx = evaluations.size();
    evaluations << vct;
    evaluationsAt << value;
  }
  return evaluations[idx];
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
                             "Absolute precision required for integration");
  return args;
}

MultiIntegrator * MultiIntegrator::fromOptions(const CommandOptions & opts,
                                               MultiIntegrator::Function fcn, int dimension)
{
  MultiIntegratorFactory * c = MultiIntegratorFactory::namedItem("naive");
  
  updateFromOptions(opts, "integrator", c);

  double relPrec = 1e-4;
  updateFromOptions(opts, "prec-relative", relPrec);
  double absPrec = 0;
  updateFromOptions(opts, "prec-absolute", absPrec);
  int maxc = 0;
  
  return c->creator(fcn, dimension, relPrec, absPrec, maxc);
}
