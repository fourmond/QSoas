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
#include <mruby/opcode.h>
#include <mruby/proc.h>
#include <mruby/string.h>
#include <mruby/error.h>
#include <mruby/array.h>
#include <mruby/numeric.h>
#include <mruby/variable.h>
#include <mruby/hash.h>

#include <exceptions.hh>
#include <utils.hh>

#include <gslfunction.hh>

MRuby::MRuby()
{
  mrb = mrb_open();
  cQSoasInterface = NULL;
}

MRuby::~MRuby()
{
  mrb_close(mrb);
}


void MRuby::dumpValue(const char * t, int n, const char * s, mrb_value v)
{
  printf("%s:%d:\n%s = %p\n", t, n, s, v);
}



MRuby * MRuby::globalInterpreter = NULL;

MRuby * MRuby::ruby()
{
  if(! globalInterpreter) {
    // Here is where the initialization of the global interpreter is
    // done.
    globalInterpreter = new MRuby;
    globalInterpreter->eval("include Math");
    GSLFunction::registerAllFunctions(globalInterpreter);
    GSLConstant::registerAllConstants(globalInterpreter);

    globalInterpreter->initializeInterface();
    globalInterpreter->initializeRegexp();
  }
  return globalInterpreter;
}



QString MRuby::inspect(mrb_value object)
{
  mrb_value v = mrb_inspect(mrb, object);
  // DUMP_MRUBY(v);
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
  if(mrb->exc) {
    mrb_value exc = mrb_obj_value(mrb->exc);
    throw RuntimeError("A ruby exception occurred: %1").arg(inspect(exc));
  }

  return helper;
}


