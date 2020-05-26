/*
  timedependentparameters.cc: implementation of the TimeDependentParameters class
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
#include <timedependentparameters.hh>
#include <timedependentparameter.hh>
#include <terminal.hh>
#include <exceptions.hh>
#include <fit.hh>

#include <general-arguments.hh>

TimeDependentParameters::TimeDependentParameters() :
  initialized(false)
{
}

void TimeDependentParameters::clear()
{
  for(iterator i = begin(); i != end(); ++i)
    delete i.value();
  QHash<int, TimeDependentParameter*>::clear();
  parameters.clear();
  parameterNames.clear();
}

void TimeDependentParameters::computeValues(double t, double * target, const double * params) const
{
  if(! initialized)
    throw InternalError("Calling computeValues on uninitialized TimeDependentParameters");
  for(const_iterator i = begin(); i != end(); ++i)
    target[i.key()] = i.value()->computeValue(t, params);
}


QList<ParameterDefinition> TimeDependentParameters::fitParameters() const
{
  return parameters;
}

void TimeDependentParameters::setInitialGuesses(double * target, const DataSet * ds) const
{
  for(const_iterator i = begin(); i != end(); ++i)
    i.value()->setInitialGuess(target, ds);
}

bool TimeDependentParameters::contains(const QString & name) const
{
  return parameterNames.contains(name);
}


Vector TimeDependentParameters::discontinuities(const double * params) const
{
  Vector ret;
  for(const_iterator i = begin(); i != end(); ++i)
    ret << i.value()->discontinuities(params);
  qSort(ret);
  return ret;
}


void TimeDependentParameters::initialize(const double * params)
{
  for(const_iterator i = begin(); i != end(); ++i)
    i.value()->initialize(params);
  initialized = true;
}

void TimeDependentParameters::parseFromStrings(const QStringList & specs, const std::function<int (const QString &)> & indices)
{
  int baseIndex = 0;
  
  for(int i = 0; i < specs.size(); i++) {
    Terminal::out << "Parsing spec: " << specs[i] << endl;
    QStringList s2 = specs[i].split(":");
    if(s2.size() != 2)
      throw RuntimeError("Time-dependent parameter '%1' "
                         "should contain a single :").
        arg(specs[i]);

    TimeDependentParameter * first = NULL;
    QStringList params = s2[0].split(",");
    for(const QString & p : params) {
      int idx = indices(p);
      if(idx < 0)
        throw RuntimeError("Unknown parameter: %1").arg(p);
    
      TimeDependentParameter * param = TimeDependentParameter::parseFromString(s2[1]);
      (*this)[idx] = param;
      param->baseIndex = baseIndex;
      if(first)
        param->baseTDP = first;
      else
        first = param;
      QList<ParameterDefinition> defs = param->parameters(p);
      baseIndex += defs.size();
      parameters += defs;
      parameterNames[p] = idx;
    }
  }
}


//////////////////////////////////////////////////////////////////////

// a bit out of place, but not that much

QString TDPArgument::typeDescription() const
{
  QStringList lst = TimeDependentParameter::TDPFactory::availableItems();
  std::sort(lst.begin(), lst.end());
  return QString("several specifications of [time dependent parameters](#time-dependent-parameters) (like `co:2,exp`), seperated by ';'. Available types: %1").
    arg(lst.join(", "));;
}
