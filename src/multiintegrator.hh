/**
   \file multiintegrator.hh
   The MultiIntegrator class, to integrate many functions of ONE value
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
#ifndef __MULTIINTEGRATOR_HH
#define __MULTIINTEGRATOR_HH

#include <factory.hh>
#include <argumentmarshaller.hh>


class MultiIntegrator;


class Argument;

class WrappedDouble {
public:
  double value;
  bool operator==(const WrappedDouble & other) const {
    return other.value == value;
  };
  WrappedDouble(const double & v) : value(v) {
  };
};

/// This is the base class for integrators that integrated many
/// functions of a single variable. It is very much like the 
class MultiIntegrator {

  /// The internal storage space for function evaluations
  QHash<WrappedDouble, gsl_vector *> evaluations;

  /// Frees the space taken by all the evaluations.
  void clearEvaluations();
protected:

  /// Precision: absolute
  double absolutePrec;

  /// relative precision
  double relativePrec;

  /// Number of function calls for the integration
  int funcalls;

  /// An approximative upper bound to the number of funcalls. The
  /// algorithm should probably interrupt the computation when the
  /// funcall number exceeds this value.
  ///
  /// No limits if 0.
  int maxfuncalls;

  /// Called by reset() to handle whatever it is that sub-classes must
  /// handle upon reset.
  virtual void internalReset();

public:

  typedef std::function<void (double, gsl_vector * tgt)> Function;

  /// The factory, with parameters number, relative precision, absolute
  /// precision.
  typedef Factory<MultiIntegrator, Function, int, double, double, int> MultiIntegratorFactory;

  void reset(Function fcn, int dim);

protected:

  /// The underlying function
  Function function;


  /// The dimension of the problem (ie the number of functions we're
  /// integrating)
  int dimension;

  /// Returns the function for the given value, evaluating it if
  /// necessary.
  gsl_vector * functionForValue(double value);

public:

  MultiIntegrator(Function fnc, int dim, double rel = 1e-4, double abs = 0, int maxc = 0);

  /// Integrates over the given segment. Returns the sum of all the
  /// errors over all the points. The result is stored in the res
  /// segment.
  virtual double integrate(gsl_vector * res, double a, double b) = 0;

  /// Returns the number integrator.
  static MultiIntegrator * createNamedIntegrator(const QString & name,
                                                 Function fnc,
                                                 int dimension,
                                                 double relative = 1e-4, 
                                                 double abs = 0,
                                                 int max = 0);

  /// Returns a series of arguments suitable to add to commands using
  /// integrators.
  static QList<Argument *> integratorOptions();

  /// Creates an Integrator subclass based on the given options.
  static MultiIntegrator * fromOptions(const CommandOptions & opts, Function fcn, int dimension);

  /// Returns the number of evaluations thus far.
  int evaluationNumbers() const {
    return funcalls;
  };

  /// Duplicates the integrator - returns a newly allocated integrator
  /// with the same stuff...
  virtual MultiIntegrator * dup() const = 0;

  virtual ~MultiIntegrator();

};

#endif
