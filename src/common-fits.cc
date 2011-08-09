/*
  common-fits.cc: various fits of general interest
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

class ExpFit : public FunctionFit {
public:

  /// Formula:
  /// a[1] * exp(-(x - a[0]) / a[2]) + a[3];
  ///
  /// a[0] is a redundant parameter, but it can be of use to shift the
  /// X axis without touching the original data.
  virtual double function(const double * a, 
                          FitData * , double x) {
    return a[1] * exp(-(x - a[0]) / a[2]) + a[3];
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = ds->x()[0];          // x0 = x[0]
    a[3] = ds->y().last();      
    a[1] = ds->y()[0] - a[3];
    a[2] = (ds->x().last() - a[0])/3;
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("x0", true)
      << ParameterDefinition("A")
      << ParameterDefinition("tau")
      << ParameterDefinition("B");
  };


  ExpFit() : FunctionFit("expd", 
                         QT_TR_NOOP("Exponential decay"),
                         QT_TR_NOOP("Exponential decay, formula is :...")) 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
ExpFit fit_expd;
