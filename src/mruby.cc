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
#include <mruby/class.h>

#include <exceptions.hh>
#include <utils.hh>

#include <terminal.hh>

#include <gslfunction.hh>

MRuby::MRuby()
{
  mrb = mrb_open();
  cQSoasInterface = NULL;
  cFancyHash = mrb_nil_value();
  cTime = mrb_vm_const_get(mrb, mrb_intern_lit(mrb, "Time"));
  sNew = mrb_intern_lit(mrb, "new");
  sToS = mrb_intern_lit(mrb, "to_s");
  sBrackets = mrb_intern_lit(mrb, "[]");
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

    // Here load the fancy hash structure
    QFile f(":/ruby/fancyhash.rb");
    f.open(QIODevice::ReadOnly);
    globalInterpreter->eval(&f);

    globalInterpreter->cFancyHash =
      mrb_vm_const_get(globalInterpreter->mrb,
                       mrb_intern_lit(globalInterpreter->mrb, "FancyHash"));

    globalInterpreter->initializeInterface();
    globalInterpreter->initializeRegexp();
    globalInterpreter->initializeComplex();
  }
  return globalInterpreter;
}



QString MRuby::inspect(mrb_value object)
{
  MRubyArenaContext c(this);

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
    mrb->exc = NULL;
    throw RuntimeError("A ruby exception occurred: %1").arg(inspect(exc));
  }

  return helper;
}

void MRuby::throwIfException(mrb_value obj)
{
  // printf("except: %p\n", mrb->eException_class);
#if MRUBY_RELEASE_MAJOR == 1
  if(mrb_obj_is_kind_of(mrb, obj, mrb->eException_class)) {
    throw RuntimeError("A ruby exception occurred: %1").arg(inspect(obj));
  }
#elif MRUBY_RELEASE_MAJOR == 2
  if(mrb_class(mrb, obj)->tt == MRB_TT_EXCEPTION) {
    throw RuntimeError("A ruby exception occurred: %1").arg(inspect(obj));
  }
#endif
}


struct RProc * MRuby::generateCode(const QByteArray & code,
                                   const QString & fileName, int line)
{
  struct mrbc_context * c = mrbc_context_new(mrb);
  c->capture_errors = true;
  QByteArray fn = fileName.toLocal8Bit();
  mrbc_filename(mrb, c, fn.constData());
  if(line >= 0)
    c->lineno = line;
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

  // Unsure terminal is the best place, but, well
  if(p->nwarn > 0) {
    // We have a syntax error
    QString message;
    // Point to exact code place ?
    for(size_t i = 0; i < p->nwarn; i++) {
      if(i > 0)
        message += "\n";
      message += QString("%1 at line %2:%3").
        arg(p->warn_buffer[i].message).
        arg(p->warn_buffer[i].lineno).
        arg(p->warn_buffer[i].column);
    }
    Terminal::out << "Ruby warnings: " << message << endl;
  }

  RProc * proc = mrb_generate_code(mrb, p);
  mrbc_context_free(mrb, c);
  mrb_parser_free(p);
  return proc;
}

mrb_value MRuby::eval(const char * code,
                      const QString & fileName, int line)
{
  return eval(QByteArray(code), fileName, line);
}

mrb_value MRuby::eval(const QByteArray & code,
                      const QString & fileName, int line)
{
  MRubyArenaContext c(this);
  RProc * proc = generateCode(code, fileName, line);
  return protect([this, proc]() -> mrb_value {
                   return mrb_top_run(mrb, proc, mrb_top_self(mrb), 0);
    }
    );
}

mrb_value MRuby::eval(const QString & code,
                      const QString & fileName, int line)
{
  return eval(code.toLocal8Bit(), fileName, line);
}

mrb_value MRuby::eval(QIODevice * device)
{
  MRubyArenaContext c(this);
  QByteArray code = device->readAll();
  RProc * proc = generateCode(code, Utils::fileName(device));
  return protect([this, proc]() -> mrb_value {
                   return mrb_top_run(mrb, proc, mrb_top_self(mrb), 0);
    }
    );
}

#if MRUBY_RELEASE_MAJOR == 1

// Hmmm.

