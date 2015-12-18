/**
   \file factoryargument.hh
   Template class for a arguments giving factory items
   Copyright 2013 by CNRS/AMU

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
#ifndef __FACTORYARGUMENT_HH
#define __FACTORYARGUMENT_HH

#include <argument.hh>
#include <factory.hh>

#include <utils.hh>

/// @todo An option to return either the name of the Factory item.
template <typename F> class FactoryArgument : 
  public  Argument {

  QString choiceName;
public:
  FactoryArgument(const char * cn, const char * pn,
                  const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  };

  /// Returns a wrapped Factory<.., ..>
  virtual ArgumentMarshaller * fromString(const QString & str) const {
    F * item = F::namedItem(str);
    if(! item)
      throw RuntimeError("No such item: '%1'").arg(str);
    return new ArgumentMarshallerChild< F * >(item);
  };

  QStringList proposeCompletion(const QString & starter) const
  {
    return Utils::stringsStartingWith(F::availableItems(), 
                                      starter);
  };

  virtual QString typeName() const {
    if(choiceName.isEmpty())
      return "choice";
    return choiceName;
  };

  virtual QString typeDescription() const {
    QStringList cs = F::availableItems();
    qSort(cs);
    return QString("One of: `%1`").arg(cs.join("`, `"));
  };


};

#endif
