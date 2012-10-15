/**
   \file kineticsystemevolver.hh 
   Time evolution of a KineticSystem

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
#ifndef __KINETICSYSTEMEVOLVER_HH
#define __KINETICSYSTEMEVOLVER_HH

#include <odesolver.hh>

class KineticSystem;

/// This class computes the concentrations over time of a
/// KineticSystem, for the given parameters.
///
/// @todo Add a template-based system for clever callbacks, to remove
/// the need for the void * parameter. Lambdas would be nice, now that
/// I think of it.
class KineticSystemEvolver : public ODESolver {
  KineticSystem * system;

  double * parameters;

  /// Indices of the parameters
  QHash<QString, int> parameterIndex;

  /// Parameter to the callback
  void * callbackParameter;

  /// A callback called for every time step.
  void (*callback)(double * parameters, double t, void * param);
  
public:
  /// This doesn't take ownership of the kinetic system !
  KineticSystemEvolver(KineticSystem * sys);
  virtual ~KineticSystemEvolver();

  virtual int dimension() const;
  virtual int computeDerivatives(double t, const double * y, 
                                 double * dydt);

  /// Sets the parameters. Returns the list of undefined parameters.
  QStringList setParameters(const QHash<QString, double> & parameters);

  /// Sets the parameters from a double array, in the same order as
  /// they are found. Can skip 1 parameter though
  void setParameters(const double * source, int skip = -1);

  /// Returns the current values of the parameters.
  QHash<QString, double> parameterValues() const;

  /// Initializes the solver. Uses the initial concentrations set
  /// using setParameters()
  void initialize(double tstart);

  /// Sets up a callback, or cancel it, if called with a NULL value.
  void setupCallback(void (*callback)(double *, double, void *), 
                     void * params);

  /// Returns the index of the parameter
  int getParameterIndex(const QString & str) const {
    return parameterIndex.value(str, -1);
  };
};

#endif
