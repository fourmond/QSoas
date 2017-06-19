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

static TimeDependentParameter::TDPFactory ex("exp", [](int nb, const QStringList & extra) -> TimeDependentParameter * {
    ExponentialTDP * tdp = new ExponentialTDP;
    if(nb <= 0)
      throw RuntimeError("exp parameter needs a strictly positive number, got %1").
        arg(nb);
    tdp->number = nb;
    tdp->independentBits = ! extra.contains("common");
    return tdp;
  }
);

//////////////////////////////////////////////////////////////////////

class StepsTDP : public TimeDependentParameter {
public:

  /// The number of values (i.e. the number of steps + 1);
  int number;

  /// The number of parameters
  int parameterNumber() const {
    return number * 2 - 1;
  };

  /// Parameter definitions
  QList<ParameterDefinition> parameters(const QString & prefix) const {
    QList<ParameterDefinition> ret;
    for(int i = 0; i < number; i++) { 
      if(i > 0)
        ret << ParameterDefinition(QString("%2_t_%1").
                                   arg(i).arg(prefix), true);

      ret << ParameterDefinition(QString("%2_%1").
                                 arg(i).arg(prefix), true);
    }
    return ret;
  };

  /// Returns the value at the given time...
  ///
  /// Assumes the values are increasing.
  double computeValue(double t, const double * parameters) const {
    double value = parameters[baseIndex];
    for(int i = 1; i < number; i++) {
      if(t < parameters[baseIndex + 2*i-1])
        break;
      else
        value = parameters[baseIndex + 2*i];
    }
    return value;
  };

  /// Sets a reasonable initial guess for these parameters
  void setInitialGuess(double * parameters, const DataSet * ds) const {
    double dx = ds->x().max() - ds->x().min();
    for(int i = 0; i < number; i++) {
      double & t0   = parameters[baseIndex + 2*i - 1];
      double & conc = parameters[baseIndex + 2*i];
      conc = 1+i;
      if(i > 0)
        t0 = ds->x().min() + i * dx/(number);
    }
  };

  /// Returns the time at which there are potential discontinuities
  Vector discontinuities(const double * parameters) const {
    Vector ret;
    for(int i = 1; i < number; i++)
      ret << parameters[baseIndex + 2*i - 1];
    return ret;
  };

};

static TimeDependentParameter::TDPFactory steps("steps", [](int nb, const QStringList & /*extra*/) -> TimeDependentParameter * {
    StepsTDP * tdp = new StepsTDP;
    if(nb < 0)
      throw RuntimeError("steps parameter needs a positive number, got %1").
        arg(nb);
    tdp->number = nb+1;
    return tdp;
  }
);

//////////////////////////////////////////////////////////////////////

class RampsTDP : public TimeDependentParameter {
public:

  /// The number of values (i.e. the number of steps + 1);
  int number;

  /// The number of parameters
  int parameterNumber() const {
    return number * 2;
  };

  /// Parameter definitions
  QList<ParameterDefinition> parameters(const QString & prefix) const {
    QList<ParameterDefinition> ret;
    for(int i = 0; i < number; i++) { 
      ret << ParameterDefinition(QString("%2_t_%1").
                                   arg(i).arg(prefix), true);

      ret << ParameterDefinition(QString("%2_%1").
                                 arg(i).arg(prefix), true);
    }
    return ret;
  };

  /// Returns the value at the given time...
  double computeValue(double t, const double * parameters) const {
    double lx = parameters[baseIndex];
    double ly = parameters[baseIndex+1];
    double rx, ry;
    if(t < lx)
      return ly;                 // First y value before the first t
    bool found = false;
    int i = 1;
    do {
      lx = parameters[baseIndex+i*2-2];
      ly = parameters[baseIndex+i*2-1];
      rx = parameters[baseIndex+i*2];
      ry = parameters[baseIndex+i*2+1];
      if(t < rx) {
        found = true;
        break;
      }
      ++i;
    } while(i < number);
    if(! found)
      return ry;                // Last y value after the last t
    return ly + (ry-ly)/(rx-lx)*(t-lx);
  };

  /// Sets a reasonable initial guess for these parameters
  void setInitialGuess(double * parameters, const DataSet * ds) const {
    double dx = ds->x().max() - ds->x().min();
    for(int i = 0; i < number; i++) {
      double & t0   = parameters[baseIndex + 2*i];
      double & conc = parameters[baseIndex + 2*i+1];
      conc = i % 2 ? 1 : -1;
      t0 = ds->x().min() + i * dx/(number-1);
    }
  };

  /// Returns the time at which there are potential discontinuities
  Vector discontinuities(const double * ) const {
    Vector ret;
    return ret;
  };

};

static TimeDependentParameter::TDPFactory ramps("ramps", [](int nb, const QStringList & /*extra*/) -> TimeDependentParameter * {
    RampsTDP * tdp = new RampsTDP;
    if(nb <= 0)
      throw RuntimeError("ramps parameter needs a strictly positive number, got %1").
        arg(nb);
    tdp->number = nb+1;
    return tdp;
  }
);




