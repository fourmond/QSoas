/**
   \file factory.hh
   Template class for a factory design
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
#ifndef __FACTORY_HH
#define __FACTORY_HH

#include <exceptions.hh>

/// This class provides a factory/creator scheme.
///
/// C is the class, Args is the arguments to the creation function.
template <typename C, typename... Args> class Factory {
protected:
  /// The global register for the factory.
  ///
  /// Must be a pointer to ensure we don't initialize the hash after
  /// the first registration !
  static QHash<QString, Factory<C, Args...> * > * factory;

  /// Registers the instance again. Throw an exception if already
  /// defined.
  static void registerInstance(Factory * instance) {
    if(! factory)
      factory = new QHash<QString, Factory<C, Args...> * >();
    
    if(factory->contains(instance->name))
      throw InternalError("Factory already contains an instance of %1").
        arg(instance->name);
    (*factory)[instance->name] = instance;
  };

public:
  typedef std::function<C * (Args...)> Creator;

  /// The code-like name for the creator
  QString name;

  /// A nicer (optional) description for the creator
  QString description;

  /// The creation function. Can be a normal function, or a
  /// lambda. Whatever.
  Creator creator;

  Factory(const QString & n, 
          const Creator & c,
          const QString & desc = "") : 
    name(n), description(desc), creator(c) {
    registerInstance(this);
  };

  
  /// Returns the item in the factory bearing the given name.
  static Factory * namedItem(const QString & n) {
    if(! factory)
      return NULL;
    return factory->value(n, NULL);
  };

  /// Creates a object for the given stuff.
  static C * createObject(const QString & n, Args... arguments) {
    Factory * f = namedItem(n);
    if(! f)
      throw RuntimeError("Trying to instanciate '%1', which is not known").
        arg(n);
    return f->creator(arguments...);
  };
  
  
}; 


template <typename C, typename... Args>
QHash<QString, Factory<C, Args...> * > * Factory<C, Args...>::factory = NULL;

#endif
