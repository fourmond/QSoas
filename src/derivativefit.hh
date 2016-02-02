/**
   \file derivativefit.hh
   Automatic fits with derivatives...
   Copyright 2012 by CNRS/AMU

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
#ifndef __DERIVATIVEFIT_HH
#define __DERIVATIVEFIT_HH

#include <fit.hh>

class PerDatasetFit;
class Vector;

/// This class handles the simultaneous fitting of a single buffer
/// along with its derivative.
class DerivativeFit : public Fit {
public: 
  typedef enum {
    DerivativeOnly,
    Separated,
    Combined
  } Mode;

protected:

  /// @name Redirectors
  ///
  /// The functions here just redirect to the wrapped fit
  /// 
  /// @{
  virtual void processOptions(const CommandOptions & opts, FitData * data) const;
  virtual QString optionsString(FitData * data) const;
  virtual ArgumentList * fitHardOptions() const;
  virtual ArgumentList * fitSoftOptions() const;
  virtual CommandOptions currentSoftOptions(FitData * data) const;
  virtual void processSoftOptions(const CommandOptions & opts, FitData * data) const;
  /// @}

  virtual void checkDatasets(const FitData * data) const;

  /// The underlying fit
  PerDatasetFit * underlyingFit;


  /// The mode of the fit, ie do we fit the derivative only, the
  /// derivative and the original as separated buffers or combined
  /// into one ?
  Mode mode;


  /// Make sure the buffers are the right size.
  void reserveBuffers(FitData * data) const;

  class Storage : public FitInternalStorage {
  public:
    /// Number of parameters in the original fit
    int originalParameters;

    /// Various buffers for use with the computation of the derivatives
    QList<Vector> buffers;

    /// Storage space of the original fit
    FitInternalStorage * originalStorage;
  };

  virtual FitInternalStorage * allocateStorage(FitData * data) const;
  virtual FitInternalStorage * copyStorage(FitData * data, FitInternalStorage * source, int ds = -1) const;

public:

  virtual QList<ParameterDefinition> parameters(FitData * data) const;
  virtual void function(const double * parameters,
                        FitData * data, gsl_vector * target) const;
  virtual QString annotateDataSet(int idx, FitData * data) const;
  virtual void initialGuess(FitData * data, double * guess) const;



  /// Creates (and registers) the derivative fit based on the given
  /// fit
  DerivativeFit(PerDatasetFit * fit, Mode m = Separated);
  virtual ~DerivativeFit();

};


#endif
