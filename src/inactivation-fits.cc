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


/// Three steps:
/// \li one step at E0, from a fully active enzyme (with film loss)
/// \li one step at E1, with a sequential biphasic reversible inactivation
/// \li one step back at E0, with the corresponding full reactivation
///
/// @todo Allow the parameter definition to hold descriptions too ?
/// That would raise the possibility to implement tooltips
class InactRev2SeqFit : public PerDatasetFit {
public:

  virtual QList<ParameterDefinition> parameters() const {
    return QList<ParameterDefinition>()
      << ParameterDefinition("x_1", true)
      << ParameterDefinition("x_2", true)
      << ParameterDefinition("I_0") // Fully-active current at E0
      << ParameterDefinition("I_1") // Same as E1

      << ParameterDefinition("k1_1")  // a[4]: k_1 at E1
      << ParameterDefinition("km1_1") // a[5]: k_{-1} at E1
      << ParameterDefinition("k2_1")  // a[6]: k_2 at E1
      << ParameterDefinition("km2_1") // a[7]: k_{-2} at E1 a[7]

      << ParameterDefinition("k1_0")  // a[8]:  k_1 at E0
      << ParameterDefinition("km1_0") // a[9]:  k_{-1} at E0
      << ParameterDefinition("k2_0")  // a[10]: k_2 at E0
      << ParameterDefinition("km2_0") // a[11]: k_{-2} at E0

      << ParameterDefinition("kloss1")  // First step
      << ParameterDefinition("kloss2")  // Second step
      << ParameterDefinition("kloss3"); // Third step
  };

  virtual int parametersCheck(const double * a,
                              FitData * /*data*/,
                              const DataSet * /*ds*/)
  {
    for(int i = 4; i <= 14; i++)
      if(a[i] < 0)
        return GSL_EDOM;
    return GSL_SUCCESS;
  };

  virtual void function(const double * a,
                        FitData * /*data*/,
                        const DataSet * ds,
                        gsl_vector * target)
  {
    double num1 = a[7] * a[5] + a[6] * a[4] + a[7] * a[4];
    double aeq1 = a[5]*a[7]/num1;
    double ieq1 = a[7]*a[4]/num1;
    double s1 = a[4] + a[5] + a[6] + a[7];
    double delta_1 = s1*s1 - 4 * num1;
    double kf_1 = 0.5 * (s1 + sqrt(delta_1));
    double ks_1 = 0.5 * (s1 - sqrt(delta_1));

    double num0 = a[11] * a[9] + a[10] * a[8] + a[11] * a[8];
    double aeq0 = a[9]*a[11]/num0;
    double ieq0 = a[11]*a[8]/num0;
    double s0 = a[8] + a[9] + a[10] + a[11];
    double delta_0 = s0*s0 - 4 * num0;
    double kf_0 = 0.5 * (s0 + sqrt(delta_0));
    double ks_0 = 0.5 * (s0 - sqrt(delta_0));

    // Initially fully active.
    double al_s = ((a[8] - kf_0) * (1 - aeq0) - a[9] * (0 - ieq0))/
      (ks_0 - kf_0);
    double al_f = ((a[8] - ks_0) * (1 - aeq0) - a[9] * (0 - ieq0))/
      (kf_0 - ks_0);
    int step = 0;          // To keep track of what al_s and al_f are.
    double loss = 1;       // The amount of film loss during the last
                           // step

    const Vector & xvalues = ds->x();
    for(int i = 0; i < target->size; i++) {
      double x = xvalues[i];
      double val;
      if(x < a[0]) {
        // First step !
        double ac = aeq0 + al_f * exp(-kf_0 * x) + al_s * exp(-ks_0 * x);
        val = ac * a[2] * exp(-a[12] * x);
      }
      else if(x >= a[0] && x < a[1]) {
        if(step == 0) {
          // initialize
          loss = exp(-a[12] * x);
          
          // Compute the active and inactive1 fractions:
          double aold = aeq0 + al_f * exp(-kf_0 * x) + al_s * exp(-ks_0 * x);
          double iold = ieq0 + (a[8] - kf_0)/a[9] * al_f*exp(-kf_0 * x) 
            + (a[8] - ks_0)/a[9] * al_s*exp(-ks_0 * x);
          
          // Then back to the amplitudes:
          al_s = ((a[4] - kf_1) * (aold - aeq1) - a[5] * (iold - ieq1))/
            (ks_1 - kf_1);
          al_f = ((a[4] - ks_1) * (aold - aeq1) - a[5] * (iold - ieq1))/
            (kf_1 - ks_1);
          step = 1;
        }
        x -= a[0];
        double ac = aeq1 + al_f * exp(-kf_1 * x) + al_s * exp(-ks_1 * x);
        val = ac * a[3] * exp(-a[13] * x) * loss;
      }
      else {
        if(step == 1) {
          x -= a[0];
          // initialize
          loss *= exp(-a[13] * x);
          
          // Compute the active and inactive1 fractions:
          double aold = aeq1 + al_f * exp(-kf_1 * x) + al_s * exp(-ks_1 * x);
          double iold = ieq1 + (a[4] - kf_1)/a[5] * al_f*exp(-kf_1 * x) 
            + (a[4] - ks_1)/a[5] * al_s*exp(-ks_1 * x);
          
          // Then back to the amplitudes:
          al_s = ((a[8] - kf_0) * (aold - aeq0) - a[9] * (iold - ieq0))/
            (ks_0 - kf_0);
          al_f = ((a[8] - ks_0) * (aold - aeq0) - a[9] * (iold - ieq0))/
            (kf_1 - ks_0);
          step = 2;
          x += a[0];
        }
        x -= a[1];
        double ac = aeq0 + al_f * exp(-kf_0 * x) + al_s * exp(-ks_0 * x);
        val = ac * a[2] * exp(-a[14] * x) * loss;
      }
      gsl_vector_set(target, i, val);
    }
  };

