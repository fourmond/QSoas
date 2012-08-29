/*
  rubyodesolver.cc: implementation of the ruby-based ODE solver
  Copyright 2012 by Vincent Fourmond

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
#include <rubyodesolver.hh>

#include <utils.hh>


RubyODESolver::RubyODESolver(const QString & init, const QString & der)
{
  // First, we look at the initialization to find out which variables
  // are necessary.

  // Initialization can be a function of t.

  QStringList statements = init.split(QRegExp("[;\n]"));
  QRegExp assignRE("\\s*(\\w+)\\s*=");
  for(int i = 0; i < statements.size(); i++)
    if(assignRE.indexIn(statements[i]) >= 0)
      vars << assignRE.cap(1);

  Utils::makeUnique(vars);
  // Should be fine.

  QString code = QString("%2\n[%1]").arg(vars.join(",")).arg(init);

  
  QStringList allv;
  allv << "t";
  initialization = new Expression(code, allv);

  // Now, we prepare the derivatives
  QStringList derivs;
  QStringList inits;
  for(int i = 0; i < vars.size(); i++) {
    QString n = QString("d_%1").arg(vars[i]);
    derivs << n;
    inits << QString("%1 = 0").arg(n);
  }
  code = QString("%2\n%3\n[%1]").arg(derivs.join(",")).
    arg(inits.join("\n")).arg(der);

  allv << vars;
  derivatives = new Expression(code, allv);

}

int RubyODESolver::dimension() const
{
  return vars.size();
}

int RubyODESolver::computeDerivatives(double t, const double * y, 
                                      double * dydt)
{
  double v[vars.size() + 1];
  v[0] = t;
  for(int i = 0; i < vars.size(); i++)
    v[i+1] = y[i];

  derivatives->evaluateIntoArray(v, dydt, vars.size());
  
  return GSL_SUCCESS;
}

RubyODESolver::~RubyODESolver()
{
  delete initialization;
  delete derivatives;
}

void RubyODESolver::initialize(double t)
{
  double v[vars.size()];
  initialization->evaluateIntoArray(&t, v, vars.size());
  ODESolver::initialize(v, t);
}
