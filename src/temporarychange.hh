/**
   \file temporarychange.hh
   A class to change the value of something during its scope only 
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
#ifndef __TEMPORARYCHANGE_HH
#define __TEMPORARYCHANGE_HH

/// Changes the value of the target during the scope of the existence
/// of the variable, which makes a temporary modification
/// exception-safe.
///
/// @todo This should be a child of DelayedAssign
template<typename T> class TemporaryChange{
protected:

  T & target;
  T initialValue;

public:
  
  TemporaryChange(T & t, const T & newval) : target(t) {
    initialValue = target;
    target = newval;
  };

  TemporaryChange(T & t) : target(t) {
    initialValue = target;
  };

  ~TemporaryChange() {
    target = initialValue;
  };

};


#endif