  virtual void initialGuess(FitData * , 
                            const DataSet * ds,
                            double * a)
  {
    /// @todo Jump detection
    a[0] = 200;
    a[1] = 300;
    a[2] = ds->y().first();
    a[3] = a[2] * 1.5;          // Why not, after all ?

    a[4] = 1;
    a[5] = 1;
    a[6] = 0.1;
    a[7] = 0.1;

    a[8] = 0.01;                // We definitiely want to reactivate !
    a[9] = 1;
    a[10] = 0.1;
    a[11] = 0.1;

    a[12] = 1e-4;
    a[13] = 1e-4;
    a[14] = 1e-4;
  };

  InactRev2SeqFit() : 
    PerDatasetFit("inact-rev-2seq", 
                  "Reversible inactivation with two inactive states "
                  "in sequence",
                  "Reversible inactivation with two inactive states in "
                  "sequence, studied using a three step (E0 E1 E0) "
                  "chronoamperometric technique") 
  { ;};
};

// DO NOT FORGET TO CREATE AN INSTANCE OF THE CLASS !!
// Its name doesn't matter.
InactRev2SeqFit inact_rev_2seq;



// //////////////////////////////////////////////////////////////////////


/// @todo KineticSystemFit should be rewritten almost from scratch
/// using LinearKineticSystem.

// /// This fit gives the possibility to fit something proportionnal to a
// /// concentration of a species (\todo linear combination of the
// /// species) in a kinetic system where the value of the kinetic
// /// parameters varies in a stepwise fashion.
// class KineticSystemFit : public PerDatasetFit {

//   /// The number of steps with distinct kinetic parameters.
//   int distinctSteps;

//   /// A list describing assigning to each step a set of kinetic
//   /// values.
//   QList<int> steps;

//   /// The number of distinct species (excluding those which are only
//   /// being produced irreversibly)
//   int speciesNumber;

//   void runFitCurrentDataSet(const QString & n, int species, QList<int> stps)
//   {
//     QList<const DataSet *> ds;
//     ds << soas().currentDataSet();
//     runFit(n, species, stps, ds);
//   }

//   void runFit(const QString &, int species, QList<int> stps, QList<const DataSet *> datasets)
//   {
//     speciesNumber = species;
//     steps = stps;
//     distinctSteps = 0;
//     for(int i = 0; i < steps.size(); i++)
//       if(distinctSteps < steps[i])
//         distinctSteps = steps[i];
//     distinctSteps++;            // To get the actual number ! 

//     Terminal::out << "Fit with " << speciesNumber << " species over " 
//                   << steps.size() << " steps, spanning " 
//                   << distinctSteps << " conditions" << endl;

//     {
//       FitData data(this, datasets);
//       FitDialog dlg(&data);
//       dlg.exec();
//     }
//   }

    
// public:

//   virtual QList<ParameterDefinition> parameters() const {
//     QList<ParameterDefinition> defs;
    
//     // First, step-based information, ie, only times
//     for(int i = 0; i < steps.size() - 1; i++)
//       defs << ParameterDefinition(QString("x_%1").arg(QChar('a' + i)), true);


//     /// Then the initial concentration of species:
//     for(int j = 0; j < speciesNumber; j++)
//       defs << ParameterDefinition(QString("A%1_init").arg(j), true);

//     // Now, the kinetic information.
//     for(int i = 0; i < distinctSteps; i++) {

//       /// Current of the active form
//       defs << ParameterDefinition(QString("I_%1").arg(i));
      
//       // Global film loss on the step
//       defs << ParameterDefinition(QString("kloss_%1").arg(i));

//       // Kinetic parameters:
//       for(int j = 0; j < speciesNumber; j++) {
//         for(int k = 0;  k < speciesNumber; k++) {
//           QString param;
//           if(j == k) // external loss
//             param = QString("k_%1_%2").arg(j).arg(i);
//           else
//             param = QString("k_%1%2_%3").arg(j).arg(k).arg(i);
//           defs << ParameterDefinition(param);
//         }
//       }
//     }
    
//     return defs;
//   };


