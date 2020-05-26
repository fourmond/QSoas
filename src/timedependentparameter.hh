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

#include <factory.hh>

class ParameterDefinition;
class DataSet;
class Vector;

/// This class represents a dependence on time of a parameter. It is
/// the base class of a whole hierarchy
class TimeDependentParameter {
protected:

  friend class TimeDependentParameters;

  /// This is used when one parameter is dependent on another.  When
  /// that is the case, the "time-related" parameters of the this one
  /// are not in fact parameters, but derived from the @a baseTDP
  /// TimeDependentParameter. The target TimeDependantParameter should
  /// have been built with the same 
  ///
  /// The list of the parameters concerned by this is given by the
  /// sharedParameters() function.
  TimeDependentParameter * baseTDP;

  /// Returns the parameters that are shared with the base. The list
  /// MUST be sorted.
  ///
  /// @todo Maybe this should be cached in the initialize function ?
  virtual QList<int> sharedParameters() const;

  /// @name
  ///
  /// All these are the real functions, which are redirected to from
  /// the other series of (public) functions without the @a real
  /// prefix. The redirection is here to process the baseTDP and
  /// sharedParameters() thingy.
  /// 
  /// @{

  /// Performs the initialization. Called from initialize()
  virtual void realInitialize(const double * parameters);
  
  /// The "real" number of parameters
  virtual int realParameterNumber() const = 0;

  virtual QList<ParameterDefinition> realParameters(const QString & prefix) 
    const = 0;

  /// Returns the value at the given time...
  virtual double realComputeValue(double t, const double * parameters) const = 0;

  /// Sets a reasonable initial guess for these parameters
  virtual void realSetInitialGuess(double * parameters, const DataSet * ds) const = 0;
  /// Returns the time at which there are potential discontinuities
  virtual Vector realDiscontinuities(const double * parameters) const = 0;

  /// @}


  /// Splice the parameters, that is copy from @a parameters, which is
  /// assumed to be the normal parameter argument, to @a target, an
  /// array that contains all the argument necessary for this
  /// TimeDependentParameter, including the one derived from
  /// baseTDP. It is 0-based.
  void spliceParameters(const double * parameters, double * target) const;

  /// Put back parameters that do not concern baseTDP into @a
  /// parameters, a parameters-like array.
  void spliceBackParameters(const double * spliced, double * parameters) const;
  
public:

  TimeDependentParameter();
  
  /// The base index of the parameters.
  int baseIndex;

  typedef Factory<TimeDependentParameter, int, const QStringList&> TDPFactory;

  /// Called once before any series of calls to computeValue()
  void initialize(const double * parameters);


  /// The number of parameters
  int parameterNumber() const;

  /// Parameter definitions
  QList<ParameterDefinition> parameters(const QString & prefix) const;

  /// Returns the value at the given time...
  double computeValue(double t, const double * parameters) const;

  /// Sets a reasonable initial guess for these parameters
  void setInitialGuess(double * parameters, const DataSet * ds) const;
  
  /// Returns the time at which there are potential discontinuities
  Vector discontinuities(const double * parameters) const;

  /// Parses a spec for the time-based stuff. Takes a string of the form:
  /// x,type(,....), where x is a number and type 
  static TimeDependentParameter * parseFromString(const QString & str);
  
  virtual ~TimeDependentParameter();
};


#endif
