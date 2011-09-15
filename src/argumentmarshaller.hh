/**
   \file argumentmarshaller.hh
   Argumentmarshaller handling for QSoas.
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

#ifndef __ARGUMENTMARSHALLER_HH
#define __ARGUMENTMARSHALLER_HH

#include <exceptions.hh>

/// A base class for handling type-safe argument list passing
class ArgumentMarshaller {
public:

  template<typename T> T value() const;
  template<typename T> T & value();
  virtual ~ArgumentMarshaller() {;};
};

template <typename T> class ArgumentMarshallerChild : 
  public ArgumentMarshaller {
public:
  T value;

  ArgumentMarshallerChild(const T & t) : value(t) {;};
};

template<typename T> T ArgumentMarshaller::value() const {
  const ArgumentMarshallerChild<T> * child = 
    dynamic_cast< const ArgumentMarshallerChild<T> *>(this);
  if(! child)
    throw InternalError("Bad argument type conversion attempted");
  return child->value;
};

template<typename T> T & ArgumentMarshaller::value() {
  ArgumentMarshallerChild<T> * child = 
    dynamic_cast<ArgumentMarshallerChild<T> *>(this);
  if(! child)
    throw InternalError("Bad argument type conversion attempted");
  return child->value;
};

// A few useful typedefs:
typedef QList<ArgumentMarshaller *> CommandArguments;
typedef QHash<QString, ArgumentMarshaller *> CommandOptions;

#endif
