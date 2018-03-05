/*
  fwexpression.cc: implementation of the FWExpression class
  Copyright 2018 by CNRS/AMU

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
#include <fwexpression.hh>
#include <exceptions.hh>
#include <vector.hh>

#include <fitworkspace.hh>
#include <fitdata.hh>
#include <idioms.hh>
#include <mruby.hh>

#include <terminal.hh>

FWExpression::FWExpression(const QString & formula,
                           FitWorkspace * ws, const QStringList & extra) :
  workSpace(ws), extraParameters(extra)
{
  fitParameters = workSpace->parameterNames();
  realFormula = Expression::rubyIzeExpression(formula, fitParameters);
  finalParameters << "z" << "ds" << fitParameters;
  expr = new Expression(realFormula, finalParameters);
  if(extra.size() > 0)
    throw InternalError("Not implemented");
}

FWExpression::~FWExpression()
{
  delete expr;
}

mrb_value FWExpression::evaluate(int dataset, const double * extra)
{
  if(extra)
    throw InternalError("Not implemented");

  if(dataset < 0)
    dataset = workSpace->currentDataset();

  MRuby * mr = MRuby::ruby();


  QVarLengthArray<double, 200> params(finalParameters.size());
  params[0] = workSpace->perpendicularCoordinates.value(dataset, dataset);
  params[1] = dataset;
  Vector v = workSpace->saveParameterValues();

  int sz = fitParameters.size();
  for(int i = 0; i < sz; i++)
    params[2 + i] = v[sz * dataset + i];

  SaveGlobal p1("$params");
  mr->setGlobal("$params", workSpace->parametersToRuby(v));
  return expr->evaluateAsRuby(params.data());
}
