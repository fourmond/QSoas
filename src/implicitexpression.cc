/*
  implicitexpression.cc: implementation of the ImplicitExpression class
  Copyright 2020, 2024 by CNRS/AMU

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
#include <implicitexpression.hh>

#include <expression.hh>

#include <utils.hh>
#include <exceptions.hh>
#include <solver.hh>



ImplicitExpression::ImplicitExpression(const QString & form) :
  formula(form),
  expression(NULL),
  reporterExpression(NULL),
  seedExpression(NULL),
  variable("y"),
  storage(NULL)
{
  
}

ImplicitExpression::~ImplicitExpression()
{
  clear();
}

void ImplicitExpression::clear()
{
  delete expression;
  delete reporterExpression;
  delete seedExpression;
  expression = NULL;
  reporterExpression = NULL;
  seedExpression = NULL;
}

void ImplicitExpression::prepare()
{
  clear();

  QRegExp re(";fit=(.*)");
  QString reporter, equation, seed;
  int idx = re.indexIn(formula);
  if(idx >= 0) {
    equation = formula.left(idx);
    reporter = re.cap(1);
  }
  else
    equation = formula;

  QRegExp reS("^seed=([^;]+);");
  if(reS.indexIn(equation) == 0) {
    equation = equation.mid(reS.cap(0).size());
    seed = reS.cap(1);
  }
  
  
  // Look for all the parameters in the expression
  /// @todo Make that a function in Expression ?
  QStringList exprs;
  QSet<QString> strs;
  exprs << equation << reporter << seed;
  for(const QString & s : exprs) {
    if(s.isEmpty())
      continue;
    Expression sexp(s);
    strs += sexp.naturalVariables().toSet();
  }
  naturalVariables = strs.toList();
  std::sort(naturalVariables.begin(), naturalVariables.end());
  naturalVariables.insert(0, variable);
  Utils::makeUnique(naturalVariables);
}

QStringList ImplicitExpression::variables() const
{
  if(! expression)
    throw InternalError("Using variables() on an unprepared ImplicitExpression");
  return naturalVariables.mid(1);
}

double ImplicitExpression::f(double val)
{
  storage[0] = val;
  return expression->evaluate(storage);
}



void ImplicitExpression::setVariables(const QStringList & variables)
{
  if(! expression)
    throw InternalError("Using setVariables() on an unprepared ImplicitExpression");
  if(variables.indexOf(variable) >= 0)
    throw InternalError("Setting variables that contain the solver variable");

  QStringList fnl = variables;
  fnl.insert(0, variable);
  for(Expression * exp: {expression, reporterExpression, seedExpression}) {
    if(exp)
      exp->setVariables(fnl);
  }
}

bool ImplicitExpression::requiresSeed() const
{
  if(! expression)
    throw InternalError("Using requiresSeed() on an unprepared ImplicitExpression");
  return seedExpression == NULL;
}


double ImplicitExpression::solve(double seed, double * parameters)
{
  if(! expression)
    throw InternalError("Using solve() on an unprepared ImplicitExpression");
  storage = parameters;
  double val = Solver::solve(seed);
  parameters[0] = val;
  if(reporterExpression)
    return reporterExpression->evaluate(parameters);
  return val;
}

double ImplicitExpression::solve(double left, double right,
                                 double * parameters)
{
  if(! expression)
    throw InternalError("Using solve() on an unprepared ImplicitExpression");
  storage = parameters;
  double val = Solver::solve(left, right);
  parameters[0] = val;
  if(reporterExpression)
    return reporterExpression->evaluate(parameters);
  return val;
}

double ImplicitExpression::solve(double * parameters)
{
  if(! seedExpression)
    throw InternalError("This ImplicitExpression has no seed");
  return solve(seedExpression->evaluate(parameters), parameters);
}
