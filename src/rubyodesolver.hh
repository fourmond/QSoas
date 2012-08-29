/**
   \file rubyodesolver.hh
   Access from Ruby to GSL special mathematical functions
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

#ifndef __RUBYODESOLVER_HH
#define __RUBYODESOLVER_HH

#include <odesolver.hh>
#include <expression.hh>

/// An ODE solver that uses expressions. 
class RubyODESolver : public ODESolver {

  /// Expression called at startup
  Expression * initialization;

  /// Expression that computes the values of the derivatives
  Expression * derivatives;


  QStringList vars;
public:

  /// Builds a solver object.
  RubyODESolver(const QString & init, const QString & derivs);

  virtual ~RubyODESolver();

  virtual int dimension() const;
  virtual int computeDerivatives(double t, const double * y, 
                                 double * dydt);

  /// The variables being integrated (in the order in which they are
  /// found in the values
  const QStringList & variables() const {
    return vars;
  };

  /// Initializes the solver to the (possibly t0-dependent) initial
  /// values.
  void initialize(double t);
};

#endif
