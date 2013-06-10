/*
  stylegenerator.cc: implementation of the style generators
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
#include <stylegenerator.hh>


QHash<QString, StyleGeneratorFactoryItem*> * StyleGenerator::factory = NULL;


StyleGeneratorFactoryItem::StyleGeneratorFactoryItem(const QString & n,
                                                     const QString & pn,
                                                     StyleGeneratorFactoryItem::Creator c) : creator(c), name(n), publicName(pn)
{
  StyleGenerator::registerFactoryItem(this);
}

//////////////////////////////////////////////////////////////////////

void StyleGenerator::registerFactoryItem(StyleGeneratorFactoryItem * item)
{
  if(! factory)
    factory = new QHash<QString, StyleGeneratorFactoryItem*>;
  (*factory)[item->name] = item;
}

StyleGeneratorFactoryItem * StyleGenerator::namedFactoryItem(const QString & name)
{
  if(! factory)
    return NULL;
  return factory->value(name, NULL);
}

StyleGenerator * StyleGenerator::createNamedGenerator(const QString & name, 
                                                      int nb, const QString & a)
{
  StyleGeneratorFactoryItem * fact = namedFactoryItem(name);
  if(fact)
    return fact->creator(nb, a);
  return NULL;
}

