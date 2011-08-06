/**
   \file fit.hh
   Implementation of fits.
   Copyright 2011 by Vincent Fourmond

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


#ifndef __FIT_HH
#define __FIT_HH

class DataSet;

/// This abstract class defines the interface for handling fits.
class Fit {
protected:

  /// The definition of a parameter.
  class ParameterDefinition {
  public:
    /// Parameter name
    QString name;

    /// If true, then this parameter can be specific to one dataset
    /// (one buffer) instead of being global to all datasets fitted at
    /// the same time.
    bool canBeBufferSpecific;

    ParameterDefinition(const QString & n, bool cbs = true) :
      name(n), canBeBufferSpecific(cbs)
    {
    };
  };

  /// A parameter, once it's in use. A list of that can be used to
  /// convert GSL parameters to dataset-specific parameter values.
  class ActualParameter {
  public:
    /// The index of the parameters from within the Fit::parameters()
    /// return value.
    int paramIndex;

    /// The index of the dataset (-1) for global parameters.
    int dsIndex;

    /// The factor used for derivation (when that applies)
    double derivationFactor;
    
    /// The minimum step used for derivation
    double minDerivationStep;
  };
  

  /// Fit data. This data will be carried around using the void *
  /// argument to the function calls.
  class FitData {
  public:
    /// The fit in use
    Fit * fit;

    /// The datasets holding the data.
    QList<const DataSet *> datasets;

    /// @todo Add informations about the parameters?
  };

  static int staticF(const gsl_vector * x, void * params, gsl_vector * f);
  static int staticDf(const gsl_vector * x, void * params, gsl_matrix * df);
  static int staticFdf(const gsl_vector * x, void * params, gsl_vector * f,
                       gsl_matrix * df);

public:
  
  /// The minimum number of datasets the fit should take.
  virtual int minDataSets() const;

  /// The maximum number of datasets the fit can take (-1 for boundless)
  virtual int maxDataSets() const;

  /// The parameters
  virtual QList<ParameterDefinition> parameters() const;

  /// @name Inner computations
  ///
  /// These functions compute the residuals of the fit, or the
  /// jacobian matrix.
  ///
  /// The function computing the residuals.
  virtual int f(const gsl_vector * parameters, 
                FitData * data, gsl_vector * target_f) = 0;

  virtual int df(const gsl_vector * parameters, 
                 FitData * data, gsl_matrix * target_J) = 0;

  virtual int fdf(const gsl_vector * parameters, 
                  FitData * data, gsl_vector * f, 
                  gsl_matrix * target_J);
  /// @}

  // /// Returns an initial guess based on the data.
  // virtual void initialGuess(FitData * data, gsl_vector * guess);

  /// @todo We need now a function to pop up an interactive dialog box
  /// to review/tune the initial guess, along with (for subclasses)
  /// the selection of the parameters that are bufer-specific and the
  /// ones that are not.

  virtual ~Fit();
};


#endif
