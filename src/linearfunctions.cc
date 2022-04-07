/*
  linearfunctions.cc: implementation of the LinearFunction hierarchy
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
#include <linearfunctions.hh>

#include <exceptions.hh>

#include <gsl/gsl_multifit.h>
#include <gsl/gsl_cdf.h>
#include <gsl-types.hh>

LinearFunction::~LinearFunction()
{
}




void LinearFunction::computeJacobian(gsl_matrix * jacobian) const
{
  int nbp = parameters();
  double params[nbp];
  gsl_vector_view p = gsl_vector_view_array(params, nbp);
  gsl_vector_set_zero(&p.vector);
  for(int i = 0; i < nbp; i++) {
    params[i] = 1;
    gsl_vector_view v = gsl_matrix_column(jacobian, i);
    computeFunction(&p.vector, &v.vector);
    params[i] = 0;
  }
}

double LinearFunction::solve(const gsl_vector * func, gsl_vector * params,
                             gsl_vector * errors, gsl_matrix * covar) const

{
  gsl_multifit_linear_workspace * ws =
    gsl_multifit_linear_alloc(dataPoints(), parameters());
  GSLMatrix jac(dataPoints(), parameters());
  GSLMatrix cov(parameters(), parameters());

  computeJacobian(jac);

  double chisq = 0;
  // Zero before ? Might have an influence...
  gsl_vector_set_zero(params);
  gsl_matrix_set_zero(cov);
  int status = gsl_multifit_linear(jac, func,
                                   params, cov, &chisq,
                                   ws);
  gsl_multifit_linear_free(ws);
  if(status != GSL_SUCCESS)
    throw RuntimeError("Could not solve linear least squares problem: %1").
      arg(gsl_strerror(status));

  if(errors) {
    double conf = gsl_cdf_tdist_Pinv(0.975, dataPoints() - parameters());
    for(int i = 0; i < parameters(); i++)
      gsl_vector_set(errors, i, sqrt(gsl_matrix_get(cov, i, i)) * conf);
  }
  if(covar)
    gsl_matrix_memcpy(covar, cov);
  return chisq;
}

//////////////////////////////////////////////////////////////////////

#include <dataset.hh>
#include <datasetexpression.hh>

DataSetExpressionFunction::DataSetExpressionFunction(DataSetExpression * expr,
                                                     const QString & form,
                                                     const DataSet * ds) :
  expression(expr), dataset(ds), formula(form)
{
  size = DataSetExpression::dataSetParameters(dataset).size();
  expression->prepareExpression(formula, 0, &params);
}



QStringList DataSetExpressionFunction::parameterNames() const
{
  return params;
}

int DataSetExpressionFunction::parameters() const
{
  return params.size();
}

int DataSetExpressionFunction::dataPoints() const
{
  return dataset->nbRows();
}

void DataSetExpressionFunction::computeFunction(const gsl_vector * pms,
                                                gsl_vector * target) const
{
  QVarLengthArray<double, 1000> storage(size + parameters());
  int idx = 0;
  expression->reset();
  for(int i = 0; i < parameters(); i++)
    storage[size+i] = gsl_vector_get(pms, i);
  while(expression->nextValues(storage.data(), &idx)) {
      gsl_vector_set(target, idx, 
                     expression->expression().
                     evaluateNoLock(storage.data()));
  }

}
