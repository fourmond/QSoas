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
/// @todo Add callback systems for computing the values of parameters
/// at a given point in time.
class KineticSystemEvolver : public ODESolver {
  KineticSystem * system;

  double * parameters;

  /// Indices of the parameters
  QHash<QString, int> parameterIndex;
  
public:
  /// This doesn't take ownership of the kinetic system !
  KineticSystemEvolver(KineticSystem * sys);
  virtual ~KineticSystemEvolver();

  virtual int dimension() const;
  virtual int computeDerivatives(double t, const double * y, 
                                 double * dydt);

  /// Sets the parameters. Returns the list of undefined parameters.
  QStringList setParameters(const QHash<QString, double> & parameters);

  /// Returns the current values of the parameters.
  QHash<QString, double> parameterValues() const;

  /// Initializes the solver. Uses the initial concentrations set
  /// using setParameters()
  void initialize(double tstart);
};

#endif
