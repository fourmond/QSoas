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



void LinearFunction::computeJacobian(gsl_matrix * jacobian)
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
                             gsl_vector * errors, gsl_matrix * covar)

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
  return chisq;
}
