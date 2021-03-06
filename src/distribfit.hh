/**
   \file distribfit.hh
   Fits with a distribution of parameters
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
#ifndef __DISTRIBFIT_HH
#define __DISTRIBFIT_HH

#include <fit.hh>
#include <perdatasetfit.hh>
#include <expression.hh>

class PerDatasetFit;
class Vector;
class MultiIntegrator;

/// One of the distributions to be used for the fits.
class Distribution {
  static QHash<QString, Distribution *> * distributions;

public:
  QString name;

  /// Returns the parameters to be added to the fit, based on the name
  /// of the parameter.
  virtual QList<ParameterDefinition> parameters(const QString & param) const
  = 0;

  /// Returns the range of the parameter, given the first parameter
  /// pertaining to the distribution (assumed to be contiguous).
  virtual void range(const double * parameters, double * first,
                     double * last) const = 0;

  /// Returns the weight for the point corresponding to the parameter
  /// value x.
  virtual double weight(const double * parameters, double x) const = 0;

  /// Returns the weight of the range (probably quite close to 1) over
  /// the interval described by the parameters.
  virtual double rangeWeight(const double * parameters) const = 0;


  /// Fills the target parameters (starting at the distrib parameters)
  /// with initial guesses, given the value of the central parameter.
  virtual void initialGuess(double * parameters, double value) const = 0;

  /// Offers the possibility to convert the integration parameter into
  /// the one actually used by the fit
  virtual double convertParameter(const double * parameters, double value) const;

  Distribution(const QString & n);

  virtual ~Distribution();

  /// Returns the list of available distributions
  static QStringList availableDistributions();

  /// Returns the named Distribution
  static const Distribution * namedDistribution(const QString & n);
};


/// This class takes a single fit and integrates over the distribution
/// of a single parameter.
class DistribFit : public PerDatasetFit {
protected:

  /// The name of the parameter. Is resolved at fit options time.
  QString parameterName;

  /// The default distribution for the parameter
  const Distribution * defaultDistribution;
  
  /// The underlying fits
  PerDatasetFit * underlyingFit;

  class Storage : public FitInternalStorage {
  public:

    /// Cache for parameter definitions
    QList<ParameterDefinition> parametersCache;

    /// Storage space for the underlying fit
    FitInternalStorage * sub;

    /// The index of the parameter
    int parameterIndex;

    /// The index of the first parameter stuff.
    int distribIndex;

    const Distribution * dist;

    MultiIntegrator * integrator;
    Storage() : sub(NULL), integrator(NULL) {
    };
    ~Storage();    
  };

  virtual FitInternalStorage * allocateStorage(FitData * data) const override;
  virtual FitInternalStorage * copyStorage(FitData * data, FitInternalStorage * source, int ds = -1) const override;
  
  
  virtual void processOptions(const CommandOptions & opts, FitData * data) const override;
  virtual QString optionsString(FitData * data) const override;


  /// Own parameters
  QStringList ownParameters;

public:

  virtual QList<ParameterDefinition> parameters(FitData * data) const override;
  virtual void function(const double * parameters,
                        FitData * data, 
                        const DataSet * ds,
                        gsl_vector * target) const override;

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * guess) const override;

  virtual ArgumentList fitSoftOptions() const override;
  virtual ArgumentList fitHardOptions() const override;
  virtual void processSoftOptions(const CommandOptions & opts, FitData * data) const override;



  DistribFit(const QString & name, const QString & param, 
             PerDatasetFit * fit, const Distribution * defdist = NULL);
  virtual ~DistribFit();

};


#endif
