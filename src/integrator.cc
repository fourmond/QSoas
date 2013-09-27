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




