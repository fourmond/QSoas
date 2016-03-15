/*
  ruby.cc: implementation of (some of) the ruby communication code
  Copyright 2010, 2011 by Vincent Fourmond
            2012, 2013, 2014 by CNRS/AMU

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
#include <ruby.hh>
#include <ruby-templates.hh>
#include <gslfunction.hh>
#include <settings-templates.hh>

#include <exceptions.hh>

QMutex Ruby::rubyGlobalLock(QMutex::Recursive);

/// A global toogle. @todo Make that configurable at runtime
static SettingsValue<bool> debugRuby("ruby/debug", false);

RUBY_VALUE Ruby::globalRescueFunction(RUBY_VALUE /*dummy*/, RUBY_VALUE exception)
{
  static RUBY_VALUE cQSoasException = rbw_nil;
  if(cQSoasException == rbw_nil)
    cQSoasException = rbw_eval_string("QSoasException");

  bool isQSoasException = false;
  if(rbw_class_of(exception) == cQSoasException)
    isQSoasException = true;

  QString str = (isQSoasException ? "" : "Ruby exception: ");

  RUBY_VALUE in = rbw_inspect(exception);  // Probably shouldn't throw an exception ?
  str += rbw_string_value_cstr(in); // Or in ? See call stack too ?



  // We add the call stack
  if(debugRuby && (! isQSoasException) ) {
    RUBY_VALUE ct = rbw_funcall2(exception, rbw_intern("backtrace"), 0, NULL);
    RUBY_VALUE s = rbw_str_new2("\n");
    RUBY_VALUE s2 = rbw_funcall2(ct, rbw_intern("join"), 1, &s);
    str += "\n";
    str += rbw_string_value_cstr(s2);
  }
  QTextStream o(stderr);
  o << "Caught Ruby exception: " << str << endl;
  throw RuntimeError(str);
  return rbw_nil;
}


RUBY_VALUE Ruby::exceptionSafeCall(RUBY_VALUE (*function)(...), void * args)
{
  int error = 0;
  RUBY_VALUE ret = rbw_protect((RUBY_VALUE (*)(RUBY_VALUE)) function, 
                         (RUBY_VALUE) args, &error);
  if(error) {
    // An exception occurred, we need to handle it.
    RUBY_VALUE exception = rbw_gv_get("$!");
    return Ruby::globalRescueFunction(rbw_nil, exception);
  }
  return ret;
}



RUBY_VALUE Ruby::main;

void Ruby::initRuby()
{
  ruby_wrappers_init();
  main = rbw_eval_string("self");
  rbw_extend_object(main, rbw_mMath());

  // Here, we define a whole bunch of constants that can be useful !

  RUBY_VALUE mSpecial = GSLFunction::registerAllFunctions();
  GSLConstant::registerAllConstants();
  rbw_extend_object(main, mSpecial);

  // Has to come last ?
  Ruby::loadFile(":/ruby/qsoas-base.rb");
  Ruby::loadFile(":/ruby/conditions.rb");
}

RUBY_VALUE Ruby::loadFile(const QString & file)
{
  QFile f(file);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    return rbw_nil;  
  QByteArray bt = f.readAll();
  return rbw_eval_string(bt);
}


RUBY_VALUE Ruby::eval(QByteArray code)
{
  RUBY_VALUE s = rbw_str_new2(code.constData());
  return rbw_funcall2(main, rbw_intern("soas_eval"), 1, &s);
}

RUBY_VALUE Ruby::safeEval(const QString & code)
{
  QByteArray bt = code.toLocal8Bit();
  return Ruby::run(Ruby::eval, bt);
}

RUBY_VALUE Ruby::makeBlock(QStringList * variables, const QByteArray & code)
{
  RUBY_VALUE args[2];
  args[0] = rbw_ary_new();
  for(int i = 0; i < variables->size(); i++)
    rbw_ary_push(args[0], rbw_str_new2(variables->value(i).toLocal8Bit().
                                 constData()));
  args[1] = rbw_str_new2(code.constData());
  RUBY_VALUE ret = rbw_funcall2(main, rbw_intern("soas_make_block"), 2, args);
  
  // Now get back the string values:
  variables->clear();
  int max = rbw_array_length(args[0]);
  for(int i = 0; i < max; i++) {
    RUBY_VALUE v = rbw_ary_entry(args[0], i);
    *variables << rbw_string_value_cstr(v);
  }
  return ret;
}

static RUBY_VALUE toStringHelper(RUBY_VALUE val, QString * target)
{
  RUBY_VALUE strval = rbw_obj_as_string(val);
  *target = rbw_string_value_cstr(strval);
  return rbw_nil;
}

QString Ruby::toQString(RUBY_VALUE val)
{
  QString ret;
  Ruby::run(toStringHelper, val, &ret);
  return ret;
}

static RUBY_VALUE toDoubleHelper(RUBY_VALUE val, double * target)
{
  *target = rbw_num2dbl(val);
  return rbw_nil;
}

QVariant Ruby::toQVariant(RUBY_VALUE val)
{
  if(rbw_is_numeric(val)) {
    double v;
    Ruby::run(toDoubleHelper, val, &v);
    return v;
  }
  QString ret;
  Ruby::run(toStringHelper, val, &ret);
  return ret;
}

QString Ruby::inspect(RUBY_VALUE val)
{
  QString ret;
  RUBY_VALUE v = Ruby::run(rbw_inspect, val);
  Ruby::run(toStringHelper, v, &ret);
  return ret;
}

RUBY_VALUE Ruby::fromQString(const QString & str)
{
  return rbw_str_new2(str.toLocal8Bit().constData());
}

QString Ruby::versionString()
{
  RUBY_VALUE v = eval("RUBY_DESCRIPTION");
  return rbw_string_value_cstr(v);
}

static RUBY_VALUE conv_f(RUBY_VALUE val, double * tg)
{
  *tg = rbw_num2dbl(val);
  return rbw_nil;
}

double Ruby::toDouble(RUBY_VALUE val)
{
  double v;
  Ruby::run(&conv_f, val, &v);
  return v;
}

static RUBY_VALUE conv_i(RUBY_VALUE val, long * tg)
{
  *tg = rbw_num2int(val);
  return rbw_nil;
}

long Ruby::toInt(RUBY_VALUE val)
{
  long v;
  Ruby::run(&conv_i, val, &v);
  return v;
}

static RUBY_VALUE ruby2CHelper(QString str, QStringList * tgt)
{
  RUBY_VALUE args[2];
  args[0] = Ruby::fromQString(str);
  args[1] = rbw_ary_new();
  
  RUBY_VALUE ret = rbw_funcall2(Ruby::main, rbw_intern("soas_ruby2c"), 2, args);

  if(tgt) {
    int max = rbw_array_length(args[1]);
    for(int i = 0; i < max; i++) {
      RUBY_VALUE v = rbw_ary_entry(args[1], i);
      *tgt << rbw_string_value_cstr(v);
    }
  }

  return ret;
}

QString Ruby::ruby2C(const QString & str, QStringList * vars)
{
  RUBY_VALUE ret = Ruby::run(ruby2CHelper, str, vars);
  return toQString(ret);
}

