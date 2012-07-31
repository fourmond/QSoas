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

#include <linearkineticsystem.hh>



//////////////////////////////////////////////////////////////////////

/// This fit fits three datasets at the same time, representing two
/// phases of reactivation (fast, then slow) and a lost phase as a
/// function of the time of exposition to a tri-state inactivation
/// process.
class ReactivationAmplitudeFit : public Fit {

public:
  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;

    // Kinetic parameters for inactivation:
    for(int j = 0; j < 3; j++) {
      for(int k = 0;  k < 3; k++) {
        QString param;
        if(j == k) // external loss
          param = QString("k_i_%1").arg(j);
        else
          param = QString("k_i_%1%2").arg(j).arg(k);
        defs << ParameterDefinition(param, false, false);
      }
    }
    
    // Kinetic parameters for reactivation (way too overdefined
    for(int j = 0; j < 3; j++) {
      for(int k = 0;  k < 3; k++) {
        QString param;
        if(j == k) // external loss
          continue;           // No loss !
        else
          param = QString("k_r_%1%2").arg(j).arg(k);
        defs << ParameterDefinition(param, false, false);
      }
    }
    return defs;
  };


  /// Redefined to wrap to a call to the per-dataset function
  virtual void function(const double * a,
                        FitData * data, gsl_vector * target)
  {
    LinearKineticSystem inact(3);
    LinearKineticSystem react(3);


    /// Initial constants are fine
    inact.setConstants(a);

    QVector<double> kmr(9, 0);

    int s = 9;
    for(int j = 0; j < 3; j++) {
      for(int k = 0;  k < 3; k++) {
        if(j == k) // external loss
          continue;           // No loss !
        else {
          kmr[j * 3 + k] = a[s];
          s++;
        }
      }
    }

    react.setConstants(kmr.data());

    QTextStream o(stdout);
    o << "Inactivation matrix:\n"
      << Utils::matrixString(inact.kineticMatrixValue()) << endl;

    double concentrations[3] = {1,0,0};
    gsl_vector_view cv = gsl_vector_view_array(concentrations, 3);

    inact.setInitialConcentrations(&cv.vector);

    o << "Reactivation matrix:\n"
      << Utils::matrixString(react.kineticMatrixValue()) 
      << endl;

    // This assumes that the X values are identical over all
    // datatsets.

    gsl_vector_view fast_v = data->viewForDataset(0, target);
    gsl_vector_view slow_v = data->viewForDataset(1, target);
    gsl_vector_view dead_v = data->viewForDataset(2, target);

    const Vector & xvals = data->datasets.first()->x();

    // The side effect of that is simply to compute the
    // eigenvalues/eigenvectors
    react.setInitialConcentrations(&cv.vector);

    // We decide which is the slow phase and which is the fast one.
    int fast = -1;
    double kfast = 0;
    int slow = -1;
    for(int i = 0; i < 3; i++) {
      double v = GSL_REAL(gsl_vector_complex_get(react.eigenValuesVector(), i));
      if(v < 0) {
        if(fast < 0) {
          fast = i;
          kfast = v;
        }
        else {
          if(v < kfast) {
            slow = fast;
            fast = i;
          }
          else
            slow = i;
        }
      }
    }

    for(int i = 0; i < xvals.size(); i++) {

      // We first get the concentrations at the time t
      inact.getConcentrations(xvals[i], &cv.vector);

      // This can be used to set the dead phase
      double s = 0;
      for(int j = 0; j < 3; j++)
        s += concentrations[j];

      gsl_vector_set(&dead_v.vector, i, 1 - s);
      
      // Now, we get all the phases:
      react.setInitialConcentrations(&cv.vector);
      
      gsl_complex c = gsl_vector_complex_get(react.coordinateVector(), fast);
      c = gsl_complex_mul(c, 
                          gsl_matrix_complex_get(react.
                                                 eigenVectorsValue(), 
                                                 0, fast));

      gsl_vector_set(&fast_v.vector, i, - s * GSL_REAL(c));

      
      c = gsl_vector_complex_get(react.coordinateVector(), slow);
      c = gsl_complex_mul(c, 
                          gsl_matrix_complex_get(react.
                                                 eigenVectorsValue(), 
                                                 0, slow));


      gsl_vector_set(&slow_v.vector, i, - s * GSL_REAL(c));
    }
  };

  /// Redefined to wrap to a call to the per-dataset function
  virtual void initialGuess(FitData * data, double * a)
  {
    for(int i = 0; i < data->parametersPerDataset(); i++)
      a[i] = 1;
  };


  ReactivationAmplitudeFit() : 
    Fit("inact-react-amplitude-2phases", 
        "...",
        "...", 3, 3) 
  { ;};


};

static ReactivationAmplitudeFit react;

///////////////////////////////////////////////////////////////////////////


/// This fit somehow summarizes almost all the others, while being
/// more powerful (but slightly less easy to use).
class KineticSystemFit : public PerDatasetFit {

  /// The number of distinctSteps
  int distinctSteps;

  /// The steps.
  QList<int> steps;

  /// The number of species
  int species;


protected:

  virtual void processOptions(const CommandOptions & opts)
  {
    steps.clear();
    steps << 0 << 1 << 0; // one step forward, one step backward.
    updateFromOptions(opts, "steps", steps);

    species = 2;
    updateFromOptions(opts, "species", species);

    distinctSteps = 0;
    for(int i = 0; i < steps.size(); i++)
      if(distinctSteps < steps[i])
        distinctSteps = steps[i];
    distinctSteps++;            // To get the actual number ! 
  }

