/**
   \file perdatasetfit.hh
   Subclass for Fit for which buffers are independent
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
                        FitData * data, gsl_vector * target);

  /// Redefined to wrap to a call to the per-dataset function
  virtual void initialGuess(FitData * data, double * guess);

  /// Computes the function for a single data set.
  ///
  /// The parameters are those for this single dataset, not the
  /// overall parameters.
  virtual void function(const double * parameters,
                        FitData * data, 
                        const DataSet * ds,
                        gsl_vector * target) = 0;

  /// Provides an initial guess for the given dataset:
  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * guess) = 0;


  PerDatasetFit(const char * n, const char * sd, const char * desc,
      int min = 1, int max = -1, bool mkCmds = true) :
    Fit(n, sd, desc, min, max, mkCmds) { ;};

  virtual ~PerDatasetFit();

};


#endif
