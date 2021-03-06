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

#include <exceptions.hh>
#include <utils.hh>
#include <debug.hh>

LinearKineticSystem::LinearKineticSystem(int species) : 
  speciesNumber(species), updateNeeded(true)
{
  coordinates = gsl_vector_complex_alloc(speciesNumber);
  eigenValues = gsl_vector_complex_alloc(speciesNumber);
  storage1 = gsl_vector_complex_alloc(speciesNumber);
  storage2 = gsl_vector_complex_alloc(speciesNumber);


  kineticMatrix = gsl_matrix_alloc(speciesNumber, speciesNumber);
  savedKineticMatrix = gsl_matrix_alloc(speciesNumber, speciesNumber);
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
  gsl_matrix_free(savedKineticMatrix);
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
  gsl_matrix_transpose(kineticMatrix); // Yep !
  gsl_matrix_memcpy(savedKineticMatrix, kineticMatrix);
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
    Debug::debug()
      << "Diagonalization failed for matrix:\n" 
      << Utils::matrixString(savedKineticMatrix)
      << "\nThe process went as far as:\n"
      << Utils::matrixString(kineticMatrix)
      << endl;
    throw RuntimeError(QString("Failed to diagonalize kinetic matrix: only got %1 eigenvalues").arg(workspace->nonsymm_workspace_p->n_evals));
  }
      
  gsl_matrix_complex_memcpy(eVLU, eigenVectors);
  int sig;
  gsl_linalg_complex_LU_decomp(eVLU, eVPerm, &sig);
  gsl_eigen_nonsymmv_free(workspace);
  updateNeeded = false;
}

void LinearKineticSystem::setInitialConcentrations(const double * 
                                                   concentrations)
{
  gsl_vector_const_view v = 
    gsl_vector_const_view_array(concentrations, speciesNumber);
  setInitialConcentrations(&v.vector);
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

  // Dont forget to multiply by the coordinates !
  gsl_vector_complex_mul(storage1, coordinates);

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
