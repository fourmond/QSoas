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

  /* This is what Ruby calls VALUE */
  typedef void * RUBY_VALUE;

  /* This is what Ruby calls ID */
  typedef void * RUBY_ID;

  /* Essentially, all the wrappers keep their name, with a rb -> rbw change */


  RUBY_ID rbw_intern(const char*);

  RUBY_VALUE rbw_funcall(RUBY_VALUE, RUBY_ID, int, ...);
  RUBY_VALUE rbw_funcall2(RUBY_VALUE, RUBY_ID, int, const RUBY_VALUE*);
  
  RUBY_VALUE rbw_gv_set(const char*, RUBY_VALUE);
  RUBY_VALUE rbw_gv_get(const char*);

  RUBY_VALUE rbw_hash_new(void);
  RUBY_VALUE rbw_hash_aset(RUBY_VALUE, RUBY_VALUE, RUBY_VALUE);
  RUBY_VALUE rbw_hash_delete(RUBY_VALUE, RUBY_VALUE);
  RUBY_VALUE rbw_hash_foreach(RUBY_VALUE, int (*)(), RUBY_VALUE);

  RUBY_VALUE rbw_float_new(double);

  RUBY_VALUE rbw_define_module(const char*);
  void rbw_define_global_const(const char*, RUBY_VALUE);
  void rbw_define_method(RUBY_VALUE,const char*,
                        RUBY_VALUE(*)(), int);
  void rbw_define_singleton_method(RUBY_VALUE, const char*,
                                  RUBY_VALUE(*)(), int);

  RUBY_VALUE rbw_ary_new(void);
  RUBY_VALUE rbw_ary_push(RUBY_VALUE, RUBY_VALUE);
  RUBY_VALUE rbw_ary_entry(RUBY_VALUE, long);

  RUBY_VALUE rbw_protect(RUBY_VALUE (*)(RUBY_VALUE), RUBY_VALUE, int*);

  RUBY_VALUE rbw_obj_as_string(RUBY_VALUE);
  RUBY_VALUE rbw_obj_class(RUBY_VALUE);
  /*VALUE rb_obj_is_instance_of(VALUE, VALUE);*/

  RUBY_VALUE rbw_eval_string(const char*);

  void rbw_extend_object(RUBY_VALUE, RUBY_VALUE);

  RUBY_VALUE rbw_str_new2(const char*);


  RUBY_VALUE rbw_time_new(time_t, long);

  RUBY_VALUE rbw_inspect(RUBY_VALUE);
  
   /* egrep -ho rb_'[a-zA-Z0-9_]+' * | sort | uniq */

  /* rb_cArray */
  /* rb_cHash */
  /* rb_class_of */
  /* rb_inspect */
  /* rb_mMath */


#if defined __cplusplus
}
#endif


#endif
