/**
   \file linearfunctions.hh
   The LinearFunction class hierarchy, to simplify the resolution of linear
   least square problems
   Copyright 2022 by CNRS/AMU

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
#ifndef __LINEARFUNCTIONS_HH
#define __LINEARFUNCTIONS_HH

#include <vector.hh>

class DataSetExpression;
class DataSet;


/// A base class for defining linear least square problems and solving
/// them.
class LinearFunction {
public:

  virtual ~LinearFunction();

  /// Number of parameters
  virtual int parameters() const = 0;

  /// Number of data points
  virtual int dataPoints() const = 0;

  /// Compute the function for the given set of parameters
  virtual void computeFunction(const gsl_vector * parameters,
                               gsl_vector * target) const = 0;

  /// Computes the jacobian of the system
  virtual void computeJacobian(gsl_matrix * jacobian) const;


  /// Computes the solution to the linear least squares problem,
  /// storing the parameters found into @a parameters, and optionally
  /// computing the errors and the covariance matrix.
  /// Returns the chi square.
  double solve(const gsl_vector * func, gsl_vector * parameters,
               gsl_vector * errors = NULL, gsl_matrix * covar = NULL) const;

  /// Variant of solve taking into account weights.
  double weightedSolve(const gsl_vector * func, const gsl_vector * weights,
                       gsl_vector * parameters,
                       gsl_vector * errors = NULL, gsl_matrix * covar = NULL);    
};


/// A linear function based on a dataset expression. No guarantee is
/// made on the actual linearity.
class DataSetExpressionFunction : public LinearFunction {
protected:
  DataSetExpression * expression;

  const DataSet * dataset;

  QString formula;

  QStringList params;

  int size;

public:

  /// Constructs the linear function based on the given expression
  /// (and the corresponding dataset)
  ///
  /// Doesn't take ownership of the expression
  DataSetExpressionFunction(DataSetExpression * expr, const QString & formula,
                            const DataSet * ds);

  QStringList parameterNames() const;

  virtual int parameters() const override;
  virtual int dataPoints() const override;
  virtual void computeFunction(const gsl_vector * parameters,
                               gsl_vector * target) const override;

};

/// A simple linear function based on simple polynomials of a given
/// order on a set of X values
class PolynomialFunction : public LinearFunction {
  int order;

  Vector xValues;

public:

  PolynomialFunction(int order, const Vector & x);


  virtual int parameters() const override;
  virtual int dataPoints() const override;
  virtual void computeFunction(const gsl_vector * parameters,
                               gsl_vector * target) const override;

  
  
};



#endif
