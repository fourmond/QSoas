/**
   \file integrator.hh
   The Integrator class, a one dimensional integrator
   Copyright 2013 by CNRS/AMU

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
#ifndef __INTEGRATOR_HH
#define __INTEGRATOR_HH

#include <factory.hh>
#include <argumentmarshaller.hh>

class Integrator;
/// The factory, with parameters number, relative precision, absolute
/// precision.
typedef Factory<Integrator, int, double, double> IntegratorFactory;


class Argument;

/// This is the base class of various one-dimensional integrators
///
/// @todo Add provisions for integrating a vector over a single
/// variable?
class Integrator {
protected:

  /// Precision: absolute
  double absolutePrec;

  /// relative precision
  double relativePrec;

  /// Number of function calls for the integration
  int funcalls;

  /// The underlying function, set upon 
  std::function<double (double)> fnc;

public:

  static Integrator * createNamedIntegrator(const QString & name = "gauss31",
                                            int intervals = 30,
                                            double relative = 1e-4, 
                                            double abs = 0);

  Integrator(double rel = 1e-4, double abs = 0);

  virtual ~Integrator();

  /// Integrate over a segment
  virtual double integrateSegment(const std::function<double (double)> & f, 
                                  double a, double b, double * error = NULL) = 0;
                          

  /// Returns the number of function calls during the last
  /// integration.
  int functionCalls() const {
    return funcalls;
  };

  /// Returns the number of intervals used for the last computation
  virtual int intervals() const = 0;

  /// Returns a series of arguments suitable to add to commands using
  /// integrators.
  static QList<Argument *> integratorOptions();

  /// Creates an Integrator subclass based on the given options.
  static Integrator * fromOptions(const CommandOptions & opts, int maxnum = 30);
};

#endif
