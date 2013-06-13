/*
  parameterspaceexplorator.cc: implementation of the style generators
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
#include <parameterspaceexplorator.hh>

#include <graphicssettings.hh>
#include <soas.hh>

QHash<QString, ParameterSpaceExploratorFactoryItem*> * ParameterSpaceExplorator::factory = NULL;


ParameterSpaceExploratorFactoryItem::
ParameterSpaceExploratorFactoryItem(const QString & n,
                                    const QString & pn,
                                    ParameterSpaceExploratorFactoryItem::Creator c) : creator(c), name(n), publicName(pn)
{
  ParameterSpaceExplorator::registerFactoryItem(this);
}

//////////////////////////////////////////////////////////////////////

void ParameterSpaceExplorator::registerFactoryItem(ParameterSpaceExploratorFactoryItem * item)
{
  if(! factory)
    factory = new QHash<QString, ParameterSpaceExploratorFactoryItem*>;
  (*factory)[item->name] = item;
}

ParameterSpaceExplorator::ParameterSpaceExplorator(FitData * d) :
  data(d) 
{
}

ParameterSpaceExplorator::~ParameterSpaceExplorator()
{
}

ParameterSpaceExploratorFactoryItem * ParameterSpaceExplorator::namedFactoryItem(const QString & name)
{
  if(! factory)
    return NULL;
  return factory->value(name, NULL);
}

ParameterSpaceExplorator * ParameterSpaceExplorator::createExplorator(const QString & name, 
                                                      FitData * d)
{
  ParameterSpaceExploratorFactoryItem * fact = namedFactoryItem(name);
  if(fact)
    return fact->creator(d);
  return NULL;
}

QHash<QString, QString> ParameterSpaceExplorator::availableExplorators()
{
  QHash<QString, QString> ret;
  if(! factory)
    return ret;
  
  for(QHash<QString, ParameterSpaceExploratorFactoryItem*>::iterator i = 
        factory->begin(); i != factory->end(); i++)
    ret[i.value()->publicName] = i.value()->name;

  return ret;
}

void ParameterSpaceExplorator::resultingTrajectory(const FitTrajectory & )
{
  // no-op
}
