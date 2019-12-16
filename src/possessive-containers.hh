/**
   \file possessive-containers.hh
   Various utility containers that take ownership of pointers they hold.
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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
#ifndef __POSSESSIVE_CONTAINERS_HH
#define __POSSESSIVE_CONTAINERS_HH

/// This class is a thin wrapper around a QList of pointers taking the
/// ownership of the pointers (ie destroying them upon destruction).
///
/// If detach() is called, the class no longer assumes ownership of
/// the pointers and will not destroy them.
template <class T> class PossessiveList {

  bool shouldDelete;

  void deleteValues() {
    if(shouldDelete)
      for(int i = 0; i < values.size(); i++)
        delete values[i];
  };

public:
  QList<T*> values;

  PossessiveList() : shouldDelete(true) {;};
  explicit PossessiveList(const QList<T*> & v) : shouldDelete(true),
                                                 values(v) {;};
  explicit PossessiveList(int nb) : shouldDelete(true) {
    values.reserve(nb);
    for(int i = 0; i < nb; i++)
      values.append(NULL);
  };

  ~PossessiveList() {
    deleteValues();
  };

  void detach() {
    shouldDelete = false;
  };

  void clear() {
    deleteValues();
    values.clear();
  };

  operator QList<T*>() const {
    return values;
  };

  PossessiveList & operator <<(T * a) {
    values << a;
    return *this;
  };

  typename QList<T*>::iterator begin() {
    return values.begin();
  };

  typename QList<T*>::const_iterator begin() const {
    return values.begin();
  };

  typename QList<T*>::iterator end() {
    return values.end();
  };

  typename QList<T*>::const_iterator end() const {
    return values.end();
  };

  int size() const {
    return values.size();
  };

  /// @todo This isn't safe if the pointer is modified...
  T *& operator[](int idx) {
    return values[idx];
  };

  const T * operator[](int idx) const {
    return values[idx];
  };
};

template <class K, class V> class PossessiveHash {
public:
  typedef QHash<K, V*> Hash;
  Hash values;

  explicit PossessiveHash(const QHash<K, V*> & v) : values(v) {;};

  ~PossessiveHash() {
    for(typename Hash::iterator i = values.begin();
        i != values.end(); i++)
      delete i.value();
  };

  operator QHash<K, V*>() const {
    return values;
  };
};

#endif
