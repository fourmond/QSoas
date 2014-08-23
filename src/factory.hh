/**
   \file factory.hh
   Template class for a factory design
   Copyright 2013, 2014 by CNRS/AMU

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

#include <namedinstance.hh>
#include <exceptions.hh>

/// This class provides a factory/creator scheme.
///
/// C is the class, Args is the arguments to the creation function.
template <typename C, typename... Args> class Factory : 
  public NamedInstance<Factory<C, Args...> > {
protected:

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
    NamedInstance<Factory<C, Args...> >::registerInstance(this);
  };

  Factory(const QString & n, 
          const QString & desc, 
          const Creator & c) :
    name(n), description(desc), creator(c) {
    NamedInstance<Factory<C, Args...> >::registerInstance(this);
  };

  /// Returns all the available items
  static QStringList availableItems() {
    return NamedInstance<Factory<C, Args...> >::availableItems();
  };

  /// Returns the item in the namedinstance bearing the given name.
  static Factory * namedItem(const QString & n) {
    return NamedInstance<Factory<C, Args...> >::namedItem(n);
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



#endif
