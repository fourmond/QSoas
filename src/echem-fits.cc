/**
   @file echem-fits.cc
   Fits related to electrochemistry
   Copyright 2011 by Vincent Fourmond
             2012, 2013 by CNRS/AMU

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

/// Fit for a full nerstian single system.
///
/// @todo Extend that fit -- or write another one -- for the
/// superposition of various independant species ?
///
/// I actually need to really think about that...
class NernstFit : public FunctionFit {

  /// The number of species
  int number;

  /// The species which should be considered "unstable"
  ///
  /// @todo I should come up with a simple for handling boolean values
  /// indexed on a small number of integers - using bit fields; that
  /// would most probably be faster and definitely much more compact
  /// than a set !
  QSet<int> unstableSpecies;

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
    number = 2;
    updateFromOptions(opts, "states", number);

    QList<int> unstable;
    /// @todo Implement unstable species !
    updateFromOptions(opts, "unstable", unstable);
    unstableSpecies = QSet<int>::fromList(unstable);

  }

  
  virtual QString optionsString() const {
    return QString("%1 species").
      arg(number);
  }


public:

  /// Numbering: 0 is the most reduced species.
  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;

    defs << ParameterDefinition("temperature", true);
    
    // Absorbances
    for(int i = 0; i < number; i++)
      defs << ParameterDefinition(QString("A_%1").arg(i));
    
    for(int i = 0; i < number-1; i++) {
      defs << ParameterDefinition(QString("E_%1/%2").arg(i+1).arg(i));
      defs << ParameterDefinition(QString("n_%1/%2").arg(i+1).arg(i), true);
    }
    return defs;
  };

  virtual double function(const double * a, 
                          FitData * , double x) {
    double rel = 1;             // Relative amount of species
    double numer = 0;           // Sum for the numerator
    double denom = 0;           // Sum for the denominator


    double fara = GSL_CONST_MKSA_FARADAY /
      (a[0] * GSL_CONST_MKSA_MOLAR_GAS);

    const double * ampl = a+1;
    const double * couples = ampl + number;

    for(int i = 0; i < number; i++) {
      if(i > 0) {
        rel *= exp( fara * couples[1] * (x - couples[0]));
        couples += 2;
      }
      denom += rel;
      numer += *ampl * rel;
      ++ampl;
    }
    return numer/denom;
  };

  virtual void initialGuess(FitData * /*data*/, 
                            const DataSet *ds,
                            double * a)
  {
    double *t = a-1;
    *(++t) = soas().temperature();
    const double ymin = ds->y().min();
    const double ymax = ds->y().max();
    for(int i = 0; i < number; i++)
      *(++t) = ymin + i *(ymax - ymin)/(number-1);
    
    // Now transitions
    const double xmin = ds->x().min();
    const double xmax = ds->x().max();
    for(int i = 0; i < number-1; i++) {
      *(++t) = xmin + (i+0.5) *(xmax - xmin)/(number-1);
      *(++t) = 1;
    }
  };
  
  virtual ArgumentList * fitHardOptions() const {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new IntegerArgument("states", 
                                          "Number of states",
                                          "Number of redox states")
                   );
    return opts;
  };
  
  NernstFit() : FunctionFit("nernst", 
                            "Nerstian behaviour",
                            "Fit to a Nerst equation", 1, -1, false) 
  { 
    makeCommands();

  };
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
NernstFit fit_nernst;

//////////////////////////////////////////////////////////////////////


/// Fit to any number of 'ideal' adsorbed species.
class AdsorbedFit : public PerDatasetFit {

  /// The number of species
  int number;

  /// The number of 2-el species
  int twoElectrons;

  /// Whether or not the species are distinct
  bool distinct;

protected:

  virtual void processOptions(const CommandOptions & opts)
  {
    number = 1;
    twoElectrons = 0;
    distinct = true;

    updateFromOptions(opts, "species", number);
    updateFromOptions(opts, "2el", twoElectrons);
    updateFromOptions(opts, "distinct", distinct);
  }

  
  virtual QString optionsString() const {
    return QString("%1 species").
      arg(number);
  }