QStringList MRuby::detectParameters(const QByteArray & code)
{
  MRubyArenaContext c(this);

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

#elif MRUBY_RELEASE_MAJOR == 2


#define CASE(insn,ops) case insn: FETCH_ ## ops (); L_ ## insn

QStringList MRuby::detectParameters(const QByteArray & code)
{
  MRubyArenaContext c(this);

  // QTextStream o(stdout);
  // o << "Detecting parameters for : '" << code << "'" << endl;
  
  RProc * proc = generateCode(code);
  struct mrb_irep * irep = proc->body.irep;

  // The idea is just to scan the code and detect funcalls of single
  // arguments from what seems to be top level self.
  int cur_top_self = -1;
  QSet<QString> rv;

  const mrb_code *pc, *pcend;
  mrb_code ins;
  int ai;

  pc = irep->iseq;
  pcend = pc + irep->ilen;
  while (pc < pcend) {
    ptrdiff_t i;
    uint32_t a;
    uint16_t b;
    uint8_t c;

    ai = mrb_gc_arena_save(mrb);

    i = pc - irep->iseq;

    ins = READ_B();
    // We unfortunately need the full case from mruby/src/codedump.c,
    // since the opcodes now have variable lengths
    switch (ins) {
      CASE(OP_NOP, Z): break;
      CASE(OP_MOVE, BB): break;
      CASE(OP_LOADL, BB): break;
      CASE(OP_LOADI, BB): break;
      CASE(OP_LOADINEG, BB): break;
      CASE(OP_LOADI__1, B): break;
      CASE(OP_LOADI_0, B): break;
      CASE(OP_LOADI_1, B): break;
      CASE(OP_LOADI_2, B): break;
      CASE(OP_LOADI_3, B): break;
      CASE(OP_LOADI_4, B): break;
      CASE(OP_LOADI_5, B): break;
      CASE(OP_LOADI_6, B): break;
      CASE(OP_LOADI_7, B): break;
      CASE(OP_LOADSYM, BB): break;
      CASE(OP_LOADNIL, B): break;
      CASE(OP_LOADSELF, B):
        cur_top_self = a;
      break;
      CASE(OP_LOADT, B): break;
      CASE(OP_LOADF, B): break;
      CASE(OP_GETGV, BB): break;
      CASE(OP_SETGV, BB): break;
      CASE(OP_GETSV, BB): break;
      CASE(OP_SETSV, BB): break;
      CASE(OP_GETCONST, BB): break;
      CASE(OP_SETCONST, BB): break;
      CASE(OP_GETMCNST, BB): break;
      CASE(OP_SETMCNST, BB): break;
      CASE(OP_GETIV, BB): break;
      CASE(OP_SETIV, BB): break;
      CASE(OP_GETUPVAR, BBB): break;
      CASE(OP_SETUPVAR, BBB): break;
      CASE(OP_GETCV, BB): break;
      CASE(OP_SETCV, BB): break;
      CASE(OP_JMP, S): break;
      CASE(OP_JMPIF, BS): break;
      CASE(OP_JMPNOT, BS): break;
      CASE(OP_JMPNIL, BS): break;
      CASE(OP_SENDV, BB):
        if(cur_top_self == a && c == 0)
          rv.insert(mrb_sym2name(mrb, irep->syms[b]));
      break;
      CASE(OP_SENDVB, BB):
        if(cur_top_self == a && c == 0)
          rv.insert(mrb_sym2name(mrb, irep->syms[b]));
      break;
      CASE(OP_SEND, BBB):
        if(cur_top_self == a && c == 0)
          rv.insert(mrb_sym2name(mrb, irep->syms[b]));
      break;
      CASE(OP_SENDB, BBB):
        if(cur_top_self == a && c == 0)
          rv.insert(mrb_sym2name(mrb, irep->syms[b]));
      break;
      CASE(OP_CALL, Z): break;
      CASE(OP_SUPER, BB): break;
      CASE(OP_ARGARY, BS): break;
      CASE(OP_ENTER, W): break;
      CASE(OP_KEY_P, BB): break;
      CASE(OP_KEYEND, Z): break;
      CASE(OP_KARG, BB): break;
      CASE(OP_RETURN, B): break;
      CASE(OP_RETURN_BLK, B): break;
      CASE(OP_BREAK, B): break;
      CASE(OP_BLKPUSH, BS): break;
      CASE(OP_LAMBDA, BB): break;
      CASE(OP_BLOCK, BB): break;
      CASE(OP_METHOD, BB): break;
      CASE(OP_RANGE_INC, B): break;
      CASE(OP_RANGE_EXC, B): break;
      CASE(OP_DEF, BB): break;
      CASE(OP_UNDEF, B): break;
      CASE(OP_ALIAS, BB): break;
      CASE(OP_ADD, B): break;
      CASE(OP_ADDI, BB): break;
      CASE(OP_SUB, B): break;
      CASE(OP_SUBI, BB): break;
      CASE(OP_MUL, B): break;
      CASE(OP_DIV, B): break;
      CASE(OP_LT, B): break;
      CASE(OP_LE, B): break;
      CASE(OP_GT, B): break;
      CASE(OP_GE, B): break;
      CASE(OP_EQ, B): break;
      CASE(OP_ARRAY, BB): break;
      CASE(OP_ARRAY2, BBB): break;
      CASE(OP_ARYCAT, B): break;
      CASE(OP_ARYPUSH, B): break;
      CASE(OP_ARYDUP, B): break;
      CASE(OP_AREF, BBB): break;
      CASE(OP_ASET, BBB): break;
      CASE(OP_APOST, BBB): break;
      CASE(OP_INTERN, B): break;
      CASE(OP_STRING, BB): break;
      CASE(OP_STRCAT, B): break;
      CASE(OP_HASH, BB): break;
      CASE(OP_HASHADD, BB): break;
      CASE(OP_HASHCAT, B): break;
      CASE(OP_OCLASS, B): break;
      CASE(OP_CLASS, BB): break;
      CASE(OP_MODULE, BB): break;
      CASE(OP_EXEC, BB): break;
      CASE(OP_SCLASS, B): break;
      CASE(OP_TCLASS, B): break;
      CASE(OP_ERR, B): break;
      CASE(OP_EPUSH, B): break;
      CASE(OP_ONERR, S): break;
      CASE(OP_EXCEPT, B): break;
      CASE(OP_RESCUE, BB): break;
      CASE(OP_RAISE, B): break;
      CASE(OP_POPERR, B): break;
      CASE(OP_EPOP, B): break;
      CASE(OP_DEBUG, BBB): break;
      CASE(OP_STOP, Z): break;
      CASE(OP_EXT1, Z): break;
      CASE(OP_EXT2, Z): break;
      CASE(OP_EXT3, Z): break;
      mrb_gc_arena_restore(mrb, ai);
    }
  }
  // QStringList l = rv.toList();
  // o << " -> " << l.join(", ") << endl;
  return rv.toList();
}

#endif

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
  mrb_value v = protect([this, self, func, nb, params]() -> mrb_value {
      return funcall_up(self, func, nb, params);
    }
    );
  // Funcall does not throw an exception, but returns it, so we have
  // to check whether the return value is an exception.
  // 
  // This probably means we don't have to use protect.
  throwIfException(v);
  return v;
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
  if(mrb_nil_p(mrb_check_string_type(mrb, value))) {
    // convert to string using to_s
    value = funcall(value, sToS, 0, NULL);
  }
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

mrb_value MRuby::newFancyHash()
{
  return mrb_funcall(mrb, cFancyHash, "new", 0);
}

void MRuby::fancyHashSet(mrb_value hash, mrb_value key, mrb_value value)
{
  mrb_funcall(mrb, hash, "[]=", 2, key, value);
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


QString MRuby::memoryUse()
{
  QString s;
  mrb_value v = eval("ObjectSpace.count_objects");
  QTextStream o(&s);
  o <<  mrb->gc.live << " live, "
    << mrb->gc.arena_idx << " arena -- "
    << inspect(v);
  return s;
}


                     




#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <terminal.hh>


static void mRubyArgs(const QString &, QString code,
                      const CommandOptions & )
{
  MRuby * r = MRuby::ruby();
  QStringList vars = r->detectParameters(code.toLocal8Bit());
  Terminal::out << " => " << vars.join(", ") << endl;
}

static ArgumentList 
eA(QList<Argument *>() 
   << new StringArgument("code", 
                         "Code",
                         "Ruby code"));

static Command 
ar("mruby-detect-args", // command name
   effector(mRubyArgs), // action
   "file",  // group name
   &eA, // arguments
   NULL,
   "Ruby args",
   "Detects the arguments within a Ruby expression", "");

