/**
   \file linearkineticsystem.hh 
   The LinearKineticSystem, a system for computing the evolution of
   linear kinetic systems with arbitrary number of species.

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
#ifndef __LINEARKINETICSYSTEM_HH
#define __LINEARKINETICSYSTEM_HH

/// A class that provides the solution to an arbitrary linear kinetic
/// system.
class LinearKineticSystem {

  /// The number of species
  int speciesNumber;

  /// Whether or not we should recompute matrices
  bool updateNeeded;

  /// The matrix of evolution of the system
  gsl_matrix * kineticMatrix;


  /// The matrix of evolution of the system, saved from the ravages of
  /// the eigen values search (for debugging purposes)
  gsl_matrix * savedKineticMatrix;

  /// The eigenvalues of the system
  gsl_vector_complex * eigenValues;
  
  /// The eigenvectors of the system
  gsl_matrix_complex * eigenVectors;

  /// The LU decomp of eigenVectors (for inverting)
  gsl_matrix_complex * eVLU;

  /// The corresponding permutatin
  gsl_permutation * eVPerm;

  /// Initial coordinates in the eigenVector space.
  gsl_vector_complex * coordinates;

  /// Temporary storage of complex values.
  gsl_vector_complex * storage1;
  gsl_vector_complex * storage2;
  

  /// Computes the whole system
  void computeMatrices();
public:

  LinearKineticSystem(int species);

  ~LinearKineticSystem();

  /// Sets the values of the kineticConstants to the given values.
  ///
  /// The additionalDiagonalTerm is added to the intrisic decay
  /// constants of each state (it is convenient to have it for fits
  /// like KineticSystemFit.
  void setConstants(const double * values, double additionalDiagonalTerm = 0);

  /// Sets the initial concentrations
  void setInitialConcentrations(const gsl_vector * concentrations);

  /// Sets the initial concentrations
  void setInitialConcentrations(const double * concentrations);

  /// Returns in \a target the concentrations of the species at time
  /// t.
  void getConcentrations(double t, gsl_vector * target) const;

  /// Returns the kinetic matrix
  const gsl_matrix * kineticMatrixValue() const {
    return kineticMatrix;
  };

  /// Returns the kinetic matrix
  const gsl_matrix_complex * eigenVectorsValue() const {
    return eigenVectors;
  };

  /// Returns the kinetic matrix
  const gsl_vector_complex * eigenValuesVector() const {
    return eigenValues;
  };

  /// Returns the coordinates
  const gsl_vector_complex * coordinateVector() const {
    return coordinates;
  };
  
  
};

#endif
