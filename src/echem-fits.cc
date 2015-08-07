/**
   @file echem-fits.cc
   Fits related to electrochemistry
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2015 by CNRS/AMU

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

#include <terminal.hh>

#include <soas.hh>
#include <fitdata.hh>

#include <gsl/gsl_const_mksa.h>

/// A fit of a sum of nersnt equations.
class NernstFit : public FunctionFit {
protected:

  class Storage : public FitInternalStorage {
  public:
    /// The number of species
    QList<int> number;

  };



  virtual void processOptions(const CommandOptions & opts, FitData * data) const
  {
    int species = -1;
    Storage * s = storage<Storage>(data);
    s->number.clear();
    s->number << 2;
    if(opts.contains("states"))
      updateFromOptions(opts, "states", s->number);
    updateFromOptions(opts, "species", species);
    if(species > 1) {
      if(s->number.size() != 1)
        Terminal::out << "/species specified, but /states already gives several species, ignoring" << endl;
      else {
        while(--species > 0)
          s->number << s->number[0];
      }
    }
  }

  
  virtual QString optionsString(FitData * data) const {
    Storage * s = storage<Storage>(data);
    return QString("%1 species: %2").
      arg(s->number.size()).arg("??");
  }

  /// Returns a name suitable for the given redox state of the numbered state
  static QString stateName(int st, int nb) {
    if(nb > 3)
      return QString::number(st);
    if(st == 0)
      return "red";
    if(st == 1 && nb == 3)
      return "int";
    return "ox";
  };

public:
  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int ds = -1) const {
    return deepCopy<Storage>(source);
  };



  /// Numbering: 0 is the most reduced species.
  virtual QList<ParameterDefinition> parameters(FitData * data) const {
    Storage * s = storage<Storage>(data);
    QList<ParameterDefinition> defs;

    defs << ParameterDefinition("temperature", true);

    for(int i = 0; i < s->number.size(); i++) {
      int nb = s->number[i];
      QChar id('a' + i);

      // Absorbances
      for(int j = 0; j < nb; j++)
        defs << ParameterDefinition(QString("A_%1_%2").arg(id).
                                    arg(stateName(j, nb)),
                                    (j == 0 && i > 0));
    
      for(int j = 0; j < nb-1; j++) {
        defs << ParameterDefinition(QString("E_%1_%2/%3").arg(id).
                                    arg(stateName(j+1, nb)).
                                    arg(stateName(j, nb)));
        defs << ParameterDefinition(QString("n_%1_%2/%3").arg(id).
                                    arg(stateName(j+1, nb)).
                                    arg(stateName(j, nb)), true);
      }
    }
    return defs;
  };

  virtual double function(const double * a, 
                          FitData * data, double x) const {
    Storage * s = storage<Storage>(data);


    double fara = GSL_CONST_MKSA_FARADAY /
      (a[0] * GSL_CONST_MKSA_MOLAR_GAS);


    double rv = 0;
    int offset = 1;
    int nbs = s->number.size();
    for(int j = 0; j < nbs; j++) {
      int nb = s->number[j];
      const double * ampl = a+offset;
      const double * couples = ampl + nb;
      offset += 3*nb - 2;

      double rel = 1;
      double numer = 0;
      double denom = 0;
      for(int i = 0; i < nb; i++) {
        if(i > 0) {
          rel *= exp( fara * couples[1] * (x - couples[0]));
          couples += 2;
        }
        denom += rel;
        numer += *ampl * rel;
        ++ampl;
      }
      rv += numer/denom;
    }
    return rv;
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet *ds,
                            double * a) const
  {
    Storage * s = storage<Storage>(data);
    double *t = a-1;
    *(++t) = soas().temperature();
    const double ymin = ds->y().min();
    const double ymax = ds->y().max();

    for(int j = 0; j < s->number.size(); j++) {
      int nb = s->number[j];
      for(int i = 0; i < nb; i++)
        *(++t) = (j == 0 || i != 0 ? ymin + i *(ymax - ymin)/(nb-1) : 0);
    
      // Now transitions
      const double xmin = ds->x().min();
      const double xmax = ds->x().max();
      for(int i = 0; i < nb-1; i++) {
        *(++t) = xmin + (i+0.5) *(xmax - xmin)/(nb-1);
        *(++t) = 1;
      }
    }
  };
  
  virtual ArgumentList * fitHardOptions() const {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new SeveralIntegersArgument("states", 
                                                  "Number of states",
                                                  "Number of redox states for each species")
                   << new IntegerArgument("species", 
                                          "Number of distinct species",
                                          "Number of distinct species (regardless of their redox state)")
                   
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

  class Storage : public FitInternalStorage {
  public:
    /// The number of species
    int number = 1;

    /// The number of 2-el species
    int twoElectrons = 0;

    /// Whether or not the species are distinct
    bool distinct = true;
  };

public:
  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const {
    return new Storage;
  };

  virtual FitInternalStorage * copyStorage(FitData * /*data*/, FitInternalStorage * source, int ds = -1) const {
    return deepCopy<Storage>(source);
  };

  
