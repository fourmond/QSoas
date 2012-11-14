/**
   @file echem-fits.cc
   Fits related to electrochemistry
   Copyright 2011, 2012 by Vincent Fourmond

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

/// Fit for a full nerstian system.
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
        rel *= exp( fara * (x - couples[0])/couples[1]);
        couples += 2;
      }
      denom += rel;
      numer += *ampl * rel;
      ++ampl;
    }
    return numer/denom;
  };

  virtual void initialGuess(FitData * params, 
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
  
  
  NernstFit() : FunctionFit("nernst", 
                            "Nerstian behaviour",
                            "Fit to a Nerst equation", 1, -1, false) 
  { 
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new IntegerArgument("states", 
                                          "Number of states",
                                          "Number of redox states")
                   );
    makeCommands(NULL, NULL, NULL, opts);

  };
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
NernstFit fit_nernst;
