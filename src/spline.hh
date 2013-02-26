/**
   \file spline.hh
   The Spline class, providing GSL-based interpolation facilities
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
#ifndef __SPLINE_HH
#define __SPLINE_HH

#include <vector.hh>

/// This class provides an interface to the spline interpolation
/// functions from the GSL. Just insert points into it and get the
/// interpolated values back using evaluate().
///
/// As stated, it provides \b interpolation, which in principle
/// precludes from returning data from outside of the range of points
/// entered. However, as a convenience, the Y values are extrapolated
/// to the boundary by taking the closest value.
///
/// @todo It would be clever to cache the interpolation... (but I
/// don't think recomputation costs a lot).
class Spline {
  
  /// The X -> Y values
  QMap<double, double> values;

public:
  
  Spline() {;};

  /// Returns the data points currently in.
  QList<QPointF> pointList() const;

  /// Inserts the given point.
  void insert(const QPointF & point);

  /// Removes the point the closest to the given abcissa
  void remove(double x);

  /// Remove all points
  void clear() { values.clear();};

  typedef enum {
    Linear,
    Polynomial,
    CSpline,
    Akima,
    SplineTypes
  } Type;

  /// Computes the values for the given X vector.
  ///
  /// Returns an empty vector if there wasn't enough points to compute
  /// an interpolation. As stated above, if the X values span outside
  /// the range given, 
  Vector evaluate(const Vector & xvalues, Type type = CSpline);

  /// Evaluates the first derivative of the interpolation.
  ///
  /// @todo integration
  Vector derivative(const Vector & xvalues, Type type = CSpline);

  /// Evaluates the second derivative of the interpolation.
  Vector secondDerivative(const Vector & xvalues, Type type = CSpline);

private:
  const gsl_interp_type * interpolationType(Type type) const;

  /// Prepare the points
  void preparePoints(Vector * x, Vector * xvals, Vector * yvals);
  
  /// Prepare the interpolation
  /// 
  /// Returns NULL if there aren't enough data points.
  gsl_interp * prepareInterpolant(Type t, const Vector & xvals, 
                                  const Vector & yvals);

  Vector innerEvaluation(const Vector & xvalues, Type type,
                         double (*f)(const gsl_interp *, const double x[], 
                                     const double ya[], double, gsl_interp_accel *));

  

};

#endif
