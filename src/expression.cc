/*
  expression.cc: implementation of the mathematical expressions
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
#include <expression.hh>
#include <exceptions.hh>
#include <ruby.hh>
#include <ruby-templates.hh>

#include <mruby.hh>

mrb_sym Expression::callSymCache = 0;

mrb_sym Expression::callSym()
{
  if(callSymCache == 0) {
    MRuby * mr = MRuby::ruby();
    callSymCache = mr->intern("call");
  }
  return callSymCache;
}

void Expression::buildArgs()
{
  MRuby * mr = MRuby::ruby();
  if(args) {
    for(int i = 0; i < variables.size(); i++)
      mr->gcUnregister(args[i]);
  }
  delete[] args;
  args = new mrb_value[variables.size()];

  for(int i = 0; i < variables.size(); i++) {
    args[i] = mr->newFloat(0.0);
    mr->gcRegister(args[i]);
  }
}

bool Expression::isAVariable() const
{
  return singleVariableIndex >= 0;
}

/// @bug This function raises exceptions while being called from the
/// constructor, which may lead to memory leaks.
void Expression::buildCode()
{
  QMutexLocker m(&Ruby::rubyGlobalLock);
  singleVariableIndex = -1;

  if(variables.isEmpty() && minimalVariables.isEmpty()) {
    minimalVariables = variablesNeeded(expression);
    variables = minimalVariables;
  }

  MRuby * mr = MRuby::ruby();
  code = mr->makeBlock(expression.toLocal8Bit(), variables);
  mr->gcRegister(code);
  buildArgs();                  // Build the arguments cache
}

void Expression::freeCode()
{
  MRuby * mr = MRuby::ruby();
  mr->gcUnregister(code);
  code = mrb_nil_value();
  if(args) {
    for(int i = 0; i < variables.size(); i++)
      mr->gcUnregister(args[i]);
  }
  delete[] args;
  args = NULL;
}

void Expression::setParametersFromExpression(const QStringList & params,
                                             const QString &expression,
                                             double * target, 
                                             bool forceDefault,
                                             double def)
{
  if(params.size() == 0)
    return;                     // Nothing to do
  QStringList pm = params;
  QString expr = rubyIzeExpression(expression, pm);

  QStringList beg;
  for(int i = 0; i < pm.size(); i++)
    beg << QString("%1 = %2").arg(pm[i]).
      arg(forceDefault ? def : target[i], 0, 'g', 20);
  QString final = QString("%1\n%2\n[%3]").
    arg(beg.join("\n")).arg(expr).arg(pm.join(", "));

  Expression ex(final);
  if(ex.minimalVariables.size() > 0)
    throw RuntimeError("Undefined parameters: '%1'").
      arg(ex.minimalVariables.join("', '"));
  double a;
  ex.evaluateIntoArray(&a, target, pm.size());
}


Expression::Expression(const QString & expr) :
  expression(expr), args(NULL)
{
  buildCode();
}

Expression::Expression(const QString & expr, const QStringList & vars, 
                       bool skip) :
  expression(expr), args(NULL)
{
  if(! skip)
    buildCode();
  setVariables(vars);
}

void Expression::setVariables(const QStringList & vars)
{
  variables = vars;
  buildCode();
}

Expression::Expression(const Expression & c) :
  expression(c.expression), args(NULL), 
  minimalVariables(c.minimalVariables), variables(c.variables)
{
  buildCode();
}

Expression::~Expression()
{
  freeCode();
}

const QStringList & Expression::currentVariables() const
{
  return variables;
}

const QStringList & Expression::naturalVariables() const
{
  return minimalVariables;
}

mrb_value Expression::rubyEvaluation(const double * values) const
{
  // Should this be cached at the Expression level ?
  MRuby * mr = MRuby::ruby();
  int size = variables.size();
  for(int i = 0; i < size; i++)
    SET_FLOAT_VALUE(mr->mrb, args[i], values[i]);
  
  return mr->funcall(code, callSym(), size, args);
}

double Expression::evaluate(const double * values) const
{
  QMutexLocker m(&Ruby::rubyGlobalLock);
  return evaluateNoLock(values);
}

double Expression::evaluateNoLock(const double * values) const
{
  // if(singleVariableIndex >=  0)
  //   return values[singleVariableIndex];
  MRuby * mr = MRuby::ruby();
  return mr->floatValue(rubyEvaluation(values));
}

bool Expression::evaluateAsBoolean(const double * values) const
{
  return mrb_test(rubyEvaluation(values));
}

int Expression::evaluateIntoArray(const double * values, 
                                  double * target, int ts) const
{
  QMutexLocker m(&Ruby::rubyGlobalLock);
  return evaluateIntoArrayNoLock(values, target, ts);
}

int Expression::evaluateIntoArrayNoLock(const double * values, 
                                  double * target, int ts) const
{
  MRuby * mr = MRuby::ruby();
  mrb_value ret =  rubyEvaluation(values);
  
  // Now, we parse the return value
  if(! mr->isArray(ret)) {
    if(ts >= 1)
      *target = mr->floatValue(ret);
    return 1;
  }

  int sz = mr->arrayLength(ret);
  sz = std::min(ts, sz);
  for(int i = 0; i < sz ; i++)
    target[i] = mr->floatValue(mr->arrayRef(ret, i));
  return sz;
}

Vector Expression::evaluateAsArray(const double * values) const
{
  QMutexLocker m(&Ruby::rubyGlobalLock);
  return evaluateAsArrayNoLock(values);
}


Vector Expression::evaluateAsArrayNoLock(const double * values) const
{
  MRuby * mr = MRuby::ruby();
  mrb_value ret =  rubyEvaluation(values);

  Vector tg;
    
  // Now, we parse the return value
  if(! mr->isArray(ret)) {
    tg << mr->floatValue(ret);
  }
  else {
    int sz = mr->arrayLength(ret);
    for(int i = 0; i < sz ; i++)
      tg << mr->floatValue(mr->arrayRef(ret, i));
  }
  return tg;
}

QStringList Expression::variablesNeeded(const QString & expression,
                                        const QStringList & variables)
{
  MRuby * mr = MRuby::ruby();
  QStringList v = mr->detectParameters(expression.toLocal8Bit());
  for(int i = 0; i < variables.size(); i++) {
    int idx = v.indexOf(variables[i]);
    if(idx >= 0)
      v.takeAt(idx);
  }
  return v;
}


/// @questions Shall we replace simply everything that cannot be part
/// of an identifier ? This would allow basically everything !
QString Expression::rubyIzeName(const QString & name)
{
  QString ret = name;
  // For now, conversions are simple
  // This is looking for clashes if two parameters differ only by the case
  // of the first letter
  if(ret[0].isUpper())
    ret = "_up_" + ret;

  ret.replace(QRegExp("[#/-]"), "_"); 
  return ret;
}

QString Expression::rubyIzeExpression(const QString & expr, 
                                      QStringList & variables)
{
  QStringList oldVars = variables;
  QList<QRegExp> replacements;
  QString ret = expr;
  for(int i = 0; i < oldVars.size(); i++) {
    variables[i] = rubyIzeName(oldVars[i]);
    ret.replace(QRegExp("(^|\\W)" + QRegExp::escape(oldVars[i]) + "($|\\W)"), 
                QString("\\1") + variables[i] + QString("\\2"));
  }
  return ret;
}
