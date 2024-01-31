/*
  complexexpression.cc: implementation of the mathematical complexexpressions
  Copyright 2012, 2013, 2014 by CNRS/AMU

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
#include <complexexpression.hh>
#include <exceptions.hh>

#include <mruby.hh>

#include <utils.hh>


void ComplexExpression::buildArgs()
{
  MRuby * mr = MRuby::ruby();
  delete[] args;
  argsSize = effectiveMinVars.size();
  args = new mrb_value[argsSize];

  gsl_complex a;
  GSL_SET_COMPLEX(&a, 0, 0);
  for(int i = 0; i < argsSize; i++) {
    if(i == 0)
      args[i] = mr->newComplex(a);
    else
      args[i] = mr->newFloat(0.0);
    guard.protect(args[i]);
  }

  // Update the cache
  //
  // NOTE that this DOES NOT reflect the complex variable
  delete[] indexInVariables;
  indexInVariables = new int[argsSize];
  for(int i = 0; i < argsSize; i++) {
    indexInVariables[i] = -1;
    for(int j = 0; j < variables.size(); j++) {
      if(variables[j] == effectiveMinVars[i]) {
        indexInVariables[i] = j;
        break;
      }
    }
    if(indexInVariables[i] < 0)
      throw InternalError("One of the natural variables, '%1' was not found, shouldn't happen. Variables: %2").arg(effectiveMinVars[i]).arg(variables.join(","));
    
  }
}

void ComplexExpression::buildCode()
{
  Expression::buildCode();
  buildArgs();
}


ComplexExpression::ComplexExpression(const QString & variable,
                                     const QString & expr) :
  Expression(expr)
{
  effectiveMinVars = minimalVariables;
  effectiveMinVars.insert(0, variable);
  Utils::makeUnique(effectiveMinVars);
  Expression::setVariables(effectiveMinVars);
  buildArgs();
}


void ComplexExpression::setVariables(const QStringList & vars)
{
  variables = vars;
  variables.insert(0, variable);
  Utils::makeUnique(variables);
  buildCode();
}


mrb_value ComplexExpression::rubyEvaluation(const double * values) const
{
  // Should this be cached at the Complexexpression level ?
  MRuby * mr = MRuby::ruby();
  for(int i = 0; i < argsSize; i++) {
    if(i == 0) {
      gsl_complex * c = mr->complexInternal(args[i]);
      GSL_SET_COMPLEX(c, values[0], values[1]);
    }
    else
      // +1 since the first variable takes two spots
      SET_FLOAT_VALUE(mr->mrb, args[i], values[indexInVariables[i] + 1]);
  }

  mrb_value rv = mr->funcall(code, callSym(), argsSize, args);
  return rv;
}

mrb_value ComplexExpression::evaluateAsRuby(const double * values) const
{
  return rubyEvaluation(values);
}

gsl_complex ComplexExpression::evaluate(const double * values) const
{
  MRuby * mr = MRuby::ruby();
  MRubyArenaContext c(mr);  
  return mr->complexValue(rubyEvaluation(values));
}

