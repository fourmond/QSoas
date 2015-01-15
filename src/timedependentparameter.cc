/*
  timedependentparameter.cc: implementation of the TimeDependentParameter class (and its children)
  Copyright 2014 by CNRS/AMU

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
#include <timedependentparameter.hh>

#include <fit.hh>
#include <vector.hh>
#include <dataset.hh>

class ExponentialTDP : public TimeDependentParameter {
public:

  /// The number of bits
  int number;

  /// Whether the bits have independent accessory parameters or not
  bool independentBits;

  /// The number of parameters
  int parameterNumber() const {
    return number*2 + (independentBits ? number : 1);
  };

  /// Parameter definitions
  QList<ParameterDefinition> parameters(const QString & prefix) const {
    QList<ParameterDefinition> ret;
    if(! independentBits)
      ret << ParameterDefinition(prefix + "_tau");
    for(int i = 0; i < number; i++) { 
      ret << ParameterDefinition(QString("%2_%1").
                                 arg(i+1).arg(prefix), true);
      ret << ParameterDefinition(QString("%2_t_%1").
                                 arg(i+1).arg(prefix), true);
      if(independentBits)
        ret << ParameterDefinition(QString("%2_tau_%1").
                                   arg(i+1).arg(prefix));
    }
    return ret;
  };

  /// Returns the value at the given time...
  double computeValue(double t, const double * parameters) const {
    double value = 0;
    for(int i = 0; i < number; i++) {
      double t0   = parameters[baseIndex + (independentBits ? i*3+1 : 2*i+2)];
      double conc = parameters[baseIndex + (independentBits ? i*3   : 2*i+1)];
      double tau  = parameters[baseIndex + (independentBits ? i*3+2 : 0)];
      if(tau < 0)             // Well, the check happens a lot, but
        // is less expensive than an
        // exponential anyway
        throw RangeError("Negative tau value");
      if(t >= t0)
        value += conc * exp(-(t - t0)/tau);
    }
    return value;
  };

  /// Sets a reasonable initial guess for these parameters
  void setInitialGuess(double * parameters, const DataSet * ds) const {
    double dx = ds->x().max() - ds->x().min();
    for(int i = 0; i < number; i++) {
      double & t0   = parameters[baseIndex + (independentBits ? i*3+1 : 2*i+2)];
      double & conc = parameters[baseIndex + (independentBits ? i*3   : 2*i+1)];
      double & tau  = parameters[baseIndex + (independentBits ? i*3+2 : 0)];
      tau = 20;
      conc = 1;
      t0 = ds->x().min() + (i+1) * dx/(number+1);
    }
  };

  /// Returns the time at which there are potential discontinuities
  Vector discontinuities(const double * parameters) const {
    Vector ret;
    for(int i = 0; i < number; i++)
      ret << parameters[baseIndex + (independentBits ? i*3+1 : 2*i+2)];
    return ret;
  };

};


  
TimeDependentParameter * TimeDependentParameter::parseFromString(const QString & str) {
    
  QRegExp parse("^\\s*(\\d+)\\s*,\\s*(\\w+)\\s*(,\\s*common)?\\s*$");
  if(parse.indexIn(str) != 0)
    throw RuntimeError(QString("Invalid specification for time "
                               "dependence: '%1'").arg(str));
  if(parse.cap(2) == "exp") {
    ExponentialTDP * tdp = new ExponentialTDP;
    tdp->number = parse.cap(1).toInt();
    tdp->independentBits = parse.cap(3).isEmpty();
    return tdp;
  }

  throw RuntimeError(QString("Invalid type of time "
                             "dependence: '%1'").arg(parse.cap(2)));
  return NULL;
};

TimeDependentParameter::~TimeDependentParameter()
{
}
