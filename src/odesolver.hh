/**
   \file odesolver.hh
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

#include <headers.hh>
#ifndef __ODESOLVER_HH
#define __ODESOLVER_HH

#include <argumentmarshaller.hh>

class Vector;
class Argument;

/// Simple wrapper around options for ODEStepper.
class ODEStepperOptions {
public:

  /// Which stepper is used
  const gsl_odeiv2_step_type * type;

  /// Initial step size
  double hStart;

  /// Absolute error
  double epsAbs;

  /// Relative error
  double epsRel;

  /// Whether or not the system uses a fixed step
  bool fixed;

  ODEStepperOptions(const gsl_odeiv2_step_type * t = gsl_odeiv2_step_rkf45,
                    double hs = 0.01, double ea = 0, 
                    double er = 1e-3, bool fixed = false);

  /// Returns a list of commandOptions suitable to add to stepper
  /// functions.
  static QList<Argument*> commandOptions();

  /// Parses the stepper-related options in the given CommandOptions
  void parseOptions(const CommandOptions & opts);

  /// Converts the ODEStepperOptions into plain CommandOptions.
  CommandOptions asOptions() const;

  /// Describes the options
  QString description() const;
};

/// This class embeds a driver object
class ODEStepper {
  
  /// The driver for the integration
  gsl_odeiv2_driver * driver;

  /// The target system
  gsl_odeiv2_system * system;

  /// Frees the driver
  void freeDriver();

  /// The configuration options.
  ODEStepperOptions options;

public:
  ODEStepper();
  ~ODEStepper();
  
  /// Initializes the stepper to work on the target system.
  void initialize(gsl_odeiv2_system * system);

  /// Resets the system (necessary to restart evolution ?)
  void reset();

  /// Steps from *t to t1
  int apply(double * t, const double t1, double y[]);

  /// Configures the stepper
  void setOptions(const ODEStepperOptions & options);

  /// Returns the current options
  const ODEStepperOptions & getOptions() const { return options;};
};

/// This is the base class for all the ODE solver problems. To use it,
/// you must use a derived class and reimplement the dimension() and
/// computeDerivatives() function.
///
///
/// @todo Add automatic jacobian computation ? (don't know if that's
/// that smart...) Possibly using gsl numerical differentiation
/// functions ?
class ODESolver {

  /// Initialize the driver function.
  void initializeDriver();

  /// Frees the driver
  void freeDriver();

  /// The underlying stepper
  ODEStepper stepper;

  /// The current values
  double *yValues;

  /// The current time
  double t;

  /// The function computing the next derivative
  static int function(double t, const double y[], double dydt[], 
                      void * params);

  static int jacobian(double t, const double y[], 
                      double * dfdy,
                      double dydt[], 
                      void * params);


  /// Underlying system
  gsl_odeiv2_system system;

public:

  /// Builds a solver object.
  ///
  /// @todo Add "control" object selection (and parametrization)
  ODESolver();
  virtual ~ODESolver();

  /// Sets the stepper options
  void setStepperOptions(const ODEStepperOptions & opts);

  /// Gets the current stepper options
  const ODEStepperOptions & getStepperOptions();

  /// Returns the dimension of the problem
  virtual int dimension() const = 0;

  /// Resets the stepper
  void resetStepper();

  /// Computes the derivatives at the given point, and store them in
  /// \a dydt
  virtual int computeDerivatives(double t, const double * y, 
                                 double * dydt) = 0;

  /// Resets the solver to the given starting values
  void initialize(const double * yStart, double tstart);

  /// Returns the current Y values
  const double * currentValues() const {
    return yValues;
  };

  /// Returns the current time
  double currentTime() const {
    return t;
  };

  /// Steps to the given time
  ///
  /// This function probably should raise an exception upon GSL error
  void stepTo(double t);

  /// Performs several steps to all the t values, and returns all the
  /// concentrations.
  ///
  /// If @a annotate is true, then a last column is added that will
  /// contain the number of function evaluations needed for each
  /// point. (may prove really useful for debugging !)
  QList<Vector> steps(const Vector & tvalues, bool annotate = false);

  /// The overall number of function evaluations since the beginning.
  int evaluations;
};

#endif
