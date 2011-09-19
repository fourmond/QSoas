/**
   \file possessive-containers.hh
   Various utility containers that take ownership of pointers they hold.
   Copyright 2011 by Vincent Fourmond

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

#ifndef __POSSESSIVE_CONTAINERS_HH
#define __POSSESSIVE_CONTAINERS_HH

template <class T> class PossessiveList {
  void deleteValues() {
    for(int i = 0; i < values.size(); i++)
      delete values[i];
  };
public:
  QList<T*> values;

  PossessiveList() {;};
  PossessiveList(const QList<T*> & v) : values(v) {;};

  ~PossessiveList() {
    deleteValues();
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
};

template <class K, class V> class PossessiveHash {
public:
  typedef QHash<K, V*> Hash;
  Hash values;

  PossessiveHash(const QHash<K, V*> & v) : values(v) {;};

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
