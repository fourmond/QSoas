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


#endif
