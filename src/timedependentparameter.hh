/**
  \file timedependentparameter.hh
  Hierarchy for flexible time-dependent "parameters"
  Copyright 2012, 2013, 2014, 2015 by CNRS/AMU

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
#ifndef __TIMEDEPENDENTPARAMETER_HH
#define __TIMEDEPENDENTPARAMETER_HH

class ParameterDefinition;
class DataSet;
class Vector;

/// This class represents a dependence on time of a parameter. It is
/// the base class of a whole hierarchy
class TimeDependentParameter {
public:
  /// The base index of the parameters. @hack Mutable so that it can
  /// be updated from the const functions.
  int baseIndex;


  /// The number of parameters
  virtual int parameterNumber() const = 0;

  /// Parameter definitions
  virtual QList<ParameterDefinition> parameters(const QString & prefix) 
    const = 0;

  /// Returns the value at the given time...
  virtual double computeValue(double t, const double * parameters) const = 0;

  /// Sets a reasonable initial guess for these parameters
  virtual void setInitialGuess(double * parameters, const DataSet * ds) const = 0;
  /// Returns the time at which there are potential discontinuities
  virtual Vector discontinuities(const double * parameters) const = 0;

  /// Parses a spec for the time-based stuff. Takes a string of the
  /// form number, type, common
  static TimeDependentParameter * parseFromString(const QString & str);
  

  virtual ~TimeDependentParameter();
};


#endif
