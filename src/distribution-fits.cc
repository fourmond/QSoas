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

#include <gsl/gsl_randist.h>


/// Base class for all distribution-based fits
/// @todo make it possible to use a common "sigma" parameter ?
/// @todo use peak information for initial values.
template <double f(double, double)>
class DistributionFit : public PerDatasetFit {

  class Storage : public FitInternalStorage {
  public:
    /// The number of different distributions
    int number;

    /// Whether the amplitude or the surface is used for each 'peak'.
    ///
    /// Keep in mind that it is 
    bool useSurface;

  };

  /// Name of the accessory parameter
  QString accessoryName;

protected:

  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int /*ds = -1*/) const {
    return deepCopy<Storage>(source);
  };

  virtual void processOptions(const CommandOptions & opts, FitData * data) const
  {
    Storage * s = storage<Storage>(data);

    s->useSurface = false;
    s->number = 1;
    updateFromOptions(opts, "use-surface", s->useSurface);
    updateFromOptions(opts, "number", s->number);
  }

  
  virtual QString optionsString(FitData * data) const {
    Storage * s = storage<Storage>(data);
    return QString("%1 species, %2").arg(s->number).
      arg(s->useSurface ? "surface" : "amplitude");
  }

  void annotatedFunction(const double * a, FitData * data, 
                         const DataSet * ds , gsl_vector * target,
                         QList<Vector> * annotations = NULL) const
  {
    Storage * s = storage<Storage>(data);
        
    const Vector & xv = ds->x();

    double prefactors[s->number];
    for(int j = 0; j < s->number; j++)
      prefactors[j] = a[3*j+2] / (s->useSurface ? 1 : f(0, a[3*j+3]));

    for(int i = 0; i < xv.size(); i++) {
      double cur = a[0];
      const double & x = xv[i];
      for(int j = 0; j < s->number; j++) {
        // Assignments for readability, I hope they're optimized out.
        const double & x0 = a[3*j+1];
        const double & pref = prefactors[j];
        const double & param = a[3*j + 3];
        double val = pref * f(x - x0, param);
        if(annotations)
          (*annotations)[j][i] = a[0] + val;
        cur += val;
      }
      if(target)
        gsl_vector_set(target, i, cur);
    }
  };




public:

  virtual bool hasSubFunctions(FitData * data) const {
    Storage * s = storage<Storage>(data);

    return s->number > 1;
  };

  virtual bool displaySubFunctions(FitData * ) const {
    return true;               
  };

  virtual void computeSubFunctions(const double * parameters,
                                   FitData * data, 
                                   const DataSet * ds,
                                   QList<Vector> * targetData,
                                   QStringList * targetAnnotations) const
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
                        const DataSet * ds , gsl_vector * target) const {
    annotatedFunction(a, data, ds, target);
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet *ds,
                            double * a) const
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
    double param = (xmax - xmin)/(s->number * 4);
    for(int i = 0; i < s->number; i++) {
      *(++t) = xmin + (i+0.5) * (xmax - xmin)/s->number; // position
      *(++t) = (ymax - ymin) / (s->useSurface ? f(0,param) : 1); // amplitude
      *(++t) = param;
    }
  };

  virtual QList<ParameterDefinition> parameters(FitData * data) const {
    Storage * s = storage<Storage>(data);

    QList<ParameterDefinition> params;
    params << ParameterDefinition("Y0"); // base line
    for(int j = 0; j < s->number; j++)
      params << ParameterDefinition(QString("x_%1").arg(j+1))
             << ParameterDefinition(QString("%2_%1").arg(j+1).
                                    arg(s->useSurface ? "S" : "A"))
             << ParameterDefinition(QString("%2_%1").arg(j+1).
                                    arg(accessoryName));

    return params;
  };

  virtual ArgumentList * fitHardOptions() const {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new BoolArgument("use-surface", 
                                       "Use Surface",
                                       "Whether to use a surface or an "
                                       "amplitude parameter")
                   << new IntegerArgument("number", 
                                          "Number of peaks",
                                          "Number of distinct peaks")

                   );
    return opts;
  };

  DistributionFit(const char * name,
             const char * desc,
             const QString & param) : 
    PerDatasetFit(name, desc, desc, 1, -1, false),
    accessoryName(param)
  { 
    makeCommands();
  };
};


static DistributionFit<gsl_ran_gaussian_pdf> gaussian("gaussian", 
                                          "One or several gaussians", "sigma");

static DistributionFit<gsl_ran_cauchy_pdf> lorentzian("lorentzian", 
                                                 "A Lorentzian (also named Cauchy distribution)", "gamma");