//////////////////////////////////////////////////////////////////////


/// Like the steps, but with an exponential relaxation at each step.
class ExponentialRelaxationTDP : public TimeDependentParameter {
public:

  /// The number of changes
  int number;

  /// Whether the bits have independent time constants or not
  bool independentBits;

  /// The initial values of each step. Computed at initialization
  Vector initialValues;


  /// The number of parameters
  int parameterNumber() const {
    return 1 + number*2 + (independentBits ? number : 1);
  };

  /// Parameter definitions
  QList<ParameterDefinition> parameters(const QString & prefix) const {
    QList<ParameterDefinition> ret;
    if(! independentBits)
      ret << ParameterDefinition(prefix + "_tau");
    ret << ParameterDefinition(prefix + "_0");
    for(int i = 0; i < number; i++) { 
      ret << ParameterDefinition(QString("%2_t_%1").
                                 arg(i+1).arg(prefix), true);
      ret << ParameterDefinition(QString("%2_%1").
                                 arg(i+1).arg(prefix), true);
      if(independentBits)
        ret << ParameterDefinition(QString("%2_tau_%1").
                                   arg(i+1).arg(prefix));
    }
    return ret;
  };

  inline double t0(const double * parameters, int which) const {
    return parameters[baseIndex + (independentBits ? which*3+1 : 2*which+2)];
  }

  inline double asympt(const double * parameters, int which) const {
    return parameters[baseIndex + (independentBits ? which*3+2 : 2*which+3)];
  }
  
  inline double tau(const double * parameters, int which) const {
    return parameters[baseIndex + (independentBits ? which*3+3 : 0)];

  }

  void initialize(const double * params) {
    initialValues.resize(number);
    double val = params[baseIndex + (independentBits ? 0 : 1)];
    for(int i = 0; i < number; i++) {
      initialValues[i] = val;
      if(i < number - 1) {
        val = (val - asympt(params, i)) *
          exp((t0(params, i) - t0(params, i+1))/tau(params, i))
          + asympt(params, i);
      }
    }
  }

  /// Returns the value at the given time...
  double computeValue(double t, const double * parameters) const {
    int which = -1;
    for(int i = 0; i < number; i++) {
      if(t < t0(parameters, i))
        break;
      else
        which = i;
    }
    if(which == -1)
      return parameters[baseIndex + (independentBits ? 0 : 1)];
    
    double t0 = this->t0(parameters, which);
    double conc = asympt(parameters, which);
    double tau  = this->tau(parameters, which);
    double prev = initialValues[which];

    if(tau < 0)             // Well, the check happens a lot, but
      // is less expensive than an
      // exponential anyway
      throw RangeError("Negative tau value");

    return (prev - conc) * exp(-(t - t0)/tau) + conc;
  };


  /// Sets a reasonable initial guess for these parameters
  void setInitialGuess(double * parameters, const DataSet * ds) const {
    double dx = ds->x().max() - ds->x().min();
    for(int i = 0; i < number; i++) {
      double & t0   = parameters[baseIndex + 1 + (independentBits ? i*3 : 2*i+1)];
      double & conc = parameters[baseIndex + 1 + (independentBits ? i*3+1   : 2*i+2)];
      double & tau  = parameters[baseIndex + 1 + (independentBits ? i*3+2 : -1)];
      tau = dx/(3*number);
      conc = i+2;
      t0 = ds->x().min() + (i+1) * dx/(number+1);
    }
    parameters[baseIndex + (independentBits ? 0 : 1)] = 1;
  };

  /// Returns the time at which there are potential discontinuities
  Vector discontinuities(const double * parameters) const {
    Vector ret;
    for(int i = 0; i < number; i++)
      ret << parameters[baseIndex + (independentBits ? i*3+1 : 2*i+2)];
    return ret;
  };

};


static TimeDependentParameter::TDPFactory rex("rexp", [](int nb, const QStringList & extra) -> TimeDependentParameter * {
    ExponentialRelaxationTDP * tdp = new ExponentialRelaxationTDP;
    if(nb <= 0)
      throw RuntimeError("exp parameter needs a strictly positive number, got %1").
        arg(nb);
    tdp->number = nb;
    tdp->independentBits = ! extra.contains("common");
    return tdp;
  }
);



//////////////////////////////////////////////////////////////////////

TimeDependentParameter * TimeDependentParameter::parseFromString(const QString & str) {
  QStringList elems = str.split(QRegExp("\\s*,\\s*"));
  bool ok;
  int nb = elems[0].toInt(&ok);
  if(ok)
    elems.takeFirst();
  else
    nb = -1;

  if(elems.size() < 1)
    throw RuntimeError("Not a time-dependent-parameter spec: '%1'").
      arg(str);

  QString type = elems.takeFirst();
  return TDPFactory::createObject(type, nb, elems);
} 

TimeDependentParameter::~TimeDependentParameter()
{
}

void TimeDependentParameter::initialize(const double * )
{
}
