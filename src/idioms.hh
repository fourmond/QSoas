/**
   \file idoms.hh
   A few template classes for things that come in quite useful
   Copyright 2013 by Vincent Fourmond

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
#ifndef __IDIOMS_HH
#define __IDIOMS_HH

/// Assigns the current value of source to dest when the object goes
/// out of scope.
///
/// @todo This is really TemporaryChange with another name ?
template <class T> class DelayedAssign {
  T & dest;

  /// @todo This shouldn't be a const ref
  const T & source;
public:
  bool cancel;
  DelayedAssign(T &  dst, const T & src) : dest(dst), source(src), 
                                           cancel(false) {;};
  ~DelayedAssign() {
    if(! cancel)
      dest = source;
  };
};

template<typename T> class TemporaryChange : public DelayedAssign<T> {
protected:
  T initialValue;

public:
  
  TemporaryChange(T & t, const T & newval) : 
    DelayedAssign<T>(t, initialValue), initialValue(t)  {
    t = newval;
  };
  
  TemporaryChange(T & t) : 
  DelayedAssign<T>(t, initialValue), initialValue(t) {
  };
  
};


#endif
