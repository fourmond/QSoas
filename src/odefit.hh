/**
  \file odefit.hh
  An abstract class for ODESolver-based fits
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
#ifndef __ODEFIT_HH
#define __ODEFIT_HH

#include <perdatasetfit.hh>
#include <timedependentparameters.hh>

class ODESolver;

/// A fit based on solving ODEs. 
class ODEFit : public PerDatasetFit {
protected:

  /// @name Interface
  ///
  /// Functions that must be implemented by children. 
  ///
  /// @{
  
  /// The function to be reimplemented by the children giving the base
  /// class access to the solver object.
  virtual ODESolver * solver(FitData * data) const = 0;

  /// A function returning the index of a named "parameter" (formula
  /// parameter and not fit parameter), for use with
  /// TimeDependentParameters.
  virtual int getParameterIndex(const QString & name, FitData * data) const = 0;

  /// Returns the list of all the system parameters.
  virtual QStringList systemParameters(FitData * data) const = 0;

  /// Returns the name of the variables (ie the things whose
  /// dependence over time the solver() is computing)
  virtual QStringList variableNames(FitData * data) const = 0;

  /// Whether the named parameter should be fixed by default
  virtual bool isFixed(const QString & name, FitData * data) const;


  /// Initializes the system at the given time t and using the given
  /// parameters.
  ///
  /// This should call the solver.
  virtual void initialize(double t0, const double * params, FitData * data) const = 0;

  /// Whether the solver has reporters or not
  virtual bool hasReporters(FitData * data) const;

  /// Returns the value of reporters
  virtual double reporterValue(FitData * data) const;

  /// Setups a callback that should be called at each time, and will be given
  /// * the time
  /// * the system parameters (to be modified)
  virtual void setupCallback(const std::function<void (double, double * )> & cb) const = 0;

  /// @}

  class Storage : public FitInternalStorage {
  public:
  
    /// The time dependent parameters of the fit.
    TimeDependentParameters timeDependentParameters;


    bool voltammogram;
    double lastTime;
    double lastPot;
    double direction;

    /// Whether or not a different time origin has been specified.
    bool hasOrigTime;

    /// The variables related to the position of solver's paremeters
    /// within the fit paratemers

    /// The index of the first parameter
    int parametersBase;

    /// The set of indices which should be skipped while reading from
    /// the fit parameters + parametersBase to fill the target system
    /// parameters.
    QSet<int> skippedIndices;

    /// The base index for the TimeDependentParameters
    int tdBase;

    int timeIndex;

  
    int potentialIndex;
    int faraIndex;
    int temperatureIndex;
    int parametersNumber;

    /// A cache for parameters
    QList<ParameterDefinition> parametersCache;
  };

  virtual void processOptions(const CommandOptions & opts, FitData * data) const;

  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const {
    return deepCopy<Storage>(source);
  };




  /// Updates the parameters cache.
  void updateParameters(FitData * data) const;
  

public:

  virtual CommandOptions currentSoftOptions(FitData * data) const;
  
  /// Process the soft options, i.e. the stepper options
  virtual void processSoftOptions(const CommandOptions & opts, FitData * data) const;

  virtual QList<ParameterDefinition> parameters(FitData * data) const;
  
  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const;

  virtual ArgumentList * fitSoftOptions() const;

  virtual ArgumentList * fitHardOptions() const;


  ODEFit(const QString & n, const QString & sd, const QString & desc,
         int min = 1, int max = -1, bool mkCmds = true) :
    PerDatasetFit(n, sd, desc, min, max, mkCmds) {
  };

  virtual ~ODEFit();
};





#endif