//   virtual void function(const double * a,
//                         FitData * /*data*/,
//                         const DataSet * ds,
//                         gsl_vector * target)
//   {
//     const Vector & x = ds->x();

//     gsl_vector_complex * species = gsl_vector_complex_alloc(speciesNumber);
//     gsl_vector_complex * amplitudes = gsl_vector_complex_alloc(speciesNumber);


//     QVarLengthArray<gsl_vector_complex *, 30> eigenValues(distinctSteps);
//     QVarLengthArray<gsl_matrix *, 30> kineticMatrices(distinctSteps);
//     QVarLengthArray<gsl_matrix_complex *, 30> amplitudes2Species(distinctSteps);

//     /// LU decomposition of the amplitudes2Species stuff.
//     QVarLengthArray<gsl_matrix_complex *, 30> a2SLU(distinctSteps);
//     QVarLengthArray<gsl_permutation *, 30> a2Sperm(distinctSteps);

//     int currentStep = -1;

//     gsl_eigen_nonsymmv_workspace * workspace = 
//       gsl_eigen_nonsymmv_alloc(speciesNumber);



//     // Diagonalisation of the matrices.
//     for(int i = 0; i < distinctSteps; i++) {
//       kineticMatrices[i] = gsl_matrix_alloc(speciesNumber, speciesNumber);
//       eigenValues[i] = gsl_vector_complex_alloc(speciesNumber);

//       amplitudes2Species[i] = 
//         gsl_matrix_complex_alloc(speciesNumber, speciesNumber);
//       a2SLU[i] = gsl_matrix_complex_alloc(speciesNumber, speciesNumber);
//       a2Sperm[i] = gsl_permutation_alloc(speciesNumber);

//       int paramsPerStep = speciesNumber * speciesNumber + 2; 
//       /// (2 for kloss and I)


      

//       for(int j = 0; j < speciesNumber; j++) {
//         double sum = 0;
//         for(int k = 0; k < speciesNumber; k++) {
//           double val = a[steps.size() + paramsPerStep * i + 2 +
//                          j * speciesNumber + k];
//           if(k == j)
//             val += a[steps.size() + paramsPerStep * i + 1]; // kloss

//           sum += val;
//           gsl_matrix_set(kineticMatrices[i], j, k, val);
//         }
//         gsl_matrix_set(kineticMatrices[i], j, j, -sum); // Should work ?
//       }
      
//       // Now, we diagonalize the matrix
//       //
//       // We use a temporary complex matrix.
//       gsl_eigen_nonsymmv_params(0, workspace);
//       int status = gsl_eigen_nonsymmv(kineticMatrices[i],
//                                       eigenValues[i],  
//                                       amplitudes2Species[i], 
//                                       workspace);

//       /// @todo We need the 
//       if(status != GSL_SUCCESS) {
//         Terminal::out << "Failed to diagonalize matrix for conditions #" 
//                       << i << endl;
//         return;
//       }
      
//       gsl_matrix_complex_memcpy(a2SLU[i], amplitudes2Species[i]);
//       int sig;
//       gsl_linalg_complex_LU_decomp(a2SLU[i], a2Sperm[i], &sig);
      
//     }
//     gsl_eigen_nonsymmv_free(workspace);
    

//     for(int i = 0; i < x.size(); i++) {
      
      
//     }


//     gsl_vector_complex_free(species);
//     gsl_vector_complex_free(amplitudes);
//     for(int i = 0; i < distinctSteps; i++) {
//       gsl_vector_complex_free(eigenValues[i]);
//       gsl_matrix_free(kineticMatrices[i]);
//       gsl_matrix_complex_free(amplitudes2Species[i]);
//       gsl_matrix_complex_free(a2SLU[i]);
//       gsl_permutation_free(a2Sperm[i]);
//     }
//   };

//   virtual void initialGuess(FitData * params, 
//                             const DataSet *,
//                             double * a)
//   {
//     for(int i = 0; i < params->parameterDefinitions.size(); i++)
//       a[i] = 1;
//   };


//   KineticSystemFit() : PerDatasetFit("kinetic-system", 
//                                      "Kinetic system fit",
//                                      "", 1, -1, false) 
//   { 
//     ArgumentList * al = new 
//       ArgumentList(QList<Argument *>()
//                    << new IntegerArgument("species", 
//                                           "Number of species",
//                                           "Number of interconnected species")
//                    << new 
//                    SeveralIntegersArgument("steps", 
//                                            "Steps",
//                                            "Step list with numbered conditions"));
                   

//     makeCommands(al, 
//                  optionLessEffector(this, 
//                                     &KineticSystemFit::runFitCurrentDataSet),
//                  optionLessEffector(this, 
//                                     &KineticSystemFit::runFit));
//   };
// };

// static KineticSystemFit arbFit;


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
    for(int i = 0; i < data->parameterDefinitions.size(); i++)
      a[i] = 1;
  };


  ReactivationAmplitudeFit() : 
    Fit("inact-react-amplitude-2phases", 
        "...",
        "...", 3, 3) 
  { ;};


};

static ReactivationAmplitudeFit react;
