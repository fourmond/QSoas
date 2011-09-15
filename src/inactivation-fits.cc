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
