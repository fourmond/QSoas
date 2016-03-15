/**
   \file ruby-wrappers.h
   Pure C wrapper around Ruby code. The point is to avoid to expose the
   ruby includes to C++ code, as they don't coexist very well with the
   Qt headers (at least on some arches).
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


#ifndef __RUBY_WRAPPERS_H
#define __RUBY_WRAPPERS_H

#if defined __cplusplus
extern "C" {
#endif

  typedef long __rbw_internal_int;

  /* This is what Ruby calls VALUE */
  typedef __rbw_internal_int RUBY_VALUE;

  /* This is what Ruby calls ID */
  typedef __rbw_internal_int RUBY_ID;

  /* Essentially, all the wrappers keep their name, with a rb -> rbw change */


  RUBY_ID rbw_intern(const char*);

  RUBY_VALUE rbw_funcall2(RUBY_VALUE, RUBY_ID, int, const RUBY_VALUE*);
  
  RUBY_VALUE rbw_gv_set(const char*, RUBY_VALUE);
  RUBY_VALUE rbw_gv_get(const char*);

  /** This forces allocation on the heap ? */
  RUBY_VALUE rbw_float_new(double);

  /** 
      @name Class and module-related functions
      
      @{
  */

  typedef RUBY_VALUE (*rbw_function)();
  
  RUBY_VALUE rbw_define_module(const char*);
  void rbw_define_method(RUBY_VALUE obj,const char* name,
                         rbw_function func, int nbargs);
  void rbw_define_singleton_method(RUBY_VALUE obj, const char* name,
                                   rbw_function func, int nbargs);


  RUBY_VALUE rbw_define_class(const char * name, RUBY_VALUE parent);

  /* RUBY_VALUE rb_define_class_under(RUBY_VALUE mod, const char* name, */
  /*                                  RUBY_VALUE parent); */
  
  /* RUBY_VALUE rb_define_module_under(RUBY_VALUE mod, const char* name); */

  RUBY_VALUE rbw_data_object_alloc(RUBY_VALUE klass, void * data,
                                   void (*mark)(void *),
                                   void (*free)(void *));

  void * rbw_data_get_struct(RUBY_VALUE obj);

  /** 
      @}
  */

  void rbw_define_global_const(const char*, RUBY_VALUE);

  /** 
      @name Array-related functions

      @{
  */
  RUBY_VALUE rbw_ary_new(void);
  RUBY_VALUE rbw_ary_push(RUBY_VALUE, RUBY_VALUE);
  RUBY_VALUE rbw_ary_entry(RUBY_VALUE, long);

  long rbw_array_length(RUBY_VALUE);

  /** Strict type checking: class is the class (not children) ! */
  int rbw_is_array(RUBY_VALUE);

  /** 
      @}
  */

  RUBY_VALUE rbw_protect(RUBY_VALUE (*)(RUBY_VALUE), RUBY_VALUE, int*);

  void rbw_p(RUBY_VALUE obj);

  RUBY_VALUE rbw_obj_as_string(RUBY_VALUE);
  /*RUBY_VALUE rbw_obj_class(RUBY_VALUE);*/
  /*VALUE rb_obj_is_instance_of(VALUE, VALUE);*/

  RUBY_VALUE rbw_eval_string(const char*);

  void rbw_extend_object(RUBY_VALUE, RUBY_VALUE);

  RUBY_VALUE rbw_str_new2(const char*);


  RUBY_VALUE rbw_time_new(time_t, long);

  RUBY_VALUE rbw_inspect(RUBY_VALUE);

  RUBY_VALUE rbw_class_of(RUBY_VALUE obj);

  void rbw_raise(RUBY_VALUE klass, const char * message);

  double rbw_num2dbl(RUBY_VALUE);

  long rbw_num2int(RUBY_VALUE);


  const char * rbw_string_value_cstr(RUBY_VALUE obj);

  void ruby_wrappers_init();

  /** 
      @name Class/modules constants

      @{
  */


  
  /** The module Math */
  RUBY_VALUE rbw_mMath();

  RUBY_VALUE rbw_cObject();

  RUBY_VALUE rbw_eArgError();

  RUBY_VALUE rbw_eRuntimeError();

  RUBY_VALUE rbw_eException();
  /** 
      @}
  */

  /** 
      @name Hash-related functions
      
      @{
  */

  /** Strict type checking: class is the class (not children) ! */
  int rbw_is_hash(RUBY_VALUE);

  RUBY_VALUE rbw_hash_new(void);
  RUBY_VALUE rbw_hash_aset(RUBY_VALUE, RUBY_VALUE, RUBY_VALUE);
  RUBY_VALUE rbw_hash_aref(RUBY_VALUE hsh, RUBY_VALUE key);
  RUBY_VALUE rbw_hash_delete(RUBY_VALUE, RUBY_VALUE);
  void rbw_hash_foreach(RUBY_VALUE, int (*)(), RUBY_VALUE);


  /** 
      @}
  */


  /** 
      Returns true if the object is a numeric type, i.e. something it
      makes sense to convert to a double 
  */
  int rbw_is_numeric(RUBY_VALUE obj);
  
  /** Test if the given Ruby object is true. The equivalent of RTEST */
  int rbw_test(RUBY_VALUE);


  /** Sets the target to the given float value */
  void rbw_set_float(RUBY_VALUE *hsh, double value);

  /** Returns the int value */
  RUBY_VALUE rbw_int(int);


  RUBY_VALUE rbw_long2num(long);

   /* egrep -ho rb_'[a-zA-Z0-9_]+' * | sort | uniq */

  /* rb_cArray */
  /* rb_cHash */
  /* rb_class_of */
  /* rb_inspect */
  /* rb_mMath */

  /* Constants */
  extern const RUBY_VALUE rbw_nil;

  #define RBW_ST_CONTINUE 0

#if defined __cplusplus
}
#endif


#endif