  /// This computes the same thing as function but in addition
  /// computes the annotations should the annotations pointer be NULL.
  void annotatedFunction(const double * a, FitData * /*data*/, 
                         const DataSet * ds , gsl_vector * target,
                         QList<Vector> * annotations = NULL)
  {
    const Vector & xv = ds->x();

    // We look at the sign of the first
    double sign = (xv[0] > xv[1] ? -1 : 1);

    double fara = GSL_CONST_MKSA_FARADAY /
      (a[0] * GSL_CONST_MKSA_MOLAR_GAS);
    const double & scan_rate = a[1];

    for(int i = 0; i < xv.size(); i++) {
      double cur = 0;

      
      int idx = (distinct ? 2 : 3);

      // These are always considered 1-el peaks :
      for(int j = 0; j < number; j++) {
        // Assignments for readability, I hope they're optimized out.
        const double & gamma = a[distinct ? idx++ : 2];
        const double & e0 = a[idx++];
        const double & n = a[idx++];
        double e = exp(fara * n * (xv[i] - e0));
        double spec = scan_rate * n * 1 * GSL_CONST_MKSA_FARADAY * 
          fara * gamma * e/(1 + e*e) * sign;
        if(annotations)
          (*annotations)[j][i] = spec;
        cur += spec;
      }

      for(int j = number; j < number + twoElectrons; j++) {
        const double & gamma = a[distinct ? idx++ : 2];
        const double & e0 = a[idx++];
        const double & deltae = a[idx++];

        double sqrt_k = exp(fara * 0.5 * deltae);
        double sqrt_eps = exp(fara * (xv[i] - e0));
        double denom = sqrt_eps + sqrt_k + 1/sqrt_eps;
        double spec = scan_rate * 2 * GSL_CONST_MKSA_FARADAY * 
          fara * gamma * sqrt_k * (sqrt_eps + 4/sqrt_k + 1/sqrt_eps)/
          (denom * denom);
        if(annotations)
          (*annotations)[j][i] = spec;
        cur += spec;
      }

      if(target)
        gsl_vector_set(target, i, cur);
    }
  };

public:

  /// Numbering: 0 is the most reduced species.
  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;

    defs << ParameterDefinition("temperature", true);

    // We use the scan rate to determine the electroactive coverage
    defs << ParameterDefinition("nu", true);
    if(! distinct)
      defs << ParameterDefinition(QString("Gamma"));
    
    // amplitudes
    for(int i = 0; i < number; i++) {
      // We use electroactive coverage, that's what we want !
      if(distinct)
        defs << ParameterDefinition(QString("Gamma_%1").arg(i));
      defs << ParameterDefinition(QString("E_%1").arg(i));
      defs << ParameterDefinition(QString("n_%1").arg(i), true);
    }

    for(int i = 0; i < twoElectrons; i++) {
      // We use electroactive coverage, that's what we want !
      if(distinct)
        defs << ParameterDefinition(QString("Gamma_%1").arg(i + number));
      defs << ParameterDefinition(QString("E_%1").arg(i + number));
      defs << ParameterDefinition(QString("Delta_E_%1").arg(i + number));
    }


    return defs;
  };

  virtual bool hasSubFunctions () const {
    return number > 1;
  };

  virtual bool displaySubFunctions () const {
    return true;               
  };

  

  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target)
  {
    annotatedFunction(a, data, ds, target);
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
    for(int i = 0; i < number + twoElectrons; i++) {
      (*targetData) << Vector(sz, 0);
      *targetAnnotations << QString("Species %1").arg(i+1);
    }
    
    annotatedFunction(parameters, data, ds, NULL,
                      targetData);
  };


  virtual void initialGuess(FitData * /*data*/, 
                            const DataSet *ds,
                            double * a)
  {
    double *t = a-1;
    *(++t) = soas().temperature();
    *(++t) = 0.1;               // default scan rate

    const double xmin = ds->x().min();
    const double xmax = ds->x().max();

    if(! distinct)
      *(++t) = 1e-11;           // gamma

    for(int i = 0; i < number; i++) {
      if(distinct)
        *(++t) = 1e-11;           // gamma
      *(++t) = xmin + (i+0.5) *(xmax - xmin)/(number + twoElectrons);
      *(++t) = 1;
    }
    for(int i = 0; i < twoElectrons; i++) {
      if(distinct)
        *(++t) = 1e-11;           // gamma
      *(++t) = xmin + (i+number+0.5) *(xmax - xmin)/(number + twoElectrons);
      *(++t) = 0.1;             // DeltaE of 100 mV ?
    }
  };
  
  virtual ArgumentList * fitHardOptions() const {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new IntegerArgument("species", 
                                          "Number of 1-electron species",
                                          "Number of 1-electron species")
                   << new IntegerArgument("2el", 
                                          "Number of 2-electron species",
                                          "Number of true 2-electron species")
                   << new BoolArgument("distinct", 
                                       "Whether the species are distinct",
                                       "If true (default) then all species have their own surface concentrations")
                   );
    return opts;
  };
  
  AdsorbedFit() : PerDatasetFit("adsorbed", 
                                "Adsorbed species",
                                "Electrochemical response of ideal adsorbed "
                                "species",
                                1, -1, false) 
  { 
    number = 1;
    makeCommands();

  };
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
AdsorbedFit fit_adsorbed;
