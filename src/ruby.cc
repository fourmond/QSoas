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


/// A global toogle. @todo Make that configurable at runtime
static SettingsValue<bool> debugRuby("ruby/debug", false);

VALUE Ruby::globalRescueFunction(VALUE /*dummy*/, VALUE exception)
{
  static VALUE cQSoasException = Qnil;
  if(cQSoasException == Qnil)
    cQSoasException = rb_eval_string("QSoasException");

  bool isQSoasException = false;
  if(rb_class_of(exception) == cQSoasException)
    isQSoasException = true;

  QString str = (isQSoasException ? "" : "Ruby exception: ");

  VALUE in = rb_inspect(exception);  // Probably shouldn't throw an exception ?
  str += StringValueCStr(in); // Or in ? See call stack too ?



  // We add the call stack
  if(debugRuby && (! isQSoasException) ) {
    VALUE ct = rb_funcall2(exception, rb_intern("backtrace"), 0, NULL);
    VALUE s = rb_str_new2("\n");
    VALUE s2 = rb_funcall2(ct, rb_intern("join"), 1, &s);
    str += "\n";
    str += StringValueCStr(s2);
  }
  QTextStream o(stderr);
  o << "Caught Ruby exception: " << str << endl;
  throw RuntimeError(str);
  return Qnil;
}


VALUE Ruby::exceptionSafeCall(VALUE (*function)(...), void * args)
{
  int error;
  VALUE ret = rb_protect((VALUE (*)(VALUE)) function, 
                         (VALUE) args, &error);
  if(error) {
    // An exception occurred, we need to handle it.
    VALUE exception = rb_gv_get("$!");
    return Ruby::globalRescueFunction(Qnil, exception);
  }
  return ret;
}



VALUE Ruby::main;

void Ruby::initRuby()
{
  ruby_init();
  main = rb_eval_string("self");
  rb_extend_object(main, rb_mMath);

  // Here, we define a whole bunch of constants that can be useful !

  VALUE mSpecial = GSLFunction::registerAllFunctions();
  GSLConstant::registerAllConstants();
  rb_extend_object(main, mSpecial);

  // Has to come last ?
  Ruby::loadFile(":/ruby/qsoas-base.rb");
  Ruby::loadFile(":/ruby/conditions.rb");
}

VALUE Ruby::loadFile(const QString & file)
{
  QFile f(file);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    return Qnil;  
  QByteArray bt = f.readAll();
  return rb_eval_string(bt);
}


VALUE Ruby::eval(QByteArray code)
{
  VALUE s = rb_str_new2(code.constData());
  return rb_funcall2(main, rb_intern("soas_eval"), 1, &s);
}

VALUE Ruby::makeBlock(QStringList * variables, const QByteArray & code)
{
  VALUE args[2];
  args[0] = rb_ary_new();
  for(int i = 0; i < variables->size(); i++)
    rb_ary_push(args[0], rb_str_new2(variables->value(i).toLocal8Bit().
                                 constData()));
  args[1] = rb_str_new2(code.constData());
  VALUE ret = rb_funcall2(main, rb_intern("soas_make_block"), 2, args);
  
  // Now get back the string values:
  variables->clear();
  int max = RARRAY_LEN(args[0]);
  for(int i = 0; i < max; i++) {
    VALUE v = rb_ary_entry(args[0], i);
    *variables << StringValueCStr(v);
  }
  return ret;
}

static VALUE toStringHelper(VALUE val, QString * target)
{
  VALUE strval = rb_obj_as_string(val);
  *target = StringValueCStr(strval);
  return Qnil;
}

QString Ruby::toQString(VALUE val)
{
  QString ret;
  Ruby::run(toStringHelper, val, &ret);
  return ret;
}

QString Ruby::inspect(VALUE val)
{
  QString ret;
  VALUE v = Ruby::run(rb_inspect, val);
  Ruby::run(toStringHelper, v, &ret);
  return ret;
}

VALUE Ruby::fromQString(const QString & str)
{
  return rb_str_new2(str.toLocal8Bit().constData());
}

QString Ruby::versionString()
{
  VALUE v = eval("RUBY_DESCRIPTION");
  return StringValueCStr(v);
}

static VALUE conv(VALUE val, double * tg)
{
  *tg = NUM2DBL(val);
  return Qnil;
}

double Ruby::toDouble(VALUE val)
{
  double v;
  Ruby::run(&conv, val, &v);
  return v;
}

