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
    for(int i = 0; i < argsSize; i++)
      mr->gcUnregister(args[i]);
  }
  delete[] args;
  argsSize = minimalVariables.size();
  args = new mrb_value[argsSize];

  for(int i = 0; i < argsSize; i++) {
    args[i] = mr->newFloat(0.0);
    // printf("Argument #%d -- %p: ", i, args[i]);
    // mrb_p(mr->mrb, args[i]);
    // DUMP_MRUBY(args[i]);
    mr->gcRegister(args[i]);
  }

  // Update the cache
  delete[] indexInVariables;
  indexInVariables = new int[argsSize];
  for(int i = 0; i < argsSize; i++) {
    indexInVariables[i] = -1;
    for(int j = 0; j < variables.size(); j++) {
      if(variables[j] == minimalVariables[i]) {
        indexInVariables[i] = j;
        break;
      }
    }
    if(indexInVariables[i] < 0)
      throw InternalError("One of the natural variables, '%1' was not found, shouldn't happen. Variables: %2").arg(minimalVariables[i]).arg(variables.join(","));
    
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
  // QMutexLocker m(&Ruby::rubyGlobalLock);
  singleVariableIndex = -1;

  if(variables.isEmpty() && minimalVariables.isEmpty()) {
    minimalVariables = variablesNeeded(expression);
    variables = minimalVariables;
  }
  else {
    // Sanity check: make sure the variables contain each of the minimalvariables
    
    MRuby * mr = MRuby::ruby();
    localVariables.clear();
    minimalVariables = mr->detectParameters(expression.toLocal8Bit(),
                                            &localVariables);

    QSet<QString> v = variables.toSet();
    // If a local is in the "variables", then it really is a variable,
    // not a local. Examples:
    // @li x += 2
    // @li x = sin(x)
    // 
    // In both these cases, Ruby is happy because the local x is
    // defined... But it can't work.
    for(const QString & loc : localVariables)
      if(v.contains(loc))
        minimalVariables << loc;
    
    QSet<QString> mv = minimalVariables.toSet();
    mv.subtract(v);
    if(mv.size() > 0) {
      QStringList l = mv.toList();
      l.sort();
      throw RuntimeError("The following variables are needed but not defined: '%1'").arg(l.join("', '"));
    }
    
  }


  //////////////////////////////////////////////////
  ///// Optimizations !

  /// @todo Maybe code optimization could end up in a separate function ? 

  // Now, we find out if the expression is reduced to a single
  // variable:
  if(minimalVariables.size() == 1) {
    if(expression.trimmed() == minimalVariables[0]) {
      // Then, we have a single variable, whose index we'll find
      while(true) {
        ++singleVariableIndex;
        if(variables[singleVariableIndex] == minimalVariables[0])
          break;
      }
    }
  }

  MRuby * mr = MRuby::ruby();
  code = mr->makeBlock(expression.toLocal8Bit(), minimalVariables);
  // printf("%p -> %p\n", this, code);
  // DUMP_MRUBY(code);
  mr->gcRegister(code);
  buildArgs();                  // Build the arguments cache
}

void Expression::freeCode()
{
  MRuby * mr = MRuby::ruby();
  if(args) {
    for(int i = 0; i < argsSize; i++)
      mr->gcUnregister(args[i]);
  }
  delete[] args;
  args = NULL;
  mr->gcUnregister(code);
  delete[] indexInVariables;
  code = mrb_nil_value();
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
  double a = 0 /* never used */;
  ex.evaluateIntoArray(&a, target, pm.size());
}


Expression::Expression(const QString & expr) :
  expression(expr), args(NULL), argsSize(0), indexInVariables(NULL)
{
  buildCode();
}

Expression::Expression(const QString & expr, const QStringList & vars, 
                       bool skip) :
  expression(expr), args(NULL), indexInVariables(NULL)
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
  indexInVariables(NULL),
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

const QStringList & Expression::locals() const
{
  return localVariables;
}

mrb_value Expression::rubyEvaluation(const double * values) const
{
  // Should this be cached at the Expression level ?
  MRuby * mr = MRuby::ruby();
  for(int i = 0; i < argsSize; i++)
    SET_FLOAT_VALUE(mr->mrb, args[i], values[indexInVariables[i]]);

  mrb_value rv = mr->funcall(code, callSym(), argsSize, args);
  return rv;
}

mrb_value Expression::evaluateAsRuby(const double * values) const
{
  return rubyEvaluation(values);
}

double Expression::evaluate(const double * values) const
{
  // QMutexLocker m(&Ruby::rubyGlobalLock);
  return evaluateNoLock(values);
}

double Expression::evaluateNoLock(const double * values) const
{
  if(singleVariableIndex >=  0)
    return values[singleVariableIndex];
  MRuby * mr = MRuby::ruby();
  MRubyArenaContext c(mr);  
  return mr->floatValue(rubyEvaluation(values));
}

bool Expression::evaluateAsBoolean(const double * values) const
{
  MRubyArenaContext c(MRuby::ruby());  
  return mrb_test(rubyEvaluation(values));
}

int Expression::evaluateIntoArray(const double * values, 
                                  double * target, int ts) const
{
  // QMutexLocker m(&Ruby::rubyGlobalLock);
  return evaluateIntoArrayNoLock(values, target, ts);
}

int Expression::evaluateIntoArrayNoLock(const double * values, 
                                  double * target, int ts) const
{
  MRuby * mr = MRuby::ruby();
  MRubyArenaContext c(mr);  
  mrb_value ret = rubyEvaluation(values);
  
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
  // QMutexLocker m(&Ruby::rubyGlobalLock);
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
