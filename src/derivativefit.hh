/**
   \file derivativefit.hh
   Automatic fits with derivatives...
   Copyright 2012, 2016 by CNRS/AMU

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
#include <vector.hh>

class PerDatasetFit;

/// This class handles the simultaneous fitting of a single buffer
/// along with its derivative.
///
/// @todo It's rather ugly that we have two classes that do almost the
/// same thing. Maybe shared inheritance would be a good thing.
///
/// @todo This code shouldn't be in a header file, it's purely
/// internal.
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
  virtual void processOptions(const CommandOptions & opts,
                              FitData * data) const override;
  virtual QString optionsString(FitData * data) const override;
  virtual ArgumentList fitHardOptions() const override;
  virtual ArgumentList fitSoftOptions() const override;
  virtual CommandOptions currentSoftOptions(FitData * data) const override;
  virtual void processSoftOptions(const CommandOptions & opts,
                                  FitData * data) const override;

  /// @}

  virtual void checkDatasets(const FitData * data) const override;


  /// Name of the underlying fit.
  QString underlyingFitName;


  /// The mode of the fit, ie do we fit the derivative only, the
  /// derivative and the original as separated buffers or combined
  /// into one ?
  Mode mode;


  /// Make sure the buffers are the right size.
  void reserveBuffers(FitData * data) const;

public:

  /// This class is also used by CombinedDerivativeFit
  class Storage : public FitInternalStorage {
  public:
    /// Number of parameters in the original fit
    int originalParameters;

    /// Various buffers for use with the computation of the derivatives
    QList<Vector> buffers;

    /// Subdatasets used for the combined-fits:
    /// -even numbered ones are the original functions
    /// -odd numbered ones are the derivatives
    QList<DataSet*> splittedDatasets;

    /// In combined mode, whether a given dataset has the same X
    /// values for the derivative and the original data.
    bool * sameX;

    Vector buffer;

    /// In combined mode, whether a given dataset has the same X
    /// values for the derivative and the original data.
    QHash<const DataSet *, bool> sameXH;

    QHash<const DataSet *, DataSet *> mainDS;

    QHash<const DataSet *, DataSet *> subDS;


    /// Storage space of the original fit
    FitInternalStorage * originalStorage;

    /// The underlying fit
    PerDatasetFit * underlyingFit;


    Storage();
    Storage(const Storage & o);
    ~Storage();
  };

protected:
  
  virtual FitInternalStorage * allocateStorage(FitData * data) const override;
  virtual FitInternalStorage * copyStorage(FitData * data, FitInternalStorage * source, int ds = -1) const override;

public:

  virtual QList<ParameterDefinition> parameters(FitData * data) const override;
  virtual void function(const double * parameters,
                        FitData * data, gsl_vector * target) const override;
  virtual QString annotateDataSet(int idx, FitData * data) const override;
  virtual void initialGuess(FitData * data, double * guess) const override;

  bool threadSafe() const override;


  /// Creates (and registers) the derivative fit based on the given
  /// fit
  DerivativeFit(PerDatasetFit * fit, Mode m = Separated);
  virtual ~DerivativeFit();

  static QString derivativeFitName(PerDatasetFit * source,
                                   DerivativeFit::Mode mode);

  // // subfunctions forwarding.
  // virtual bool hasSubFunctions(FitData * data) const;
  // virtual bool displaySubFunctions(FitData * data) const;
  // virtual void computeSubFunctions(const double * parameters,
  //                                  FitData * data, 
  //                                  QList<Vector> * targetData,
  //                                  QStringList * targetAnnotations) const;


};


#endif
