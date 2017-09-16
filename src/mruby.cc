/*
  mruby.cc: implementation of the MRuby classes
  Copyright 2017 by CNRS/AMU

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
#include <mruby.hh>

#include <mruby/compile.h>
#include <mruby/string.h>
#include <mruby/error.h>

#include <exceptions.hh>

MRuby::MRuby()
{
  mrb = mrb_open();
}

MRuby::~MRuby()
{
  mrb_close(mrb);
}


MRuby * MRuby::globalInterpreter = NULL;

MRuby * MRuby::ruby()
{
  if(! globalInterpreter)
    globalInterpreter = new MRuby;
  return globalInterpreter;
}



QString MRuby::inspect(mrb_value object)
{
  mrb_value v = mrb_inspect(mrb, object);
// mrb_obj_as_string(mrb, object);
  
  QString rv(mrb_string_value_cstr(mrb, &v));
  return rv;
}


static mrb_value protect_helper(mrb_state * ,mrb_value helper)
{
  const std::function<mrb_value ()> * f =
    reinterpret_cast<const std::function<mrb_value ()> *>(mrb_cptr(helper));
  return (*f)();
    
}


mrb_value MRuby::protect(const std::function<mrb_value ()> &function)
{
  mrb_bool failed;
  mrb_value helper;
  const void * v = &function;
  SET_CPTR_VALUE(mrb, helper, const_cast<void*>(v));
  mrb->exc = NULL;
  helper = mrb_protect(mrb, &::protect_helper, helper, &failed);
  // fprintf(stdout, "Return value: %d, %p\n", failed, mrb->exc);
  if(mrb->exc) {
    mrb_value exc = mrb_obj_value(mrb->exc);
    fprintf(stdout, "MRuby error: \n");
    mrb_print_error(mrb);
    mrb_p(mrb, exc);
    mrb_value bt = mrb_funcall(mrb, exc, "backtrace", 0);
    // mrb_value bt = mrb_funcall(mrb, exc, "message", 0);
    mrb_p(mrb, bt);
    throw RuntimeError("A ruby exception occurred: %1").arg(inspect(mrb_exc_backtrace(mrb, exc)));
  }

  return helper;
}


struct RProc * MRuby::generateCode(const QByteArray & code)
{
  struct mrbc_context * c = mrbc_context_new(mrb);
  c->capture_errors = true;
  struct mrb_parser_state * p =
    mrb_parse_string(mrb, code.constData(), c);
  if(p->nerr > 0) {
    // We have a syntax error
    QString message;
    // Point to exact code place ?
    for(size_t i = 0; i < p->nerr; i++) {
      if(i > 0)
        message += "\n";
      message += QString("%1 at line %2:%3").
        arg(p->error_buffer[i].message).
        arg(p->error_buffer[i].lineno).
        arg(p->error_buffer[i].column);
    }
    mrbc_context_free(mrb, c);
    mrb_parser_free(p);
    throw RuntimeError("Syntax error: %1").arg(message);
  }
  RProc * proc = mrb_generate_code(mrb, p);
  mrbc_context_free(mrb, c);
  mrb_parser_free(p);
  return proc;
}

mrb_value MRuby::eval(const QByteArray & code)
{
  RProc * proc = generateCode(code);
  return protect([this, proc]() -> mrb_value {
      return mrb_run(mrb, proc, mrb_top_self(mrb));
    }
    );
}




#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <terminal.hh>
//////////////////////////////////////////////////////////////////////

static void mRubyEval(const QString &, QString code,
                      const CommandOptions & )
{
  MRuby * r = MRuby::ruby();
  Terminal::out << " => " << r->inspect(r->eval(code.toLocal8Bit()))
                << endl;
}

static ArgumentList 
eA(QList<Argument *>() 
   << new StringArgument("code", 
                         "Code",
                         "Any ruby code"));



static Command 
ev("mruby-eval", // command name
   effector(mRubyEval), // action
   "file",  // group name
   &eA, // arguments
   NULL,
   "Ruby eval",
   "Evaluates a Ruby expression and prints the result", "");
