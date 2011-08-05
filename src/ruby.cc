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

VALUE Ruby::globalRescueFunction(VALUE /*dummy*/, VALUE exception)
{
  rb_p(exception);
  QString str = QObject::tr("Ruby exception: ");
  VALUE in = rb_inspect(exception);  // Probably shouldn't throw an exception ?
  str += StringValueCStr(exception); // Or in ? See call stack too ?
  throw std::runtime_error(str.toStdString());
  return Qnil;
}

void Ruby::initRuby()
{
  ruby_init();
  VALUE main = rb_eval_string("self");
  rb_extend_object(main, rb_mMath);
  rb_eval_string("def soas_eval(str)\n"
                 "  begin\n"
                 "    eval(str)\n"
                 "  rescue SyntaxError => e\n"
                 "    raise \"Syntax error: #{e.to_s}\"\n"
                 "  end\n"
                 "end\n");
}

/// @todo Just to remember later: I need a function that converts a
/// "formula" into a block, while at the same time detecting all
/// missing arguments by catching the NameError exception. That should
/// be plain ruby code, as it will turn out to be delicate in C++.
VALUE Ruby::eval(const char * str)
{
  VALUE s = rb_str_new2(str);
  VALUE main = rb_eval_string("self");
  return rb_funcall2(main, rb_intern("soas_eval"), 1, &s);
}
