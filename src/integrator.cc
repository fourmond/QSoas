/*
  integrator.cc: implementation of the Integrator class
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
#include <integrator.hh>

#include <exceptions.hh>

#include <general-arguments.hh>
#include <factoryargument.hh>

Integrator::Integrator(double rel, double abs) :
  absolutePrec(abs), relativePrec(rel)
{
}

Integrator * Integrator::createNamedIntegrator(const QString & name,
                                               int intervals,
                                               double relative,
                                               double abs)
{
  return IntegratorFactory::createObject(name, intervals, relative, abs);
}

Integrator::~Integrator()
{
}

QList<Argument *> Integrator::integratorOptions()
{
  QList<Argument *> args;
  args << new FactoryArgument<IntegratorFactory>
    ("integrator", 
     "Integrator",
     "The algorithm used for integration")
       << new NumberArgument("prec-relative", "Relative integration precision",
                             "Relative precision required for integration")
       << new NumberArgument("prec-absolute", "Absolute integration precision",
                             "Absolute precision required for integration");
  return args;
}

Integrator * Integrator::fromOptions(const CommandOptions & opts, 
                                     int maxnum)
{
  IntegratorFactory * c = IntegratorFactory::namedItem("gauss31");
  
  updateFromOptions(opts, "integrator", c);

  double relPrec = 1e-4;
  updateFromOptions(opts, "prec-relative", relPrec);
  double absPrec = 0;
  updateFromOptions(opts, "prec-absolute", absPrec);
  
  return c->creator(maxnum, relPrec, absPrec);
}