protected:

 
  virtual void processOptions(const CommandOptions & opts, FitData * data) const
  {
    Storage * s = storage<Storage>(data);

    s->number = 1;
    s->twoElectrons = 0;
    s->distinct = true;

    updateFromOptions(opts, "species", s->number);
    updateFromOptions(opts, "2el", s->twoElectrons);
    updateFromOptions(opts, "distinct", s->distinct);
  }

  
  virtual QString optionsString(FitData * data) const {
    Storage * s = storage<Storage>(data);
    return QString("%1 species").
      arg(s->number);
  }

  /// This computes the same thing as function but in addition
  /// computes the annotations should the annotations pointer be NULL.
  void annotatedFunction(const double * a, FitData * data, 
                         const DataSet * ds , gsl_vector * target,
                         QList<Vector> * annotations = NULL) const
  {
    Storage * s = storage<Storage>(data);

    const Vector & xv = ds->x();

    // We look at the sign of the first
    double sign = (xv[0] > xv[1] ? -1 : 1);

    double fara = GSL_CONST_MKSA_FARADAY /
      (a[0] * GSL_CONST_MKSA_MOLAR_GAS);
    const double & scan_rate = a[1];

    for(int i = 0; i < xv.size(); i++) {
      double cur = 0;

      
      int idx = (s->distinct ? 2 : 3);

      // These are always considered 1-el peaks :
      for(int j = 0; j < s->number; j++) {
        // Assignments for readability, I hope they're optimized out.
        const double & gamma = a[s->distinct ? idx++ : 2];
        const double & e0 = a[idx++];
        const double & n = a[idx++];
        double e = exp(fara * n * (xv[i] - e0));
        double spec = scan_rate * n * 1 * GSL_CONST_MKSA_FARADAY * 
          fara * gamma * e/((1 + e)*(1 + e)) * sign;
        if(annotations)
          (*annotations)[j][i] = spec;
        cur += spec;
      }

      for(int j = s->number; j < s->number + s->twoElectrons; j++) {
        const double & gamma = a[s->distinct ? idx++ : 2];
        const double & e0 = a[idx++];
        const double & deltae = a[idx++];

        double sqrt_k = exp(fara * 0.5 * deltae);
        double sqrt_eps = exp(fara * (xv[i] - e0));
        double denom = sqrt_eps + sqrt_k + 1/sqrt_eps;
        double spec = scan_rate * GSL_CONST_MKSA_FARADAY * 
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
  virtual QList<ParameterDefinition> parameters(FitData * data) const {
    Storage * s = storage<Storage>(data);
    QList<ParameterDefinition> defs;

    defs << ParameterDefinition("temperature", true);

    // We use the scan rate to determine the electroactive coverage
    defs << ParameterDefinition("nu", true);
    if(! s->distinct)
      defs << ParameterDefinition(QString("Gamma"));
    
    // amplitudes
    for(int i = 0; i < s->number; i++) {
      // We use electroactive coverage, that's what we want !
      if(s->distinct)
        defs << ParameterDefinition(QString("Gamma_%1").arg(i));
      defs << ParameterDefinition(QString("E_%1").arg(i));
      defs << ParameterDefinition(QString("n_%1").arg(i), true);
    }

    for(int i = 0; i < s->twoElectrons; i++) {
      // We use electroactive coverage, that's what we want !
      if(s->distinct)
        defs << ParameterDefinition(QString("Gamma_%1").arg(i + s->number));
      defs << ParameterDefinition(QString("E_%1").arg(i + s->number));
      defs << ParameterDefinition(QString("Delta_E_%1").arg(i + s->number));
    }


    return defs;
  };

  virtual bool hasSubFunctions (FitData * data) const {
    Storage * s = storage<Storage>(data);
    return s->number > 1;
  };

  virtual bool displaySubFunctions (FitData *) const {
    return true;               
  };

  

  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target) const
  {
    annotatedFunction(a, data, ds, target);
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
    for(int i = 0; i < s->number + s->twoElectrons; i++) {
      (*targetData) << Vector(sz, 0);
      *targetAnnotations << QString("Species %1").arg(i+1);
    }
    
    annotatedFunction(parameters, data, ds, NULL,
                      targetData);
  };


  virtual void initialGuess(FitData * data, 
                            const DataSet *ds,
                            double * a) const
  {
    Storage * s = storage<Storage>(data);

    double *t = a-1;
    *(++t) = soas().temperature();
    *(++t) = 0.1;               // default scan rate

    const double xmin = ds->x().min();
    const double xmax = ds->x().max();

    if(! s->distinct)
      *(++t) = 1e-11;           // gamma

    for(int i = 0; i < s->number; i++) {
      if(s->distinct)
        *(++t) = 1e-11;           // gamma
      *(++t) = xmin + (i+0.5) *(xmax - xmin)/(s->number + s->twoElectrons);
      *(++t) = 1;
    }
    for(int i = 0; i < s->twoElectrons; i++) {
      if(s->distinct)
        *(++t) = 1e-11;           // gamma
      *(++t) = xmin + (i+s->number+0.5) *(xmax - xmin)/(s->number + s->twoElectrons);
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
    makeCommands();

  };
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
AdsorbedFit fit_adsorbed;
