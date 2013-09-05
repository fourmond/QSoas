/**
   \file kineticsystemsteadystate.hh 
   (potential-dependent) steady-state of a KineticSystem

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
#ifndef __KINETICSYSTEMSTEADYSTATE_HH
#define __KINETICSYSTEMSTEADYSTATE_HH

#include <msolver.hh>

class KineticSystem;
class Vector;

/// Solves the steady state
class KineticSystemSteadyState : public MSolver {
  KineticSystem * system;

  /// This contains the values of all the parameters.
  double * parameters;

  /// The name of the parameters, as returned by
  /// KineticSystem::allParameters()
  QStringList parameterNames;

  /// Index of the temperature
  int tempIndex;

  /// Index of the total concentration
  int tcIndex;

  /// Index of the potential
  int potIndex;

public:
  /// This doesn't take ownership of the kinetic system !
  KineticSystemSteadyState(KineticSystem * sys);
  virtual ~KineticSystemSteadyState();

  virtual int dimension() const;
  virtual int f(const gsl_vector * x,
                gsl_vector * tg);

  /// Sets the parameters.
  void setParameters(const QString & str);

  /// Solves the steady-state and returns the list of
  /// concentrations. 
  QList<Vector> computeConcentrations(const Vector & potentials);

};

#endif
