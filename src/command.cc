/*
  command.cc: implementation of the Command class
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
#include <command.hh>
#include <group.hh>
#include <argument.hh>
#include <argumentmarshaller.hh>


QHash<QString, Command*> * Command::availableCommands = NULL;

void Command::registerCommand(Command * cmd)
{
  if(! availableCommands)
    availableCommands = new QHash<QString, Command*>;

  if(availableCommands->contains(cmd->commandName())) {
    QString str = "Duplicate command name : " + cmd->commandName();
    throw std::logic_error(str.toStdString());
  }
  (*availableCommands)[cmd->commandName()] = cmd;

  if(cmd->shortCommandName().isEmpty())
    return;
  if(availableCommands->contains(cmd->shortCommandName())) {
    QString str = "Duplicate short command name : " + cmd->shortCommandName();
    throw std::logic_error(str.toStdString());
  }
  (*availableCommands)[cmd->shortCommandName()] = cmd;
}

void Command::crosslinkCommands()
{
  for(QHash<QString, Command *>::iterator i = availableCommands->begin();
      i != availableCommands->end(); i++) {
    if(! i.value()->group) {
      Group * grp = Group::namedGroup(i.value()->groupName);
      i.value()->group = grp;
      grp->commands.append(i.value());
    }
  }
}
