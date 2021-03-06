/**
   \file perdatasetfit.hh
   Subclass for Fit for which buffers are independent
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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
#ifndef __PERDATASETFIT_HH
#define __PERDATASETFIT_HH

#include <fit.hh>


/// This abstract subclass of Fit is a convenience wrapper for fits in
/// which buffer are completely independent, and hence all get the
/// same treatment.
class PerDatasetFit : public Fit {
public:
  /// Redefined to wrap to a call to the per-dataset function
  virtual void function(const double * parameters,
                        FitData * data, gsl_vector * target) const override;

  /// Reimplemented for performance :
  virtual void functionForDataset(const double * parameters,
                                  FitData * data, gsl_vector * target, 
                                  int dataset) const override;

  /// Redefined to wrap to a call to the per-dataset function
  virtual void initialGuess(FitData * data, double * guess) const override;

  /// Computes the function for a single data set.
  ///
  /// The parameters are those for this single dataset, not the
  /// overall parameters.
  virtual void function(const double * parameters,
                        FitData * data, 
                        const DataSet * ds,
                        gsl_vector * target) const = 0;

  /// Defined to give nothing by default, as it doesn't make any sense
  /// for these kinds of fits.
  virtual QString annotateDataSet(int idx, FitData * data) const override;


  /// Provides an initial guess for the given dataset:
  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * guess) const = 0;

  PerDatasetFit(const QString & n, const QString & sd, const QString & desc,
      int min = 1, int max = -1, bool mkCmds = true) :
    Fit(n, sd, desc, min, max, mkCmds) { ;};

  virtual ~PerDatasetFit();


  /// Reimplementation that calls the computeSubFunctions()
  /// virtual function.
  /// @todo Should I use final ?
  virtual void computeSubFunctions(const double * parameters,
                                   FitData * data, 
                                   QList<Vector> * targetData,
                                   QStringList * targetAnnotations) const override;

  virtual void computeSubFunctions(const double * parameters,
                                   FitData * data, 
                                   const DataSet * ds,
                                   QList<Vector> * targetData,
                                   QStringList * targetAnnotations) const;

};

/// This abstract class handles the very simple but also very common
/// case when the function to be fitted can be simply expressed as a
/// function of X.
class FunctionFit : public PerDatasetFit {
public:
  virtual void function(const double * parameters,
                        FitData * data, 
                        const DataSet * ds,
                        gsl_vector * target) const override;

  /// Computes the Y value for the given \a x.
  virtual double function(const double * parameters, 
                          FitData * data, double x) const = 0;

  /// Prepare internal parameters for a given dataset. Called at the
  /// beginning of each evaluation over a given dataset.
  virtual void prepare(const double * parameters, FitData * data, 
                       const DataSet * ds) const;

  FunctionFit(const QString &n, const QString &sd, const QString &desc,
      int min = 1, int max = -1, bool mkCmds = true) :
    PerDatasetFit(n, sd, desc, min, max, mkCmds) { ;};

};


#endif