  const double * initialConcentrations(const double * params) {
    return params + (species * (species + 1) + 1) * distinctSteps;
  };

  double * initialConcentrations(double * params) {
    return params + (species * (species + 1) + 1) * distinctSteps;
  };

  const double * stepConstants(const double * params, int step) {
    return params + (species * (species + 1) + 1) * step + species;
  };

  double * stepConstants(double * params, int step) {
    return params + (species * (species + 1) + 1) * step + species;
  };

  const double * currents(const double * params, int step) {
    return params + (species * (species + 1) + 1) * step;
  };

  double * currents(double * params, int step) {
    return params + (species * (species + 1) + 1) * step;
  };


public:

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;

    // First, potential-dependent information
    for(int i = 0; i < distinctSteps; i++) {
      QString suffix = QString("_#%1").arg(i+1);

      /// Current of each species
      for(int j = 0; j < species; j++)
        defs << ParameterDefinition(QString("I_%1%2").arg(j+1).arg(suffix));

      /// Reaction constants
      for(int j = 0; j < species; j++)
        for(int k = 0; k < species; k++)
          defs << ParameterDefinition(QString("k_%1_%2%3").
                                      arg(j+1).arg(k+1).arg(suffix)); 

      defs << ParameterDefinition(QString("k_loss%1").arg(suffix)); 
      /// @todo Offer the possibility to set the sum and ratio rather
      /// than each individually
        
    }

    // Then, initial conditions
    for(int j = 0; j < species; j++)
      defs << ParameterDefinition(QString("alpha_%1_0").arg(j+1), true);
    
    // Then, steps.
    for(int i = 0; i < steps.size(); i++) {
      QChar step('a' + i);
      /// Starting time (including the 0)
      defs << ParameterDefinition(QString("xstart_%1").arg(step), true);
    }
    
    return defs;
  };

  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target)
  {
    LinearKineticSystem sys(species);


    QVarLengthArray<double, 30> concentrations(species);
    gsl_vector_view vco = gsl_vector_view_array(concentrations.data(), species);

    QVarLengthArray<double, 30> currents(species);
    gsl_vector_view vcu = gsl_vector_view_array(currents.data(), species);

    int cur = -1;
    double xstart = 0;
    double xend = 0;

    const Vector & xv = ds->x();

    // Copy initial concentrations
    const double *c = initialConcentrations(a);
    for(int i = 0; i < species; i++)
      concentrations[i] = c[i];

    for(int i = 0; i < xv.size(); i++) {
      double x = xv[i];
      if(cur < 0 || xend < x) {
        cur++;

        // First, potential-dependant-stuff:
        int potential = steps[cur];
        
        // Currents:
        const double  * base = this->currents(a, potential);
        for(int i = 0; i < species; i++)
          currents[i] = base[i];


        // Now, range checking: we don't want rate constants to be
        // negative !
        const double * sc = stepConstants(a, potential);

        /// @todo This check could optionnally be turned off.
        for(int i = 0; i <= species * species; i++)
          if(sc[i] < 0)
            throw RangeError(QString("Found a negative rate "
                                     "constant (step: %1): %2 !").
                             arg(potential).arg(i));

        // Global loss...
        sys.setConstants(sc, sc[species*species]);

        /// @todo There may be an "off-by-one-data-point" mistake here
        sys.setInitialConcentrations(&vco.vector);

        base = initialConcentrations(a) + species;
        xstart = base[cur];
        if(cur < steps.size() - 1)
          xend = base[cur+1];
        else
          xend = xv.max();
      }
      x -= xstart;
      sys.getConcentrations(x, &vco.vector);
      double res = 0;
      gsl_blas_ddot(&vco.vector, &vcu.vector, &res);
      gsl_vector_set(target, i, res);
    }

  };

  virtual void initialGuess(FitData * params, 
                            const DataSet *ds,
                            double * a)
  {
    const Vector & x = ds->x();
    const Vector & y = ds->y();

    double * base = NULL;
    for(int i = 0; i < distinctSteps; i++) {
      base = currents(a, i);
      for(int j = 0; j < species; j++)
        base[j] = (j == 0 ? y.max() : 0);

      base = stepConstants(a, i);
      for(int j = 0; j < species; j++)
        for(int k = 0; k < species; k++)
          base[j * species + k] = ( j == k ? 0 : ((j+0.5)*(i+k+1))*1e-2);
      base[species * species] = 1e-4;
    }

    base = initialConcentrations(a);
    for(int i = 0; i < species; i++)
      base[i] = (i ? 0 : 1);

    base += species;

    double xmax = Utils::roundValue(x.max());
    for(int i = 0; i < steps.size(); i++)
      base[i] = i * xmax/steps.size();
    
  };

  KineticSystemFit() :
    PerDatasetFit("kinetic-system", 
                  "Several steps with a kinetic evolution",
                  "Fits of exponentials on several steps with "
                  "film loss bookkeeping", 1, -1, false) 
  { 
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new IntegerArgument("species", 
                                          "Number of species",
                                          "Number of species")
                   << new 
                   SeveralIntegersArgument("steps", 
                                           "Steps",
                                           "Step list with numbered conditions")
                   );
    makeCommands(NULL, NULL, NULL, opts);
  };
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
KineticSystemFit fit_kinetic_system;
