/**
   @file distrib-fits.cc
   GSL Distribution-based fits
   Copyright 2013 by Vincent Fourmond

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

#include <gsl/gsl_randist.h>


/// Base class for all distribution-based fits
template <double f(double, double)>
class DistribFit : public PerDatasetFit {

  /// The number of different distributions
  int number;

  /// Whether the amplitude or the surface is used for each 'peak'.
  ///
  /// Keep in mind that it is 
  bool useSurface;

  /// Name of the accessory parameter
  QString accessoryName;

protected:
  virtual void processOptions(const CommandOptions & opts)
  {
    useSurface = false;
    number = 1;
    updateFromOptions(opts, "use-surface", useSurface);
    updateFromOptions(opts, "number", number);
  }

  
  virtual QString optionsString() const {
    return QString("%1 species, %2").arg(number).
      arg(useSurface ? "surface" : "amplitude");
  }

  void annotatedFunction(const double * a, FitData * /*data*/, 
                         const DataSet * ds , gsl_vector * target,
                         QList<Vector> * annotations = NULL)
  {
    const Vector & xv = ds->x();

    double prefactors[number];
    for(int j = 0; j < number; j++)
      prefactors[j] = a[3*j+2] / (useSurface ? 1 : f(0, a[3*j+3]));

    for(int i = 0; i < xv.size(); i++) {
      double cur = a[0];
      const double & x = xv[i];
      for(int j = 0; j < number; j++) {
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

  virtual bool hasSubFunctions () const {
    return number > 1;
  };

  virtual bool displaySubFunctions () const {
    return true;               
  };

  virtual void computeSubFunctions(const double * parameters,
                                   FitData * data, 
                                   const DataSet * ds,
                                   QList<Vector> * targetData,
                                   QStringList * targetAnnotations)
  {
    targetData->clear();
    targetAnnotations->clear();

    int sz = ds->x().size();
    for(int i = 0; i < number; i++) {
      (*targetData) << Vector(sz, 0);
      *targetAnnotations << QString("Peak %1").arg(i+1);
    }
    
    annotatedFunction(parameters, data, ds, NULL,
                      targetData);
  };


  /// Formula:
  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target) {
    annotatedFunction(a, data, ds, target);
  };

  virtual void initialGuess(FitData * /*data*/, 
                            const DataSet *ds,
                            double * a)
  {
    // For now, equally distributed ?

    /// @todo Use peak info ?
    const double xmin = ds->x().min();
    const double xmax = ds->x().max();

    const double ymin = ds->y().min();
    const double ymax = ds->y().max();

    double *t = a;
    *t = ymin;
    double param = (xmax - xmin)/(number * 4);
    for(int i = 0; i < number; i++) {
      *(++t) = xmin + (i+0.5) * (xmax - xmin)/number; // position
      *(++t) = (ymax - ymin) / (useSurface ? f(0,param) : 1); // amplitude
      *(++t) = param;
    }
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> params;
    params << ParameterDefinition("Y0"); // base line
    for(int j = 0; j < number; j++)
      params << ParameterDefinition(QString("x_%1").arg(j+1))
             << ParameterDefinition(QString("%2_%1").arg(j+1).
                                    arg(useSurface ? "S" : "A"))
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

  DistribFit(const char * name,
             const char * desc,
             const QString & param) : 
    PerDatasetFit(name, desc, desc, 1, -1, false),
    accessoryName(param)
  { 
    number = 1;
    useSurface = false;
    makeCommands();
  };
};


DistribFit<gsl_ran_gaussian_pdf> gaussian("gaussian", 
                                          "One or several gaussians", "sigma");
