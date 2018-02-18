/*
  command.cc: implementation of the Command class
  Copyright 2018 by CNRS/AMU

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
#include <commandcontext.hh>
#include <command.hh>

QHash<QString, CommandContext*> * CommandContext::availableContexts = NULL;

CommandContext * CommandContext::namedContext(const QString & name,
                                              const QString & prefix,
                                              const QString & rubyClass)
{
  if(! availableContexts)
    availableContexts = new QHash<QString, CommandContext*>;
  if(! availableContexts->contains(name))
    (*availableContexts)[name] = new CommandContext(prefix, rubyClass);
  return (*availableContexts)[name];
}

CommandContext * CommandContext::globalContext()
{
  return namedContext("global", "", "QSoas");
}

CommandContext * CommandContext::fitContext()
{
  return namedContext("fit", "f", "QSoasFit");
}


CommandContext::CommandContext(const QString & p,
                               const QString & cls) :
  prefix(p), rubyClass(cls)
{
}


Command * CommandContext::namedCommand(const QString & cmd,
                                       bool convert) const
{
  QString cnv = cmd;
  QString prf = prefix;
  if(! prf.isEmpty()) {
    prf += "+";
    if(cnv.startsWith(prf))
      cnv = cnv.mid(prf.size());
  }
  if(convert)
    cnv.replace('_', '-');
  return commands.value(cnv, NULL);
}



void CommandContext::registerCommand(Command * cmd)
{

  if(commands.contains(cmd->commandName()))
    throw InternalError(QObject::tr("Duplicate command name : %1").
                        arg(cmd->commandName()));

  commands[cmd->commandName()] = cmd;

  if(cmd->shortCommandName().isEmpty())
    return;
  if(commands.contains(cmd->shortCommandName()))
    throw InternalError(QObject::tr("Duplicate short command name : %1").
                        arg(cmd->shortCommandName()));
  commands[cmd->shortCommandName()] = cmd;
}

QStringList CommandContext::commandNames() const
{
  return commands.keys();
}

QSet<Command *> CommandContext::availableCommands() const
{
  return QSet<Command * >::fromList(commands.values());
}
