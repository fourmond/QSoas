/**
   \file namedinstance.hh
   Template class for a "find instance by name" design
   Copyright 2014 by Vincent Fourmond

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
#ifndef __NAMEDINSTANCE_HH
#define __NAMEDINSTANCE_HH

#include <exceptions.hh>

/// This template class implements the "named instance" paradigm used
/// so often in this program.
///
/// Classes making use of this should derive from this one.
template <typename C> class NamedInstance {
protected:

  /// The hash of instances
  typedef QHash<QString,  C * > InstanceHash;

  

  /// The global register for the instances
  ///
  /// Must be a pointer to ensure we don't initialize the hash after
  /// the first registration !
  static InstanceHash * instances;

  /// Iterator pointing to the first instance
  static typename InstanceHash::iterator begin() {
    if(! instances)
      instances = new InstanceHash();
    return instances->begin();
  };
  
  /// Iterator pointing to the "one-after-last" instance. @warning 
  /// This is invalid if there is no instance registered and begin()
  /// wasn't called before !
  static typename InstanceHash::iterator end() {
    return instances->end();
  };

  

  /// The name of the instance
  static QString instanceName(C * instance) {
    return instance->name;
  };

  // /// The name of the instance. SFINAE overload to handle both the
  // /// cases where the name is an attribute or a function.
  // SFINAE doesn't work, it seems
  // static QString instanceName(C * instance) {
  //   return instance->name();
  // };

  /// Registers the instance again. Throw an exception if already
  /// defined.
  static void registerInstance(C * instance) {
    if(! instances)
      instances = new QHash<QString, C * >();

    QString n = instanceName(instance);
    
    if(instances->contains(n))
      throw InternalError("Global register already contains "
                          "an instance of %1").arg(n);
    (*instances)[n] = instance;
  };


public:  
  /// Returns the item in the namedinstance bearing the given name.
  static C * namedItem(const QString & n) {
    if(! instances)
      return NULL;
    return instances->value(n, NULL);
  };


  /// Returns all the available items
  static QStringList availableItems() {
    if(! instances)
      return QStringList();
    return instances->keys();
  };
  
  
}; 

template <typename C> 
QHash<QString, C * > * NamedInstance<C>::instances = NULL;

#endif