struct RProc * MRuby::generateCode(const QByteArray & code,
                                   const QString & fileName)
{
  struct mrbc_context * c = mrbc_context_new(mrb);
  c->capture_errors = true;
  QByteArray fn = fileName.toLocal8Bit();
  mrbc_filename(mrb, c, fn.constData());
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

mrb_value MRuby::eval(const char * code)
{
  return eval(QByteArray(code));
}

mrb_value MRuby::eval(const QByteArray & code)
{
  RProc * proc = generateCode(code);
  return protect([this, proc]() -> mrb_value {
      return mrb_run(mrb, proc, mrb_top_self(mrb));
    }
    );
}

mrb_value MRuby::eval(const QString & code)
{
  return eval(code.toLocal8Bit());
}

mrb_value MRuby::eval(QIODevice * device)
{
  QByteArray code = device->readAll();
  RProc * proc = generateCode(code, Utils::fileName(device));
  return protect([this, proc]() -> mrb_value {
      return mrb_run(mrb, proc, mrb_top_self(mrb));
    }
    );
}

QStringList MRuby::detectParameters(const QByteArray & code)
{
  RProc * proc = generateCode(code);
  struct mrb_irep * irep = proc->body.irep;

  // The idea is just to scan the code and detect funcalls of single
  // arguments from what seems to be top level self.
  int cur_top_self = -1;
  QSet<QString> rv;
  for (int i = 0; i < (int)irep->ilen; i++) {
    mrb_code c = irep->iseq[i];
    if(GET_OPCODE(c) == OP_SEND) {
      if(GETARG_A(c) == cur_top_self && GETARG_C(c) == 0)
        rv.insert(mrb_sym2name(mrb, irep->syms[GETARG_B(c)]));
    }
    if(GET_OPCODE(c) == OP_LOADSELF) {
      cur_top_self = GETARG_A(c);
    }
    else {
      if(GETARG_A(c) == cur_top_self)
        cur_top_self = -1;
    }
  }
  return rv.toList();
}

struct RClass * MRuby::defineModule(const char * name)
{
  return mrb_define_module(mrb, name);
}

void MRuby::defineModuleFunction(struct RClass* cls, const char* name,
                                 mrb_func_t func, mrb_aspec aspec)
{
  mrb_define_module_function(mrb, cls, name, func, aspec);
}

void MRuby::defineGlobalConstant(const char *name, mrb_value val)
{
  mrb_define_global_const(mrb, name, val);
}

void MRuby::gcRegister(mrb_value obj)
{
  // DUMP_MRUBY(obj);
  mrb_gc_register(mrb, obj);
  // mrb_sym sym = mrb_intern_lit(mrb, "$__qsoas_safe");
  // mrb_value v = mrb_gv_get(mrb, sym);
  // if(mrb_nil_p(v)) {
  //   v = mrb_hash_new(mrb);
  //   mrb_gv_set(mrb, sym, v);
  // }
  // mrb_value k = mrb_hash_get(mrb, v, obj);
  // int nb = 0;
  // if(! mrb_nil_p(k))
  //   nb = mrb_fixnum(k);
  // mrb_hash_set(mrb, v, obj, mrb_fixnum_value(nb + 1));
}

void MRuby::gcUnregister(mrb_value obj)
{
  // DUMP_MRUBY(obj);
  mrb_gc_unregister(mrb, obj);
}

mrb_sym MRuby::intern(const char * symbol)
{
  return mrb_intern_cstr(mrb, symbol);
}

mrb_value MRuby::funcall_up(mrb_value self, mrb_sym func, mrb_int nb,
                            const mrb_value * params)
{
  return mrb_funcall_argv(mrb, self, func, nb, params);
}

mrb_value MRuby::funcall(mrb_value self, mrb_sym func, mrb_int nb,
                         const mrb_value * params)
{
  return protect([this, self, func, nb, params]() -> mrb_value {
      return funcall_up(self, func, nb, params);
    }
    );
}

mrb_value MRuby::newFloat(double value)
{
  return mrb_float_value(mrb, value);
}


double MRuby::floatValue(mrb_value value)
{
  double rv;
  protect([this, value, &rv]() -> mrb_value {
      rv = floatValue_up(value);
      return mrb_nil_value();
    }
    );
  return rv;
}

double MRuby::floatValue_up(mrb_value value)
{
  return mrb_to_flo(mrb, value);
}


mrb_value MRuby::newInt(int value)
{
  return mrb_fixnum_value(value);
}

mrb_value MRuby::makeBlock(const QString & code, const QStringList & vars)
{
  if(vars.size() > 16) {
    // We need to use a catch-all block
    QString assigns;
    for(int i = 0; i < vars.size(); i++)
      assigns += QString("  %1 = args[%2]\n").arg(vars[i]).arg(i);
    return eval(QString("proc do |*args|\n%1  %2\nend").
                arg(assigns).arg(code));
  }
  else
    return eval(QString("proc do |%1|\n  %2\nend").
                arg(vars.join(",")).arg(code));
}

mrb_value MRuby::newArray()
{
  return mrb_ary_new(mrb);
}

bool MRuby::isArray(mrb_value array)
{
    return mrb_obj_is_kind_of(mrb, array, mrb->array_class);
}

mrb_value MRuby::arrayRef(mrb_value array, int index)
{
  return mrb_ary_ref(mrb, array, index);
}

void MRuby::arrayPush(mrb_value array, mrb_value val)
{
  mrb_ary_push(mrb, array, val);
}

int MRuby::arrayLength(mrb_value array)
{
  return RARRAY_LEN(array);
}

void MRuby::arrayIterate(mrb_value array,
                         const std::function <void (mrb_value)> & func)
{
  if(! isArray(array))
    throw RuntimeError("Expecting a Ruby array, but got: '%1'").
      arg(inspect(array));
  
  int nb = arrayLength(array);
  for(int i = 0; i < nb; i++) {
    mrb_value v = arrayRef(array, i);
    func(v);
  }
}


void MRuby::setGlobal(const char * name, mrb_value val)
{
  mrb_sym sym = mrb_intern_cstr(mrb, name);
  mrb_gv_set(mrb, sym, val);
}

mrb_value MRuby::getGlobal(const char * name)
{
  mrb_sym sym = mrb_intern_cstr(mrb, name);
  return mrb_gv_get(mrb, sym);
}

QString MRuby::toQString(mrb_value value)
{
  return mrb_string_value_cstr(mrb, &value);
}

mrb_value MRuby::fromQString(const QString & str)
{
  QByteArray bt = str.toLocal8Bit();
  return mrb_str_new_cstr(mrb, bt.constData());
}

mrb_value MRuby::symbolFromQString(const QString & str)
{
  QByteArray bt = str.toLocal8Bit();
  mrb_sym sym = mrb_intern_cstr(mrb, bt.constData());
  mrb_value v;
  SET_SYM_VALUE(v, sym);
  return v;
}

mrb_value MRuby::newHash()
{
  return mrb_hash_new(mrb);
}

bool MRuby::isHash(mrb_value hsh)
{
  return mrb_obj_is_kind_of(mrb, hsh, mrb->hash_class);
}


void MRuby::hashSet(mrb_value hash, mrb_value key, mrb_value value)
{
  mrb_hash_set(mrb, hash, key, value);
}

mrb_value MRuby::hashRef(mrb_value hash, mrb_value key)
{
  return mrb_hash_get(mrb, hash, key);
}

void MRuby::hashIterate(mrb_value hash,
                        const std::function <void (mrb_value key, mrb_value value)> & func)
{
  mrb_value keys = mrb_hash_keys(mrb, hash);
  arrayIterate(keys, [this, hash, func] (mrb_value key) {
      mrb_value value = mrb_hash_get(mrb, hash, key);
      func(key, value);
    });
}


mrb_value MRuby::newTime(int year, int month, int day,
                         int hour, int min, int sec, int usec)
{
  mrb_value cTime =  mrb_vm_const_get(mrb, mrb_intern_lit(mrb, "Time"));

  mrb_value rv = mrb_funcall(mrb, cTime, "new", 7,
                             mrb_fixnum_value(year),
                             mrb_fixnum_value(month),
                             mrb_fixnum_value(day),
                             mrb_fixnum_value(hour),
                             mrb_fixnum_value(min),
                             mrb_fixnum_value(sec),
                             mrb_fixnum_value(usec));
  return rv;
}


                     




#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
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

//////////////////////////////////////////////////////////////////////

static void mRubyArgs(const QString &, QString code,
                      const CommandOptions & )
{
  MRuby * r = MRuby::ruby();
  QStringList vars = r->detectParameters(code.toLocal8Bit());
  Terminal::out << " => " << vars.join(", ") << endl;
}

static Command 
ar("mruby-args", // command name
   effector(mRubyArgs), // action
   "file",  // group name
   &eA, // arguments
   NULL,
   "Ruby args",
   "Evaluates a Ruby expression and prints the result", "");

//////////////////////////////////////////////////////////////////////

void mrubyRunFile(const QString &, QString file)
{
  QFile f(file);
  Utils::open(&f, QIODevice::ReadOnly | QIODevice::Text);
  MRuby * r = MRuby::ruby();
  r->eval(&f);
}

static ArgumentList 
rA(QList<Argument *>() 
   << new FileArgument("file", 
                       "File",
                       "Ruby file to load"));


static Command 
lf("mruby-run", // command name
   optionLessEffector(mrubyRunFile), // action
   "file",  // group name
   &rA, // arguments
   NULL, // options
   "Ruby load",
   "Loads and runs a file containing ruby code", "");
