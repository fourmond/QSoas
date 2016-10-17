/*
  ruby-wrappers.c: implementation of the
  Copyright 2015 by CNRS/AMU

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
#include <ruby-wrappers.h>

#define CNV1(fn) (RUBY_VALUE) fn((VALUE) a);
#define CNV2(fn) (RUBY_VALUE) fn((VALUE) a, (VALUE) b);
#define CNV3(fn) (RUBY_VALUE) fn((VALUE) a, (VALUE) b, (VALUE) c);

const RUBY_VALUE rbw_nil = (RUBY_VALUE)Qnil;

RUBY_VALUE rbw_gv_set(const char* cnst, RUBY_VALUE val)
{
  return (RUBY_VALUE) rb_gv_set(cnst, (VALUE) val);
}

RUBY_VALUE rbw_gv_get(const char* cnst)
{
  return (RUBY_VALUE) rb_gv_get(cnst);
}

RUBY_VALUE rbw_hash_new(void)
{
  return (RUBY_VALUE) rb_hash_new();
}


RUBY_VALUE rbw_hash_aset(RUBY_VALUE a, RUBY_VALUE b, RUBY_VALUE c)
{
  return CNV3(rb_hash_aset);
}

RUBY_VALUE rbw_hash_aref(RUBY_VALUE a, RUBY_VALUE b)
{
  return CNV2(rb_hash_aref);
}



RUBY_VALUE rbw_hash_delete(RUBY_VALUE a, RUBY_VALUE b)
{
  return CNV2(rb_hash_delete);
}
     
void rbw_hash_foreach(RUBY_VALUE a, int (*fn)(), RUBY_VALUE b)
{
  return rb_hash_foreach((VALUE) a, (int (*)(ANYARGS)) fn, (VALUE) b);
}

RUBY_VALUE rbw_float_new(double val)
{
#ifdef USE_FLONUM
  return (RUBY_VALUE) rb_float_new_in_heap(val);
#else
  return (RUBY_VALUE) rb_float_new(val);
#endif
}

RUBY_VALUE rbw_define_module(const char* mod)
{
  return (RUBY_VALUE) rb_define_module(mod);
}

void rbw_define_global_const(const char* nm, RUBY_VALUE a)
{
  rb_define_global_const(nm, (VALUE) a);
}

void rbw_define_method(RUBY_VALUE slf, const char* name,
                        RUBY_VALUE(*fn)(), int nb)
{
  rb_define_method((VALUE) slf, name, fn, nb);
}

void rbw_define_singleton_method(RUBY_VALUE slf, const char* name,
                                 RUBY_VALUE(*fn)(), int nb)
{
  rb_define_singleton_method((VALUE) slf, name, fn, nb);
}

RUBY_VALUE rbw_define_class(const char * name, RUBY_VALUE parent)
{
  return (RUBY_VALUE) rb_define_class(name, (VALUE) parent);
}

RUBY_VALUE rbw_data_object_alloc(RUBY_VALUE klass, void * data,
                                 void (*mark_fn)(void *),
                                 void (*free_fn)(void *))
{
  return (RUBY_VALUE) rb_data_object_alloc((VALUE) klass, data,
                                           mark_fn, free_fn);
}

void * rbw_data_get_struct(RUBY_VALUE obj)
{
  void * d;
  Data_Get_Struct(obj, void, d);
  return d;
}

void rbw_raise(RUBY_VALUE klass, const char * msg)
{
  rb_raise(klass, "%s", msg);
}

RUBY_VALUE rbw_ary_new(void)
{
  return (RUBY_VALUE) rb_ary_new();
}

RUBY_VALUE rbw_ary_push(RUBY_VALUE a, RUBY_VALUE b)
{
  return CNV2(rb_ary_push);
}

RUBY_VALUE rbw_ary_entry(RUBY_VALUE a, long i)
{
  return (RUBY_VALUE) rb_ary_entry((VALUE)a, i);
}


RUBY_VALUE rbw_protect(RUBY_VALUE (*fn)(RUBY_VALUE), RUBY_VALUE a, int*b)
{
  return (RUBY_VALUE) rb_protect(fn, (VALUE) a, b);
}

RUBY_VALUE rbw_obj_as_string(RUBY_VALUE a)
{
  return CNV1(rb_obj_as_string);
}

RUBY_VALUE rbw_eval_string(const char* str)
{
  return (RUBY_VALUE) rb_eval_string(str);
}

void rbw_extend_object(RUBY_VALUE a, RUBY_VALUE b)
{
  rb_extend_object((VALUE) a, (VALUE) b);
}

RUBY_VALUE rbw_str_new2(const char* s)
{
  return (RUBY_VALUE) rb_str_new2(s);
}

RUBY_VALUE rbw_time_new(time_t a, long b)
{
  return (RUBY_VALUE) rb_time_new(a, b);
}

RUBY_VALUE rbw_inspect(RUBY_VALUE a)
{
  return CNV1(rb_inspect);
}

RUBY_VALUE rbw_class_of(RUBY_VALUE a)
{
  return CNV1(rb_class_of);
}

double rbw_num2dbl(RUBY_VALUE a)
{
  return rb_num2dbl((VALUE) a);
}

long rbw_num2int(RUBY_VALUE a)
{
  return NUM2INT((VALUE) a);
}

int rbw_test(RUBY_VALUE a)
{
  return RTEST((VALUE) a);
}


RUBY_ID rbw_intern(const char* s)
{
  return (RUBY_ID)(rb_intern(s));
}

RUBY_VALUE rbw_funcall2(RUBY_VALUE slf, RUBY_ID id, int n,
                        const RUBY_VALUE* args)
{
  return (RUBY_VALUE) rb_funcall2((VALUE)slf, (ID) id, n,
                                  (const VALUE *)args);
}

const char * rbw_string_value_cstr(RUBY_VALUE obj)
{
  return rb_string_value_cstr((VALUE*) (&obj));
}

long rbw_array_length(RUBY_VALUE ary)
{
  return RARRAY_LEN((VALUE) ary);
}

void ruby_wrappers_init()
{
  RUBY_INIT_STACK;
  ruby_init();
}

void ruby_wrappers_process_options(int argn, char **argv)
{
  ruby_process_options(argn, argv);
}


RUBY_VALUE rbw_int(int i)
{
  return (RUBY_VALUE)INT2FIX(i);
}

RUBY_VALUE rbw_long2num(long i)
{
  return (RUBY_VALUE) LONG2NUM(i);
}

RUBY_VALUE rbw_mMath()
{
  return (RUBY_VALUE) rb_mMath;
}

RUBY_VALUE rbw_cObject()
{
  return (RUBY_VALUE) rb_cObject;
}

RUBY_VALUE rbw_eArgError()
{
  return (RUBY_VALUE) rb_eArgError;
}

RUBY_VALUE rbw_eRuntimeError()
{
  return (RUBY_VALUE) rb_eRuntimeError;
}

void rbw_p(RUBY_VALUE a)
{
  rb_p((VALUE)a);
}

RUBY_VALUE rbw_eException()
{
  return (RUBY_VALUE) rb_eException;
}


int rbw_is_hash(RUBY_VALUE ret)
{
  return rb_obj_class((VALUE)ret) == rb_cHash;
}

int rbw_is_array(RUBY_VALUE ret)
{
  return rb_obj_class((VALUE)ret) == rb_cArray;
}

/* For now, the LVALUE stuff: */
#ifdef RFLOAT
#define RFLOAT_LVALUE(x) (RFLOAT((x))->float_value)
void rbw_set_float(RUBY_VALUE *x, double value)
{
  RFLOAT_LVALUE(*x) = value;
}
#else

/* This is what is commonly called a DIRTY HACK */
struct RFloat {
    struct RBasic basic;
    double float_value;
};

void rbw_set_float(RUBY_VALUE *x, double value)
{
  ((struct RFloat*)*x)->float_value = value;
}
#endif

int rbw_is_numeric(RUBY_VALUE value)
{
  return RTEST(rb_obj_is_kind_of(value, rb_cFixnum)) ||
    RTEST(rb_obj_is_kind_of(value, rb_cBignum)) ||
    RTEST(rb_obj_is_kind_of(value, rb_cFloat));
}
