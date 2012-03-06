/*
  ruby.cc: implementation of (some of) the ruby communication code
  Copyright 2010, 2011 by Vincent Fourmond

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

#include <exceptions.hh>

VALUE Ruby::globalRescueFunction(VALUE /*dummy*/, VALUE exception)
{
  rb_p(exception);
  QString str = QObject::tr("Ruby exception: ");
  VALUE in = rb_inspect(exception);  // Probably shouldn't throw an exception ?
  str += StringValueCStr(exception); // Or in ? See call stack too ?
  throw RuntimeError(str);
  return Qnil;
}

/// A few useful functions
static const char * ruby_init_code = 
  "def soas_eval(__str)\n"
  "  begin\n"
  "    eval(__str)\n"
  "  rescue SyntaxError => __e\n"
  "    raise \"Syntax error: #{__e.to_s}\"\n"
  "  end\n"
  "end\n\n"
  "def soas_make_block(__vars, __code)\n"
  "  __done = true\n"
  "  __blk = nil\n"
  "  begin\n" 
  "    __blk = eval \"proc do |#{__vars.join(',')}|\\n#{__code}\\nend\"\n"
  "    __tmp = [1.0] * __vars.size\n"
  "    __blk.call(*__tmp)\n"
  "    __done = true\n"
  "  rescue NameError => __e\n"
  "    if __e.is_a? NoMethodError\n"
  "      raise __e\n"
  "    end\n"
  "    __vars << __e.name.to_s\n"
  "    __done = false\n"
  "  rescue SyntaxError => __e\n"
  "    raise \"Syntax error: #{__e.to_s}\"\n"
  "  end while ! __done\n"
  "  return __blk\n"
  "end\n";

VALUE Ruby::main;

void Ruby::initRuby()
{
  ruby_init();
  main = rb_eval_string("self");
  rb_extend_object(main, rb_mMath);
  rb_eval_string(ruby_init_code);
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

