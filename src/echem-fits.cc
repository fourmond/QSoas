/**
   @file echem-fits.cc
   Fits for electrochemists
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

#include <headers.hh>
#include <perdatasetfit.hh>
#include <dataset.hh>
#include <vector.hh>

#include <argumentmarshaller.hh>
#include <commandeffector.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>

#include <soas.hh>

#include <gsl/gsl_const_mksa.h>

/// Nernstian fit
class NernstFit : public FunctionFit {
public:


  /// Formula:
  /// a[1] * exp(-(x - a[0]) / a[2]) + a[3];
  ///
  /// a[0] is a redundant parameter, but it can be of use to shift the
  /// X axis without touching the original data.
  virtual double function(const double * a, 
                          FitData * , double x) {
    double f = GSL_CONST_MKSA_FARADAY /(a[0] * GSL_CONST_MKSA_MOLAR_GAS);
    double e = exp(a[1] * f * (x - a[2]));
    return (a[3] + a[4] * e)/(1 + e);
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = 298;                 /// @todo use global temperature
                                /// settings when applicable
    a[1] = 1;
    a[2] = 0.5*(ds->x().min() + ds->x().max());
    a[3] = ds->y().first();
    a[4] = ds->y().last();
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("temperature", true)
      << ParameterDefinition("n", true)
      << ParameterDefinition("E0")
      << ParameterDefinition("ared")
      << ParameterDefinition("aox");
  };


  NernstFit() : FunctionFit("nernst", 
                            "Nerstian behaviour",
                            "Fit to a Nerst equation") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
NernstFit fit_nernst;
