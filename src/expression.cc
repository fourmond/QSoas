/*
  expression.cc: implementation of the mathematical expressions
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
#include <expression.hh>
#include <exceptions.hh>
#include <ruby.hh>
#include <ruby-templates.hh>

ID Expression::callIDCache = 0;

ID Expression::callID()
{
  if(callIDCache == 0)
    callIDCache = rb_intern("call");
  return callIDCache;
}

VALUE Expression::codeSafeKeepingHash()
{
  VALUE hsh = rb_gv_get("$expression_codes");
  if(! RTEST(hsh)) {
    hsh = rb_hash_new();
    rb_gv_set("$expression_codes", hsh);
  }
  return hsh;
}

VALUE Expression::argsSafeKeepingHash()
{
  VALUE hsh = rb_gv_get("$expression_args");
  if(! RTEST(hsh)) {
    hsh = rb_hash_new();
    rb_gv_set("$expression_args", hsh);
  }
  return hsh;
}


VALUE Expression::hashKey()
{
  long val = reinterpret_cast<long>(this);
  val >>= 1;
  return INT2FIX(val);
}

void Expression::buildArgs()
{
  delete[] args;
  args = new VALUE[variables.size()];

  VALUE ary = rb_ary_new();
  rb_hash_aset(argsSafeKeepingHash(), hashKey(), ary);

  for(int i = 0; i < variables.size(); i++) {
    VALUE db = rb_float_new(0);
    rb_ary_push(ary, db);
    args[i] = db;
  }
}

/// @bug This function raises exceptions while being called from the
/// constructor, which may lead to memory leaks.
void Expression::buildCode()
{
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
  rb_hash_aset(codeSafeKeepingHash(), hashKey(), code);

  buildArgs();                  // Build the arguments cache
}

void Expression::freeCode()
{
  rb_hash_delete(codeSafeKeepingHash(), hashKey());
  rb_hash_delete(argsSafeKeepingHash(), hashKey());
  code = Qnil;
  delete[] args;
  args = NULL;
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
  expression(c.expression), minimalVariables(c.minimalVariables), 
  variables(c.variables), args(NULL)
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

VALUE Expression::rubyEvaluation(const double * values) const
{
  int size = variables.size();
  for(int i = 0; i < size; i++)
    RFLOAT_VALUE(args[i]) = values[i];
  VALUE ret = Ruby::run(&rb_funcall2, code, 
                        callID(), size, (const VALUE *) args);
  return ret;
}

double Expression::evaluate(const double * values) const
{
  return NUM2DBL(rubyEvaluation(values));
}

bool Expression::evaluateAsBoolean(const double * values) const
{
  return RTEST(rubyEvaluation(values));
}

int Expression::evaluateIntoArray(const double * values, 
                                  double * target, int ts) const
{
  int size = variables.size();
  for(int i = 0; i < size; i++)
    RFLOAT_VALUE(args[i]) = values[i];
  VALUE ret = Ruby::run(&rb_funcall2, code, 
                        callID(), size, (const VALUE *) args);

  // Now, we parse the return value
  if(rb_obj_class(ret) != rb_cArray) {
    if(ts >= 1)
      *target = NUM2DBL(ret);
    return 1;
  }
  int sz = RARRAY_LEN(ret);
  for(int i = 0; i < ts && i < sz ; i++)
    target[i] = NUM2DBL(rb_ary_entry(ret, i));

  return sz;
}

QStringList Expression::variablesNeeded(const QString & expression,
                                        const QStringList & variables)
{
  QStringList vars = variables;
  QByteArray bta = expression.toLocal8Bit();
  VALUE code = 
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
  ret[0] = ret[0].toLower();  // avoid using constants !
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
