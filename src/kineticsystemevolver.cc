/*
  kineticsystemevolver.cc: implementation of KineticSystemEvolver
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
#include <kineticsystemevolver.hh>

#include <kineticsystem.hh>
#include <exceptions.hh>

KineticSystemEvolver::KineticSystemEvolver(KineticSystem * sys) :
  system(sys)
{
  QStringList p = system->allParameters();
  for(int i = 0; i < p.size(); i++)
    parameterIndex[p[i]] = i;
  
  parameters = new double[parameterIndex.size()];

  for(int i = 0; i < parameterIndex.size(); i++)
    parameters[i] = 0;          // Default to 0
}

KineticSystemEvolver::~KineticSystemEvolver()
{
  delete[] parameters;
}

int KineticSystemEvolver::dimension() const
{
  return system->speciesNumber();
}

int KineticSystemEvolver::computeDerivatives(double t, const double * y, 
                                             double * dydt)
{
  system->computeDerivatives(dydt, y, parameters);
  return GSL_SUCCESS;
}

QStringList KineticSystemEvolver::setParameters(const QHash<QString, double> & p)
{
  QSet<QString> params = QSet<QString>::fromList(parameterIndex.keys());
  for(QHash<QString, double>::const_iterator i = p.begin();
      i != p.end(); i++) {
    int idx = parameterIndex.value(i.key(), -1);
    if(idx >= 0) {
      parameters[idx] = i.value();
      params.remove(i.key());
    }
  }
  return params.toList();
}
