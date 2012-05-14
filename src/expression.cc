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

ID Expression::callIDCache = 0;

ID Expression::callID()
{
  if(callIDCache == 0)
    callIDCache = rb_intern("call");
  return callIDCache;
}

VALUE Expression::safeKeepingHash()
{
  VALUE hsh = rb_gv_get("$expression_codes");
  if(! RTEST(hsh)) {
    hsh = rb_hash_new();
    rb_gv_set("$expression_codes", hsh);
  }
  return hsh;
}


VALUE Expression::hashKey()
{
  long val = reinterpret_cast<long>(this);
  val >>= 1;
  return INT2FIX(val);
}

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
    throw RuntimeError("Not all the variables needed");
  rb_hash_aset(safeKeepingHash(), hashKey(), code);
}

void Expression::freeCode()
{
  rb_hash_delete(safeKeepingHash(), hashKey());
  code = Qnil;
}

Expression::Expression(const QString & expr) :
  expression(expr)
{
  buildCode();
}

Expression::Expression(const QString & expr, const QStringList & vars) :
  expression(expr)
{
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
  variables(c.variables)
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

double Expression::evaluate(const double * variables) const
{
  /// @todo Add a cache for Ruby Floats as large as the number of
  /// variables. (in the same spirit as the "cache" for code)
  return 0;
}

