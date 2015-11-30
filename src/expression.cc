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

RUBY_ID Expression::callIDCache = 0;

RUBY_ID Expression::callID()
{
  if(callIDCache == 0)
    callIDCache = rbw_intern("call");
  return callIDCache;
}

RUBY_VALUE Expression::codeSafeKeepingHash()
{
  RUBY_VALUE hsh = rbw_gv_get("$expression_codes");
  if(! rbw_test(hsh)) {
    hsh = rbw_hash_new();
    rbw_gv_set("$expression_codes", hsh);
  }
  return hsh;
}

RUBY_VALUE Expression::argsSafeKeepingHash()
{
  RUBY_VALUE hsh = rbw_gv_get("$expression_args");
  if(! rbw_test(hsh)) {
    hsh = rbw_hash_new();
    rbw_gv_set("$expression_args", hsh);
  }
  return hsh;
}


RUBY_VALUE Expression::hashKey()
{
  long val = reinterpret_cast<long>(this);
  val >>= 1;
  return rbw_int(val);
}

void Expression::buildArgs()
{
  delete[] args;
  args = new RUBY_VALUE[variables.size()];

  RUBY_VALUE ary = rbw_ary_new();
  rbw_hash_aset(argsSafeKeepingHash(), hashKey(), ary);

  for(int i = 0; i < variables.size(); i++) {
    RUBY_VALUE db = rbw_float_new(0.0);
    rbw_ary_push(ary, db);
    args[i] = db;
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
  singleVariableIndex = -1;

  QStringList vars = variables;
  QByteArray bta = expression.toLocal8Bit();
  code = 
    Ruby::run<QStringList *, const QByteArray &>(&Ruby::makeBlock, &vars, bta);
  
  if(variables.isEmpty() && minimalVariables.isEmpty()) {
    minimalVariables = vars;
    variables = vars;
  }
  else if(vars.size() > variables.size())
    throw RuntimeError(QString("Not all the variables needed: %1 vs %2").
                       arg(vars.join(", ")).arg(variables.join(", ")));
  rbw_hash_aset(codeSafeKeepingHash(), hashKey(), code);


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

  buildArgs();                  // Build the arguments cache
}

void Expression::freeCode()
{
  rbw_hash_delete(codeSafeKeepingHash(), hashKey());
  rbw_hash_delete(argsSafeKeepingHash(), hashKey());
  code = rbw_nil;
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

RUBY_VALUE Expression::rubyEvaluation(const double * values) const
{
  // Greatly simplified code in case the expression reduces to only
  // one variable !
  if(singleVariableIndex >=  0)
    return rbw_float_new(values[singleVariableIndex]);
  int size = variables.size();
  for(int i = 0; i < size; i++)
    rbw_set_float(&args[i],values[i]);
  
  RUBY_VALUE ret = Ruby::run(&rbw_funcall2, code, 
                        callID(), size, (const RUBY_VALUE *) args);
  return ret;
}

double Expression::evaluate(const double * values) const
{
  if(singleVariableIndex >=  0)
    return values[singleVariableIndex];
  return Ruby::toDouble(rubyEvaluation(values));
}

bool Expression::evaluateAsBoolean(const double * values) const
{
  return rbw_test(rubyEvaluation(values));
}

int Expression::evaluateIntoArray(const double * values, 
                                  double * target, int ts) const
{
  RUBY_VALUE ret = rubyEvaluation(values);

  // Now, we parse the return value
  if(! rbw_is_array(ret)) {
    if(ts >= 1)
      *target = Ruby::toDouble(ret);
    return 1;
  }
  int sz = rbw_array_length(ret);
  for(int i = 0; i < ts && i < sz ; i++)
    target[i] = Ruby::toDouble(rbw_ary_entry(ret, i));

  return sz;
}


Vector Expression::evaluateAsArray(const double * values) const
{
  RUBY_VALUE ret = rubyEvaluation(values);

  Vector tg;
  // Now, we parse the return value
  if(! rbw_is_array(ret))
    tg << Ruby::toDouble(ret);
  else {
    int sz = rbw_array_length(ret);
    for(int i = 0; i < sz ; i++)
      tg << Ruby::toDouble(rbw_ary_entry(ret, i));
  }
  return tg;
}

QStringList Expression::variablesNeeded(const QString & expression,
                                        const QStringList & variables)
{
  QStringList vars = variables;
  QByteArray bta = expression.toLocal8Bit();
  // RUBY_VALUE code = 
  Ruby::run<QStringList *, const QByteArray &>(&Ruby::makeBlock, &vars, bta);
  for(int i = 0; i < variables.size(); i++)
    vars.takeAt(0);
  return vars;
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
