/*
  rubysolver.cc: implementation of the RubySolver class
  Copyright 2013 by CNRS/AMU

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
#include <rubysolver.hh>

#include <expression.hh>
#include <exceptions.hh>

RubySolver::RubySolver(const QString & expr, 
                       const gsl_root_fdfsolver_type * type) :
  Solver(type)
{
  formula = new Expression(expr);
  // We check later that the expression is monovariate to avoid
  // throw-in-constructor idioms...
  nbVariables = formula->naturalVariables().size();
}

RubySolver::~RubySolver()
{
  delete formula;
}

double RubySolver::f(double x)
{
  if(nbVariables != 1)
    throw RuntimeError("Need only one variable from formula %1").
      arg(formula->formula());
  return formula->evaluate(&x);
}
