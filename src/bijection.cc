/*
  bijection.cc: implementation of the bijections infrastructure
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

#include <headers.hh>
#include <bijection.hh>


QHash<QString, BijectionFactoryItem*> * Bijection::factory = NULL;


BijectionFactoryItem::BijectionFactoryItem(BijectionFactoryItem::Creator c) :
  creator(c) {
  Bijection * bij = creator();
  name = bij->name();
  publicName = bij->publicName();
  parameters = bij->parameters();
  delete bij;

  Bijection::registerFactoryItem(this);
}

//////////////////////////////////////////////////////////////////////

void Bijection::registerFactoryItem(BijectionFactoryItem * item)
{
  if(! factory)
    factory = new QHash<QString, BijectionFactoryItem*>;
  (*factory)[item->name] = item;
}

BijectionFactoryItem * Bijection::namedFactoryItem(const QString & name)
{
  if(! factory)
    return NULL;
  return factory->value(name, NULL);
}

Bijection * Bijection::createNamedBijection(const QString & name)
{
  BijectionFactoryItem * fact = namedFactoryItem(name);
  if(fact)
    return fact->creator();
  return NULL;
}

QStringList Bijection::parameters() const
{
  return QStringList();
}

QList<const BijectionFactoryItem *> Bijection::factoryItems()
{
  QList<const BijectionFactoryItem *> ret;
  if(! factory)
    return ret;

  for(QHash<QString, BijectionFactoryItem*>::const_iterator i = 
        factory->begin(); i != factory->end(); i++)
    ret << i.value();
  return ret;
}

QString Bijection::saveAsText() const
{
  QString ret = name();
  int nbparams = parameters().size();

  if(nbparams > 0) {
    ret += ":";
    for(int i = 0; i < nbparams; i++)
      ret += " " + QString::number(parameterValues[i]);
  }
  return ret;
}

Bijection * Bijection::loadFromText(const QString & spec)
{
  QStringList lst = spec.split(QRegExp("\\s*:\\s*"));
  Bijection * bijection = createNamedBijection(lst[0]);

  /// @todo This probably should join a virtual initialization
  /// function.
  if(! bijection)
    return NULL;
  int nbparams = bijection->parameters().size();
  if(nbparams) {
    bijection->parameterValues.resize(nbparams);
#undef lst2
    QStringList lst2 = lst[1].split(QRegExp("\\s+"));
    for(int i = 0; i < nbparams; i++)
      bijection->parameterValues[i] = lst2.value(i, "").toDouble();
  }

  return bijection;
}


double Bijection::derivationStep(double value) const
{
  return 3e-4 * value;
}
