/*
  fitengine.cc: implementation of FitEngine and derived classes
  Copyright 2012, 2013, 2014, 2015 by CNRS/AMU

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
#include <fitengine.hh>
#include <fitdata.hh>
#include <exceptions.hh>

#include <valuehash.hh>

#include <factory.hh>

#include <commandeffector.hh>
#include <commandeffector-templates.hh>
#include <commandcontext.hh>
#include <command.hh>

#include <idioms.hh>

#include <fitworkspace.hh>

StoredParameters::StoredParameters(const gsl_vector *v, double r) :
  parameters(v->size, 0),
  residuals(r)
{
  for(size_t i = 0; i < v->size; i++)
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

FitEngineFactoryItem::FitEngineFactoryItem(const QString & n, 
                                           const QString & desc,
                                           const Creator & c,
                                           ArgumentList * options,
                                           bool mc) :
  Factory<FitEngine, FitData *>(n, c, desc), engineOptions(options),
  isMultiCapable(mc)
{
  fitEngineCommand = FitEngine::createCommand(this);
}


//////////////////////////////////////////////////////////////////////

FitEngineFactoryItem * FitEngine::namedFactoryItem(const QString & name)
{
  return FitEngineFactoryItem::namedItem(name);
}

FitEngineFactoryItem * FitEngine::defaultFactoryItem(int nb)
{
  // For now, it fails on mac, so we don't use it.
  FitEngineFactoryItem * it;
  if(nb < 4) {
     it = namedFactoryItem("odrpack");
    if(it)
      return it;
  }
  if(nb > FitEngine::buffersForMulti) {
    it = namedFactoryItem("multi");
    if(it)
      return it;
  }
  it = namedFactoryItem("qsoas");
  if(it)
    return it;
  // Default to the first one.
  return namedFactoryItem(availableEngines()[0]); 
}

CommandEffector * FitEngine::engineEffector(const QString & n)
{
  return new RawCommandEffector([n](const QString & commandName, 
                                    const CommandArguments & arguments,
                                    const CommandOptions & options) {
                                  FitWorkspace * ws =
                                    FitWorkspace::currentWorkspace();
                                  FitEngineFactoryItem * it =
                                    namedFactoryItem(n);
                                  ws->setFitEngineFactory(it, options);
                                });
}

Command * FitEngine::createCommand(FitEngineFactoryItem * item)
{
  QString n = item->name;
  QString cmdName = n + "-engine";
  return new Command(cmdName.toLocal8Bit().data(),
                     engineEffector(n), "fits",
                     NULL,
                     item->engineOptions,
                     n.toLocal8Bit().data(),
                     "", "",
                     CommandContext::fitContext());
}

// QList<Command *> FitEngine::createCommands()
// {
//   QList<Command * > rv;
//   for(const QString & n : availableEngines())
//     rv << createCommand(FitEngineFactoryItem::namedItem(n));
//   return rv;
// }

bool FitEngine::handlesWeights() const
{
  return false;
}

QStringList FitEngine::availableEngines()
{
  return FitEngineFactoryItem::availableItems();
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
  fitData->engine = this;
}

void FitEngine::copyCurrentParameters(gsl_vector * target) const
{
  gsl_vector_memcpy(target, currentParameters());
}

FitEngine::~FitEngine()
{
  // fitData->engine = NULL;
}


void FitEngine::pushCurrentParameters()
{
  Vector v;
  v.resize(fitData->freeParameters());
  gsl_vector_view vi  = gsl_vector_view_array(v.data(), v.size());
  copyCurrentParameters(&vi.vector);
  storedParameters << StoredParameters(v, residuals());
}

void FitEngine::recomputeJacobian()
{
  throw RuntimeError("The fit engine does not support recomputing the jacobian separately");
}

void FitEngine::resetEngineParameters()
{
}

ArgumentList * FitEngine::engineOptions() const
{
  return NULL;
}

CommandOptions FitEngine::getEngineParameters() const
{
  return CommandOptions();
}

void FitEngine::setEngineParameters(const CommandOptions & parameters)
{
}
