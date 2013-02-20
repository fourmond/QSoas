/**
   \file combinedfit.hh
   Combine several fits into one.
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
#ifndef __COMBINEDFIT_HH
#define __COMBINEDFIT_HH

#include <fit.hh>
#include <perdatasetfit.hh>
#include <expression.hh>

class PerDatasetFit;
class Vector;

/// This class combines a whole variety of fits into a single one,
/// using a mathematical formula.
///
/// It only applies to per-dataset fits, because extending to more
/// complex fits would involve splicing all the parameters.
///
/// The formula uses y1, y2 and so on to designate the various
/// fits. Also incorporates the same variables as custom fits...
class CombinedFit : public PerDatasetFit {
protected:
  virtual void processOptions(const CommandOptions & opts);
  virtual QString optionsString() const;

  /// The underlying fits
  QList<PerDatasetFit *> underlyingFits;

  /// Various buffers for use with the computation of the derivatives
  QList<Vector> buffers;

  /// Internal storage for the twisted parameters (they are renamed in
  /// order to avoid clashes between fits)
  QList<ParameterDefinition> overallParameters;

  /// An array of the same size as underlyingFits containing the index
  /// of the first parameter of each fit with respect to the overall
  /// parameters.
  QList<int> firstParameterIndex;

  /// The expression used to combine the fits. It incorporates y1,
  /// y2... and so on to designate each of the fits.
  Expression formula;

  /// Own parameters
  QStringList ownParameters;

  /// Make sure the buffers are the right size.
  void reserveBuffers(const DataSet * ds);


  /// @name Parameter splicing
  ///
  /// These bits ensure the conversion between per-fit parameters and
  /// the parameters as they are provided to CombinedFit by the fit
  /// engine.
  ///
  /// Many of these functions deal with converting parameters from a
  /// place where all parameters for all buffers in one fit are
  /// consecutive (internal representation, referred to a spliced) to
  /// one where all are spliced (ie all parameters for one dataset are
  /// consecutive).
  ///
  /// @{

  // /// Returns the base index for all the parameters of the fit \a i
  // /// for the "all parameters for one fit consecutive" representation.
  // int splicedIndex(FitData * data, int fit) const;

  /// Splice the external parameters into the internal representation.
  void spliceParameters(FitData * data, double * target, 
                        const double * source);


  /// Unsplice the internal representation back into external
  /// representation.
  void unspliceParameters(FitData * data, double * target, 
                          const double * source);

  /// @}

public:

  virtual QList<ParameterDefinition> parameters() const;
  virtual void function(const double * parameters,
                        FitData * data, 
                        const DataSet * ds,
                        gsl_vector * target);

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * guess);



  CombinedFit(const QString & name, const QString & formula, 
              QList<PerDatasetFit *> fits);
  virtual ~CombinedFit();

};


#endif
