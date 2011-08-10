/**
   \file ruby-templates.hh
   Templates to be used to catch exceptions within calls to Ruby
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

#ifndef __RUBY_TEMPLATES_HH
#define __RUBY_TEMPLATES_HH

#include <ruby.hh>

/// Argumentless callback
class RubyCallback0  {
  typedef VALUE (*Callback)();
  Callback callback;

  static VALUE wrapper(VALUE v) {
    RubyCallback0 * arg = (RubyCallback0 *) v;
    return arg->callback();
  };
public:

  RubyCallback0(Callback c) : callback(c) {;};

  /// Runs the code wrapping it into a rb_rescue code
  VALUE run() {
    return rb_rescue((VALUE (*)(...)) &wrapper, (VALUE) this,
                     (VALUE (*)(...)) &Ruby::globalRescueFunction, Qnil);
  };

};

inline VALUE Ruby::run(VALUE (*f)())
{
  RubyCallback0 cb(f);
  return cb.run();
}

/// Callback with one argument
template <typename A1> class RubyCallback1  {
  typedef VALUE (*Callback)(A1);
  Callback callback;
  A1 a1;

  static VALUE wrapper(VALUE v) {
    RubyCallback1 * arg = (RubyCallback1 *) v;
    return arg->callback(arg->a1);
  };
public:

  RubyCallback1(Callback c, A1 arg1) : callback(c), a1(arg1) {;};

  /// Runs the code wrapping it into a rb_rescue code
  VALUE run() {
    return rb_rescue((VALUE (*)(...)) &wrapper, (VALUE) this,
                     (VALUE (*)(...)) &Ruby::globalRescueFunction, Qnil);
  };

};

template<typename A1> VALUE Ruby::run(VALUE (*f)(A1), A1 a1)
{
  RubyCallback1<A1> cb(f, a1);
  return cb.run();
}

/// Callback with one argument
template <typename A1, typename A2> class RubyCallback2  {
  typedef VALUE (*Callback)(A1, A2);
  Callback callback;
  A1 a1;
  A2 a2;

  static VALUE wrapper(VALUE v) {
    RubyCallback2 * arg = (RubyCallback2 *) v;
    return arg->callback(arg->a1, arg->a2);
  };
public:

  RubyCallback2(Callback c, A1 arg1, A2 arg2) : 
    callback(c), a1(arg1), a2(arg2) {;};

  /// Runs the code wrapping it into a rb_rescue code
  VALUE run() {
    return rb_rescue((VALUE (*)(...)) &wrapper, (VALUE) this,
                     (VALUE (*)(...)) &Ruby::globalRescueFunction, Qnil);
  };

};

template<typename A1, typename A2> VALUE Ruby::run(VALUE (*f)(A1, A2), 
                                                   A1 a1, A2 a2)
{
  RubyCallback2<A1, A2> cb(f, a1, a2);
  return cb.run();
}


/// Callback with four arguments
template <typename A1, typename A2, typename A3, typename A4> 
class RubyCallback4  {
  typedef VALUE (*Callback)(A1, A2, A3, A4);
  Callback callback;
  A1 a1;
  A2 a2;
  A3 a3;
  A4 a4;

  static VALUE wrapper(VALUE v) {
    RubyCallback4 * arg = (RubyCallback4 *) v;
    return arg->callback(arg->a1, arg->a2, arg->a3, arg->a4);
  };
public:

  RubyCallback4(Callback c, A1 arg1, 
                A2 arg2, A3 arg3, A4 arg4) : 
    callback(c), a1(arg1), a2(arg2), a3(arg3), a4(arg4) {;};

  /// Runs the code wrapping it into a rb_rescue code
  VALUE run() {
    return rb_rescue((VALUE (*)(...)) &wrapper, (VALUE) this,
                     (VALUE (*)(...)) &Ruby::globalRescueFunction, Qnil);
  };

};

template <typename A1, typename A2, typename A3, typename A4> 
VALUE Ruby::run(VALUE (*f)(A1, A2, A3, A4), A1 a1, A2 a2, A3 a3, A4 a4)
{
  RubyCallback4<A1, A2, A3, A4> cb(f, a1, a2, a3, a4);
  return cb.run();
}

// Now, we'll have to do the same thing with member functions

#endif
