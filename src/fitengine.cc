/*
  fitengine.cc: implementation of FitEngine and derived classes
  Copyright 2012, 2013 by Vincent Fourmond

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
#include <fitdata.hh>
#include <fitengine.hh>
#include <exceptions.hh>

FitEngineFactoryItem::FitEngineFactoryItem(const QString & n, 
                                           const QString & pn, Creator c):
  creator(c), name(n), publicName(pn)
{
  FitEngine::registerFactoryItem(this);
}


//////////////////////////////////////////////////////////////////////

StoredParameters::StoredParameters(const gsl_vector *v, double r) :
  parameters(v->size, 0),
  residuals(r)
{
  for(int i = 0; i < v->size; i++)
    parameters[i] = gsl_vector_get(v, i);
}


const gsl_vector * StoredParameters::toGSLVector() const
{
  // Ugly, but as I can't stored a gsl_const_view...
  view = gsl_vector_view_array(const_cast<double*>(parameters.data()), 
                               parameters.size());
  return &view.vector;
}


//////////////////////////////////////////////////////////////////////

QHash<QString, FitEngineFactoryItem*> * FitEngine::factory = NULL;

void FitEngine::registerFactoryItem(FitEngineFactoryItem * item)
{
  if(! factory)
    factory = new QHash<QString, FitEngineFactoryItem*>();

  if(factory->contains(item->name))
    throw InternalError(QString("Defining the same fit engine twice: %1").
                        arg(item->name));
  factory->insert(item->name, item);
}

FitEngineFactoryItem * FitEngine::namedFactoryItem(const QString & name)
{
  if(! factory)
    return NULL;
  return factory->value(name, NULL);
}

FitEngineFactoryItem * FitEngine::defaultFactoryItem()
{
  // For now, it fails on mac, so we don't use it.
  FitEngineFactoryItem * it = namedFactoryItem("odrpack");
  if(it)
    return it;
  return namedFactoryItem("lmsder"); // For now
}

QStringList FitEngine::availableEngines()
{
  if(factory)
    return factory->keys();
  return QStringList();
}

FitEngine * FitEngine::createEngine(const QString & name, FitData * data)
{
  FitEngineFactoryItem * it = namedFactoryItem(name);
  if(! it)
    return NULL;
  return it->creator(data);
}


FitEngine::FitEngine(FitData * d) : fitData(d) 
{
}

void FitEngine::copyCurrentParameters(gsl_vector * target) const
{
  gsl_vector_memcpy(target, currentParameters());
}

FitEngine::~FitEngine()
{
}

void FitEngine::pushCurrentParameters()
{
  Vector v;
  v.resize(fitData->freeParameters());
  gsl_vector_view vi  = gsl_vector_view_array(v.data(), v.size());
  copyCurrentParameters(&vi.vector);
  storedParameters << StoredParameters(v, residuals());
}

