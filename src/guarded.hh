/**
   \file guarded.hh
   Simple implementation of QObject-less guarded pointers
   Copyright 2012 by Vincent Fourmond

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
#ifndef __GUARDED_HH
#define __GUARDED_HH

class GuardedPointerBase;

/// Base class of all stuff that can be guarded by a guarded pointer
class Guardable {
  /// The list of watchers
  QSet<GuardedPointerBase*> watchers;

  friend class GuardedPointerBase;

  void addWatcher(GuardedPointerBase * watcher) {
    watchers.insert(watcher);
  };

  void removeWatcher(GuardedPointerBase * watcher) {
    watchers.remove(watcher);
  };

public:
  Guardable() {;};
  
  Guardable(const Guardable & /*tg*/) {;}; // we don't copy the watchers list !

protected:
  virtual ~Guardable();

};

/// Base of all the guarded pointers, the latter providing the actual
/// storage.
class GuardedPointerBase {
  friend class Guardable;

protected:
  /// The target object
  Guardable * _target;

  /// Called when the pointer goes out of scope
  void terminate() {
    _target = NULL;              // We're done !
  };
  
  GuardedPointerBase(Guardable * t) : 
  _target(t) {
    if(_target)
      _target->addWatcher(this);
  };

  virtual ~GuardedPointerBase() {
    if(_target)
      _target->removeWatcher(this);
  };
};

inline Guardable::~Guardable()
{
  QSet<GuardedPointerBase*>::iterator i = watchers.begin();
  while(i != watchers.end()) {
    (*i)->terminate();
    ++i;
  }
}

template <class T> class GuardedPointer : public GuardedPointerBase {
public:
  GuardedPointer(T * ptr) : GuardedPointerBase(ptr) {;};

  T * target() const { return static_cast<T*>(_target); };
  operator T*() const { return target(); };

  T * operator ->() const { return target();}
};

template <class T> class ConstGuardedPointer : public GuardedPointerBase {
public:
  ConstGuardedPointer(const T * ptr) : 
  GuardedPointerBase(const_cast<T*>(ptr)) {;};

  const T * target() const { return static_cast<const T*>(_target); };
  operator const T*() const { return target(); };

  const T * operator ->() const { return target();}
};


#endif
