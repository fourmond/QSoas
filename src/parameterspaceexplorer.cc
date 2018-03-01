/*
  parameterspaceexplorer.cc: implementation of the style generators
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
#include <parameterspaceexplorer.hh>

#include <commandeffector.hh>
#include <commandcontext.hh>
#include <command.hh>

#include <fitworkspace.hh>

QHash<QString, ParameterSpaceExplorerFactoryItem*> * ParameterSpaceExplorer::factory = NULL;


ParameterSpaceExplorerFactoryItem::
ParameterSpaceExplorerFactoryItem(const QString & n,
                                    const QString & pn,
                                    ParameterSpaceExplorerFactoryItem::Creator c) : creator(c), name(n), publicName(pn)
{
  ParameterSpaceExplorer::registerFactoryItem(this);
}

//////////////////////////////////////////////////////////////////////

void ParameterSpaceExplorer::registerFactoryItem(ParameterSpaceExplorerFactoryItem * item)
{
  if(! factory)
    factory = new QHash<QString, ParameterSpaceExplorerFactoryItem*>;
  (*factory)[item->name] = item;
}

ParameterSpaceExplorer::ParameterSpaceExplorer(FitWorkspace * ws) :
  workSpace(ws) 
{
}

ParameterSpaceExplorer::~ParameterSpaceExplorer()
{
}

ParameterSpaceExplorerFactoryItem * ParameterSpaceExplorer::namedFactoryItem(const QString & name)
{
  if(! factory)
    return NULL;
  return factory->value(name, NULL);
}

ParameterSpaceExplorer * ParameterSpaceExplorer::createExplorer(const QString & name, 
                                                      FitWorkspace * ws)
{
  ParameterSpaceExplorerFactoryItem * fact = namedFactoryItem(name);
  if(fact)
    return fact->creator(ws);
  return NULL;
}

QHash<QString, QString> ParameterSpaceExplorer::availableExplorers()
{
  QHash<QString, QString> ret;
  if(! factory)
    return ret;
  
  for(QHash<QString, ParameterSpaceExplorerFactoryItem*>::iterator i = 
        factory->begin(); i != factory->end(); i++)
    ret[i.value()->publicName] = i.value()->name;

  return ret;
}

class RawCommandEffector : public CommandEffector {

  typedef std::function< void (const QString &, 
                               const CommandArguments &,
                               const CommandOptions &) > EffectorCode;
  EffectorCode code;
  
public:

  RawCommandEffector(const EffectorCode & c) :
    CommandEffector(false),
    code(c) {
  }
  
  virtual void runCommand(const QString & commandName, 
                          const CommandArguments & arguments,
                          const CommandOptions & options) {
    code(commandName, arguments, options);
  };

};

CommandEffector * ParameterSpaceExplorer::explorerEffector(const QString & n)
{
  return new RawCommandEffector([n](const QString & commandName, 
                                    const CommandArguments & arguments,
                                    const CommandOptions & options) {
                                  FitWorkspace * ws =
                                    FitWorkspace::currentWorkspace();
                                  ParameterSpaceExplorer * expl =
                                    createExplorer(n, ws);
                                  ws->setExplorer(expl);
                                  expl->setup(arguments, options);
                                });
}

QList<Command *> ParameterSpaceExplorer::createCommands(FitWorkspace * workspace)
{
  QList<Command *> rv;

  if(factory) {
    for(const QString & n : factory->keys()) {
      ParameterSpaceExplorer * expl = createExplorer(n, workspace);
      ParameterSpaceExplorerFactoryItem * item = (*factory)[n];
      QString cmdName = n + "-explorer";
      rv << new Command(cmdName.toLocal8Bit().data(),
                        explorerEffector(n), "fit",
                        expl->explorerArguments(),
                        expl->explorerOptions(),
                        item->publicName.toLocal8Bit().data(),
                        "", "",
                        CommandContext::fitContext());
      delete expl;
    }
  }
  return rv;
}

