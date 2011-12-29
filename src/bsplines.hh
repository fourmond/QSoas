/**
   \file bsplines.hh
   The BSplines class, providing GSL-based filtering facilities
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

#ifndef __BSPLINES_HH
#define __BSPLINES_HH

#include <vector.hh>

#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>


/// This class provides filtering based on BSplines
class BSplines {

  /// The workspace for splines
  gsl_bspline_workspace * splinesWS;

  /// The workspace for fits:
  gsl_multifit_linear_workspace * fitWS;

  /// Vector space for the coefficients
  gsl_vector * coeffs;

  /// Storage space for the spline functions
  gsl_matrix * splines;

  /// Storage space for the covariance matrix
  gsl_matrix * cov;

  /// Total number of coefficients
  int nbCoeffs;

  /// Number of points
  int nb;

  /// Order of the splines
  int order;

  /// Positions of the breakPoints
  Vector breakPoints;

  /// X values of the data to filter
  Vector x;

  /// Y values of the data to filter
  Vector y;

  /// Frees all allocated memory
  void freeWS();

  /// Allocates all the necessary things
  void allocateWS();

public:

  BSplines(const Vector & xvalues, 
           const Vector & yvalues, int order = 4);

  ~BSplines();

  /// Sets the break points
  void setBreakPoints(const Vector & bps);

  /// Computes the coefficients, according to the current breakpoints;
  /// returns the residuals
  double computeCoefficients();

  /// Computes the Y values (once the coefficients have been computed)
  void computeValues(gsl_vector * target) const;

  /// Computes the Y values and return them as a new Vector.
  Vector computeValues() const;
};

#endif
