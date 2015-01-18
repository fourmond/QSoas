/**
   \file rubyodesolver.hh
   ODE based on ruby expressions
   Copyright 2012, 2013, 2014 by CNRS/AMU

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
#ifndef __RUBYODESOLVER_HH
#define __RUBYODESOLVER_HH

#include <odesolver.hh>
#include <expression.hh>
#include <vector.hh>

/// An ODE solver that uses expressions.
class RubyODESolver : public ODESolver {

  /// Expression called at startup
  Expression * initialization;

  /// Expression that computes the values of the derivatives
  Expression * derivatives;

  /// Expression that computes the values we are interested in. Can be
  /// null, in which case the reporterValues() function returns the values of
  /// the variables.
  Expression * reporters;


  /// The variables that are being integrated
  QStringList vars;

  /// The list of the extra parameters needed for computing all the
  /// values (initial and derivatives)
  QStringList extraParams;

  /// Values for the extra parameters
  Vector extraParamsValues;

  /// A callback called for every time step.
  std::function <void (double, double *)> callback;

public:

  /// Builds a solver object.
  RubyODESolver(const QString & init, const QString & derivs);

  /// Builds a systemless solver that must be further initialized
  /// using parseSystem() or parseFromFile()
  RubyODESolver();

  /// Parses the given equations into the corresponding expressions.
  void parseSystem(const QString & initialConditions, 
                   const QString & derivatives,
                   const QString & reporters = QString());

  /// Parses the system from a file. A blank line separates the
  /// initial conditions from the derivatives.
  void parseFromFile(QIODevice * device);

  /// Overridden version that takes a file name.
  void parseFromFile(const QString & file);

  /// Sets up a callback
  void setupCallback(const std::function <void (double, double *)> & cb);

  
  virtual bool hasReporters() const {
    return reporters != 0;
  };

  /// Returns the set of values defined by the reporters expression --
  /// or the values of the variables should there not be any
  /// expression.
  virtual Vector reporterValues() const;

  virtual ~RubyODESolver();

  virtual int dimension() const;
  virtual int computeDerivatives(double t, const double * y, 
                                 double * dydt);

  /// The variables being integrated (in the order in which they are
  /// found in the values
  const QStringList & variables() const {
    return vars;
  };

  /// returns the extra parameters list
  const QStringList & extraParameters() const {
    return extraParams;
  };

  /// Initializes the solver to the (possibly t0-dependent) initial
  /// values.
  void initialize(double t);


  /// Sets extra parameters from the given ruby expression (that
  /// doesn't involve anything but the parameters themselves and
  /// constants).
  void setParameterValues(const QString & formula);

  
  /// Sets extra parameters from the given array. The elements of the
  /// \a skippedIndices set are skipped from the target.
  void setParameterValues(const double * source, const QSet<int> & skipped);

  /// Returns the values of the extra parameters
  const Vector & parameterValues() const {
    return extraParamsValues;
  };

  
  /// Dumps the contents of the object to the target stream
  void dump(QTextStream & target) const;

  /// Returns explanative text as QString
  QString dump() const;
};

#endif
