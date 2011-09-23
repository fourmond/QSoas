/*
  linearkineticsystem.cc: implementation of LinearKineticSystem
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
#include <linearkineticsystem.hh>

#include <terminal.hh>

#include <soas.hh>
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <vector.hh>
#include <dataset.hh>


LinearKineticSystem::LinearKineticSystem(int species) : 
  speciesNumber(species), updateNeeded(true)
{
  coordinates = gsl_vector_complex_alloc(speciesNumber);
  eigenValues = gsl_vector_complex_alloc(speciesNumber);
  storage1 = gsl_vector_complex_alloc(speciesNumber);
  storage2 = gsl_vector_complex_alloc(speciesNumber);


  kineticMatrix = gsl_matrix_alloc(speciesNumber, speciesNumber);
  eigenVectors = gsl_matrix_complex_alloc(speciesNumber, speciesNumber);
  eVLU = gsl_matrix_complex_alloc(speciesNumber, speciesNumber);

  eVPerm = gsl_permutation_alloc(speciesNumber);
}

LinearKineticSystem::~LinearKineticSystem()
{
  gsl_vector_complex_free(coordinates);
  gsl_vector_complex_free(eigenValues);
  gsl_vector_complex_free(storage1);
  gsl_vector_complex_free(storage2);


  gsl_matrix_free(kineticMatrix);
  gsl_matrix_complex_free(eigenVectors);
  gsl_matrix_complex_free(eVLU);

  gsl_permutation_free(eVPerm);
}


void LinearKineticSystem::setConstants(const double * values, 
                                       double additionalDiagonalTerm)
{
  for(int j = 0; j < speciesNumber; j++) {
    double sum = 0;
    for(int k = 0; k < speciesNumber; k++) {
      double val = values[j * speciesNumber + k];
      if(k == j)
        val += additionalDiagonalTerm;
      sum += val;
      gsl_matrix_set(kineticMatrix, j, k, val);
    }
    gsl_matrix_set(kineticMatrix, j, j, -sum); // Should work ?
  }
  updateNeeded = true;
}


void LinearKineticSystem::computeMatrices()
{
  ///  @todo raise appropriate exceptions when some of the gsl things
  ///  failed ?

  gsl_eigen_nonsymmv_workspace * workspace = 
    gsl_eigen_nonsymmv_alloc(speciesNumber);

  gsl_eigen_nonsymmv_params(0, workspace);
  int status = gsl_eigen_nonsymmv(kineticMatrix,
                                  eigenValues,  
                                  eigenVectors, 
                                  workspace);

  if(status != GSL_SUCCESS) {
    Terminal::out << "Failed to diagonalize matrix" << endl;
    return;
  }
      
  gsl_matrix_complex_memcpy(eVLU, eigenVectors);
  int sig;
  gsl_linalg_complex_LU_decomp(eVLU, eVPerm, &sig);
  gsl_eigen_nonsymmv_free(workspace);
  updateNeeded = false;
}

void LinearKineticSystem::setInitialConcentrations(const gsl_vector * 
                                                   concentrations)
{
  if(updateNeeded)
    computeMatrices();

  for(int i = 0; i < speciesNumber; i++) {
    gsl_complex c;
    GSL_SET_COMPLEX(&c, gsl_vector_get(concentrations, i), 0);
    gsl_vector_complex_set(coordinates, i, c);
  }

  // Compute the solution in-place

  /// @todo return value checking
  gsl_linalg_complex_LU_svx(eVLU, eVPerm, coordinates);
}

void LinearKineticSystem::getConcentrations(double t, 
                                            gsl_vector * target) const
{
  // First, we set the storage1 to the arguments of the exponentials
  gsl_vector_complex_memcpy(storage1, eigenValues);
  gsl_complex ct;
  GSL_SET_COMPLEX(&ct, t, 0);
  
  gsl_vector_complex_scale(storage1, ct);

  // Now exponentiate the vector.
  for(int i = 0; i < speciesNumber; i++)
    gsl_vector_complex_set(storage1, i, 
                           gsl_complex_exp(gsl_vector_complex_get(storage1, i)));

  // Switch back to concentrations

  gsl_complex c2;
  GSL_SET_COMPLEX(&ct, 1, 0);
  GSL_SET_COMPLEX(&c2, 0, 0);
  
  gsl_blas_zgemv(CblasNoTrans, ct, eigenVectors,
                 storage1, c2, storage2);

  for(int i = 0; i < speciesNumber; i++) {
    c2 = gsl_vector_complex_get(storage2, i);
    gsl_vector_set(target, i, GSL_REAL(c2));
    /// @todo check the imaginary part isn't too big
  }
  
}


//////////////////////////////////////////////////////////////////////


// This command may just be temporary. In any case, it's main purpose
// is to test the kinetic simulations.

static ArgumentList 
kSArgs(QList<Argument *>() 
       << new IntegerArgument("species", 
                              "Number of species")
       << new SeveralNumbersArgument("constants", 
                                     "Kinetic constants")
       );

static void kineticSystemCommand(const QString &, int specs,
                                 QList<double> consts)
{
  LinearKineticSystem system(specs);
  QVarLengthArray<double, 30> init(specs);
  QVarLengthArray<double, 900> constants(specs*specs);

  for(int i = 0; i < specs; i++) {
    if(i)
      init[i] = 0;
    else
      init[i] = 1;
  }

  for(int i = 0; i < constants.size(); i++)
    constants[i] = consts.value(i, 0);

  gsl_vector_view v = gsl_vector_view_array(init.data(), specs);
  system.setConstants(constants.data());
  system.setInitialConcentrations(&v.vector);

  // Now, we have all we need;
  
  Vector xvals;
  QVarLengthArray<Vector, 30> values(specs);
  for(int i = 0; i < 100; i++) {
    double t = i * 0.1;         // ??
    xvals << t;
    system.getConcentrations(t, &v.vector);
    for(int j = 0; j < specs; j++)
      values[j] << init[j];
  }
  

  {
    for(int i = 0; i < specs; i++) {
      QList<Vector> v;
      v << xvals;
      v << values[i];
      DataSet * ds = new DataSet(v);
      ds->name = QString("simulations_species_%1.dat").arg(i);
      soas().pushDataSet(ds);
    }
  }
}

static Command 
p("kinetic-system", // command name
  optionLessEffector(kineticSystemCommand), // action
  "file",  // group name
  &kSArgs, // arguments
  NULL, // options
  "Kinetic system simulations",
  "...",
  "...");
