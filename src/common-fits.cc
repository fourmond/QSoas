/**
   @file common-fits.cc
   various fits of general interest
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

class FilmExpFit : public FunctionFit {
public:

  /// Formula:
  /// a[1] * exp(-(x - a[0]) / a[2]) + a[3];
  ///
  /// a[0] is a redundant parameter, but it can be of use to shift the
  /// X axis without touching the original data.
  virtual double function(const double * a, 
                          FitData * , double x) {
    return (a[1] * exp(-(x - a[0]) / a[2]) + a[3]) * 
      exp(-a[4] * (x - a[0]));
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = ds->x()[0];          // x0 = x[0]
    a[3] = ds->y().last();      
    a[1] = ds->y()[0] - a[3];
    a[2] = (ds->x().last() - a[0])/3;
    a[4] = 1e-3/(ds->x().last() - a[0]);
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("x0", true)
      << ParameterDefinition("A")
      << ParameterDefinition("tau")
      << ParameterDefinition("B")
      << ParameterDefinition("kloss");
  };


  FilmExpFit() : FunctionFit("film-expd", 
                             "Exponential decay with film loss",
                             "Exponential decay with film loss, formula is :...") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
FilmExpFit fit_film_expd;

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
                         "Exponential decay",
                         "Exponential decay, formula is :...") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
ExpFit fit_expd;

class Exp2Fit : public FunctionFit {
public:

  /// Formula:
  /// a[1] * exp(-(x - a[0]) / a[2]) + a[3] * exp(-(x - a[0]) / a[4]) + a[5];
  ///
  /// a[0] is a redundant parameter, but it can be of use to shift the
  /// X axis without touching the original data.
  virtual double function(const double * a, 
                          FitData * , double x) {
    return a[1] * exp(-(x - a[0]) / a[2]) + a[3] * 
      exp(-(x - a[0]) / a[4]) + a[5];
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = ds->x()[0];          // x0 = x[0]
    a[5] = ds->y().last();      
    a[1] = 0.5*(ds->y()[0] - a[3]);
    a[2] = (ds->x().last() - a[0])/20;
    a[3] = a[1];
    a[4] = (ds->x().last() - a[0])/3;
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("x0", true)
      << ParameterDefinition("A1")
      << ParameterDefinition("tau1")
      << ParameterDefinition("A2")
      << ParameterDefinition("tau2")
      << ParameterDefinition("B");
  };


  Exp2Fit() : FunctionFit("expd2", 
                          "Bi-exponential decay",
                          "Bi-exponential decay, formula is :...") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
Exp2Fit fit_expd2;


class FilmExp2Fit : public FunctionFit {
public:

  /// Formula:
  /// a[1] * exp(-(x - a[0]) / a[2]) + a[3] * exp(-(x - a[0]) / a[4]) + a[5];
  ///
  /// a[0] is a redundant parameter, but it can be of use to shift the
  /// X axis without touching the original data.
  virtual double function(const double * a, 
                          FitData * , double x) {
    return (a[1] * exp(-(x - a[0]) / a[2]) + a[3] * 
            exp(-(x - a[0]) / a[4]) + a[5]) * 
      exp(-a[6] * (x - a[0]));
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    a[0] = ds->x()[0];          // x0 = x[0]
    a[5] = ds->y().last();      
    a[1] = 0.5*(ds->y()[0] - a[3]);
    a[2] = (ds->x().last() - a[0])/20;
    a[3] = a[1];
    a[4] = (ds->x().last() - a[0])/3;
    a[6] = 1e-3/(ds->x().last() - a[0]);
  };

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("x0", true)
      << ParameterDefinition("A1")
      << ParameterDefinition("tau1")
      << ParameterDefinition("A2")
      << ParameterDefinition("tau2")
      << ParameterDefinition("B")
      << ParameterDefinition("kloss");
  };


  FilmExp2Fit() : FunctionFit("film-expd2", 
                              "Bi-exponential decay with film loss",
                              "Bi-exponential decay with film loss, formula is :...") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
FilmExp2Fit fit_film_expd2;


/// @todo Ideas:
/// @li fits with arbitrary number of exponentials (using a number
/// parameter) ?
/// @li polynomial fits ?
