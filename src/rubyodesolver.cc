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

RubyODESolver::RubyODESolver(const QString & init, const QString & der) :
  initialization(NULL), derivatives(NULL)
{
  parseSystem(init, der);
}

RubyODESolver::RubyODESolver() : 
  initialization(NULL), derivatives(NULL)
{
}


void RubyODESolver::setParameterValues(const QString & formula)
{
  if(extraParams.size() == 0)
    return;

  QStringList defs;
  for(int i = 0; i < extraParams.size(); i++) {
    const QString & n = extraParams[i];
    defs << QString("%1 = 0").arg(n);
  }

  QString code = QString("%3\n%2\n[%1]").arg(extraParams.join(",")).
    arg(formula).
    arg(defs.join("\n"));
  
  Expression ex(code);
  double v;
  ex.evaluateIntoArray(&v, extraParamsValues.data(), 
                       extraParams.size());
}


void RubyODESolver::parseFromFile(QIODevice * file)
{
  QStringList lines = Utils::parseConfigurationFile(file);
  QRegExp blnk("^\\s*$");
  int idx = lines.indexOf(blnk);
  if(idx < 0)
    throw RuntimeError("File does not contain two sections "
                       "separated by a fully blank line");
  
  QStringList init = lines.mid(0, idx);
  QStringList derivs = lines.mid(idx+1);
  parseSystem(init.join("\n"), derivs.join("\n"));
}

void RubyODESolver::parseSystem(const QString & init, const QString & der)
{
  delete initialization;
  initialization = NULL;
  delete derivatives;
  derivatives = NULL;

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
  allv << "t" << Expression::variablesNeeded(init)
       << Expression::variablesNeeded(der, vars);

  Utils::makeUnique(allv);

  extraParams = allv;
  extraParams.takeAt(0);

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

  // We initialize all the extram parameters to 0
  extraParamsValues = Vector(extraParams.size(), 0);
}


int RubyODESolver::dimension() const
{
  return vars.size();
}

int RubyODESolver::computeDerivatives(double t, const double * y, 
                                      double * dydt)
{
  double v[vars.size() + extraParams.size() + 1];
  int j = 0;
  v[j++] = t;
  for(int i = 0; i < extraParams.size(); i++)
    v[j++] = extraParamsValues[i];
  for(int i = 0; i < vars.size(); i++)
    v[j++] = y[i];

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
  double tg[vars.size()];
  double v[extraParams.size() + 1];
  int j = 0;
  v[j++] = t;
  for(int i = 0; i < extraParams.size(); i++)
    v[j++] = extraParamsValues[i];
  initialization->evaluateIntoArray(v, tg, vars.size());
  ODESolver::initialize(tg, t);
}

