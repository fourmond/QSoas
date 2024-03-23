/**
   @file distrib-fits.cc
   GSL Distribution-based fits
   Copyright 2013 by CNRS/AMU

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
#include <functions.hh>

#include <gsl/gsl_randist.h>

/// Base class for all distribution-based fits
/// @todo make it possible to use a common "sigma" parameter ?
/// @todo use peak information for initial values.
template <class... Args>
class DistributionFit : public PerDatasetFit {

  class Storage : public FitInternalStorage {
  public:
    /// The number of different distributions
    int number;

    /// Whether the amplitude or the surface is used for each 'peak'.
    bool useSurface;

  };

  /// Names of the accessory parameters
  QStringList accessoryNames;

  typedef double (*Function)(Args...);

  Function func;
  
  

protected:

  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const override {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const override {
    return deepCopy<Storage>(source);
  };

  virtual void processOptions(const CommandOptions & opts, FitData * data) const override
  {
    Storage * s = storage<Storage>(data);

    s->useSurface = false;
    s->number = 1;
    updateFromOptions(opts, "use-surface", s->useSurface);
    updateFromOptions(opts, "number", s->number);
  }

  
  virtual QString optionsString(FitData * data) const override {
    Storage * s = storage<Storage>(data);
    return QString("%1 species, %2").arg(s->number).
      arg(s->useSurface ? "surface" : "amplitude");
  }

  /// Returns the parameters base for the nth peak
  int paramBase(int idx) const {
    return 1 + (sizeof...(Args)+1) * idx;
  }

  /// Runs the function using the given position, the given params and
  /// the given index. The position is RELATIVE TO THE PEAK CENTER
  double fn(double x, const double * a, int idx) const;
  double fn(double x, const double * a) const;


  
  // /// Override for 3 args
  // double fn(double x, const double * a, int idx) {
  //   const double & p1 = a[paramBase(idx) + 2];
  //   const double & p2 = a[paramBase(idx) + 3];
  //   return func(x, p1, p2);
  // }

  // double fn(double x, const double * a) {
  //   return func(x, a[0], a[1]);
  // }

  void annotatedFunction(const double * a, FitData * data, 
                         const DataSet * ds , gsl_vector * target,
                         QList<Vector> * annotations = NULL) const
  {
    Storage * s = storage<Storage>(data);
        
    const Vector & xv = ds->x();

    double prefactors[s->number];
    for(int j = 0; j < s->number; j++)
      prefactors[j] = a[paramBase(j) + 1] /
        (s->useSurface ? 1 : fn(0, a, j));

    for(int i = 0; i < xv.size(); i++) {
      double cur = a[0];
      const double & x = xv[i];
      for(int j = 0; j < s->number; j++) {
        // Assignments for readability, I hope they're optimized out.
        const double & x0 = a[paramBase(j)];
        const double & pref = prefactors[j];
        double val = pref * fn(x - x0, a, j);
        if(annotations)
          (*annotations)[j][i] = a[0] + val;
        cur += val;
      }
      if(target)
        gsl_vector_set(target, i, cur);
    }
  };




public:

  virtual bool hasSubFunctions(FitData * data) const override {
    Storage * s = storage<Storage>(data);

    return s->number > 1;
  };

  virtual bool displaySubFunctions(FitData * ) const override {
    return true;               
  };

  virtual void computeSubFunctions(const double * parameters,
                                   FitData * data, 
                                   const DataSet * ds,
                                   QList<Vector> * targetData,
                                   QStringList * targetAnnotations) const override
  {
    Storage * s = storage<Storage>(data);
    targetData->clear();
    targetAnnotations->clear();

    int sz = ds->x().size();
    for(int i = 0; i < s->number; i++) {
      (*targetData) << Vector(sz, 0);
      *targetAnnotations << QString("Peak %1").arg(i+1);
    }
    
    annotatedFunction(parameters, data, ds, NULL,
                      targetData);
  };


  /// Formula:
  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const override {
    annotatedFunction(a, data, ds, target);
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet *ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);

    // For now, equally distributed ?

    /// @todo Use peak info ?
    const double xmin = ds->x().min();
    const double xmax = ds->x().max();

    const double ymin = ds->y().min();
    const double ymax = ds->y().max();

    double *t = a;
    *t = ymin;
    double params[sizeof...(Args) - 1];
    params[0] = (xmax - xmin)/(s->number * 4);
    for(int k = 1; k < sizeof...(Args) - 1; k++)
      params[k] = 0.5;
    for(int i = 0; i < s->number; i++) {
      *(++t) = xmin + (i+0.5) * (xmax - xmin)/s->number; // position
      *(++t) = (ymax - ymin) / (s->useSurface ? fn(0,params) : 1); // amplitude
      for(int k = 0; k < sizeof...(Args) - 1; k++)
        *(++t) = params[k];
    }
  };

  virtual QList<ParameterDefinition> parameters(FitData * data) const override {
    Storage * s = storage<Storage>(data);

    QList<ParameterDefinition> params;
    params << ParameterDefinition("Y0",
                                  false, true, true); // base line
    for(int j = 0; j < s->number; j++) {
      params << ParameterDefinition(QString("x_%1").arg(j+1))
             << ParameterDefinition(QString("%2_%1").arg(j+1).
                                    arg(s->useSurface ? "S" : "A"),
                                    false, true, true);
      for(int k = 0; k < accessoryNames.size(); k++) {
        params << ParameterDefinition(QString("%2_%1").arg(j+1).
                                      arg(accessoryNames[k]));
      }
    }

    return params;
  };

  virtual ArgumentList fitHardOptions() const override {
    return
      ArgumentList(QList<Argument *>()
                   << new BoolArgument("use-surface", 
                                       "Use Surface",
                                       "whether to use a surface or an "
                                       "amplitude parameter (default false)")
                   << new IntegerArgument("number", 
                                          "Number of peaks",
                                          "number of distinct peaks (default 1)")

                   );
  };

  DistributionFit(Function f,
                  const char * name,
                  const char * desc,
                  const QStringList & paramNames) : 
    PerDatasetFit(name, desc, desc, 1, -1, false),
    accessoryNames(paramNames), func(f)
  {
    if(accessoryNames.size() != (sizeof...(Args) - 1))
      throw InternalError("Wrong argument size");
    makeCommands();
  };
};

template<> double DistributionFit<double, double>::fn(double x, const double * a, int idx) const {
  const double & param = a[paramBase(idx) + 2];
  return func(x, param);
}

template<> double DistributionFit<double, double>::fn(double x, const double * a) const {
  return func(x, a[0]);
}

template<> double DistributionFit<double, double, double>::fn(double x, const double * a, int idx) const {
  const double & p1 = a[paramBase(idx) + 2];
  const double & p2 = a[paramBase(idx) + 3];
  return func(x, p1, p2);
}

template<> double DistributionFit<double, double, double>::fn(double x, const double * a) const {
  return func(x, a[0], a[1]);
}





static DistributionFit<double, double> gaussian(&::gsl_ran_gaussian_pdf,
                                                "gaussian", 
                                                "One or several gaussians",
                                                QStringList() << "sigma");

static DistributionFit<double, double> lorentzian(&::gsl_ran_cauchy_pdf,
                                                  "lorentzian", 
                                                  "A Lorentzian (also named Cauchy distribution)",
                                                  QStringList() << "gamma");
static DistributionFit<double, double, double>
psvgt(&Functions::pseudoVoigt,
      "pseudo-voigt", 
      "A pseudo-voigt distribution (linear combination of a gaussian and a lorentzian)",
      QStringList() << "w" << "mu");
