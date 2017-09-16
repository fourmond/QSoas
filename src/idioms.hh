/**
   \file idoms.hh
   A few template (and not templates) classes for things that come in quite useful to do things in the C++ way
   Copyright 2013, 2014, 2016 by CNRS/AMU

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

#include <terminal.hh>
#include <mruby.hh>

/// Assigns the current value of source to dest when the object goes
/// out of scope.
template <class T> class DelayedAssign {
  T & dest;

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

/// Temporary changes the value of the given variable.
///
/// This \b cannot be a child of DelayedAssign, because it involves
/// referencing to an object living in the child that has therefore
/// already been destroyed before the base class uses its value...
template<typename T> class TemporaryChange {
protected:
  T & target;
  
  T initialValue;

public:
  
  TemporaryChange(T & t, const T & newval) : 
    target(t), initialValue(t) {
    t = newval;
  };
  
  TemporaryChange(T & t) : 
    target(t), initialValue(t) {
  };

  ~TemporaryChange() {
    target = initialValue;
  };
};


/// Same thing as TemporaryChange, but for an instance of QThreadStorage
template<typename T> class TemporaryThreadLocalChange {
protected:
  QThreadStorage<T> & target;
  
  T initialValue;

public:
  
  TemporaryThreadLocalChange(QThreadStorage<T> & t, const T & newval) : 
    target(t) {
    initialValue = target.localData();
    target.localData() = newval;
  };
  
  // TemporaryThreadLocalChange(QThreadStorage<T> & t) : 
  //   target(t) {
  //   initialValue = target.localData();
  // };

  ~TemporaryThreadLocalChange() {
    target.localData() = initialValue;
  };
};



/// Save a global Ruby variable
class SaveGlobal {
  mrb_value old;
  MRuby * mr;

  const char * name;
public:

  /// Has to be a real const char ? For now.
  SaveGlobal(const char * n) : name(n) {
    mr = MRuby::ruby();
    old = mr->getGlobal(name);
  };

  SaveGlobal(const char * n, mrb_value nv) : name(n) {
    mr = MRuby::ruby();
    old = mr->getGlobal(name);
    mr->setGlobal(name, nv);
  };

  ~SaveGlobal() {
    mr->setGlobal(name, old);
  };
};

/// Temporarily changes the current directory for the scope of this
/// variable.
class TemporarilyChangeDirectory {
  QString prev;
public:
  TemporarilyChangeDirectory(const QString & str = "") {
    if(! str.isEmpty()) {
      prev = QDir::currentPath();
      if(! QDir::setCurrent(str))
        throw RuntimeError("Could not cd to '%1'").arg(str);
    }
  };

  ~TemporarilyChangeDirectory() {
    if(! prev.isEmpty()) {
      if(! QDir::setCurrent(prev))
        Terminal::out << QString("Could not cd back to '%1'").arg(prev);
        // throw RuntimeError("Could not cd back to '%1'").arg(prev);
    }
  };
};

class TemporarilyDisableWidget {
  QWidget * target;

  bool initialState;
public:
  TemporarilyDisableWidget(QWidget * tg, bool enable = false) :
    target(tg) {
    initialState = target->isEnabled();
    target->setEnabled(enable);
  };

  ~TemporarilyDisableWidget() {
    target->setEnabled(initialState);
  };
};
                              


#endif
