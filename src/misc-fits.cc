/**
   @file exponential-fits.cc
   Exponential-based fits.
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

#include <terminal.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <soas.hh>

#include <fitdata.hh>
#include <fitdialog.hh>

#include <gsl/gsl_const_mksa.h>

class SlowScanFit : public PerDatasetFit {
public:


  /// Formula:
  virtual void function(const double * a, FitData * , 
                        const DataSet * ds , gsl_vector * target) {
    
    double fara = GSL_CONST_MKSA_FARADAY /(a[0] * GSL_CONST_MKSA_MOLAR_GAS);
    double D = pow(10, a[4]);

    const Vector & xv = ds->x();
    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      double arg = exp(-a[5]*fara*x) - exp(-a[5]*fara*xv[0]);
      gsl_vector_set(target, i, 
                     (1 - a[3] * exp(-D * arg) )
                     *(a[1] * x + a[2]));
    }
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ,
                            double * a)
  {
    a[0] = soas().temperature();
    a[1] = 1; a[2] = 1; a[4] = 1; a[5] = 1;
    a[3] = 0.5;
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("temperature", true)
      << ParameterDefinition("a")
      << ParameterDefinition("b")
      << ParameterDefinition("c")
      << ParameterDefinition("log(D)")
      << ParameterDefinition("alpha");
  };


  SlowScanFit() : PerDatasetFit("slow-scan", 
                                "Slow scan test",
                                "Slow scan") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
SlowScanFit fit_slow_scan;
