/**
   \file help-commands.cc get help!
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
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <terminal.hh>

#include <soas.hh>

static Group help("help", 1000,
                  "Help",
                  "Help");

//////////////////////////////////////////////////////////////////////


static void commandsCommand(const QString &)
{
  QStringList cmds = Command::allCommands();
  cmds.sort();
  int len = 0;
  for(int i = 0; i < cmds.size(); i++)
    if(cmds[i].size() > len)
      len = cmds[i].size();

  QHash<Command *, int> t;
  Terminal::out << "All commands:" << endl;
  for(int i = 0; i < cmds.size(); i++) {
    Command * cmd = Command::namedCommand(cmds[i]);
    Terminal::out << QString("%1 %2\n").
      arg(cmds[i], -len).arg(cmd->shortDescription());
    t[cmd] = 1;
  }
  Terminal::out << "\nIn total " << t.size() << " commands "
                << "(and " << cmds.size() - t.size() << " aliases)"
                << endl;
}

static Command 
cmds("commands", // command name
     optionLessEffector(commandsCommand), // action
     "help",  // group name
     NULL, // arguments
     NULL, // options
     "Commands",
     "List commands",
     "List all available commands, along with a little help");

//////////////////////////////////////////////////////////////////////


static void helpCommand(const QString &, QString command)
{
  Command * cmd = Command::namedCommand(command);
  
  QStringList synopsis;
  QString descs;
  if(cmd->commandArguments()) {
    const ArgumentList & args = *cmd->commandArguments();
    for(int i = 0; i < args.size(); i++) {
      QString a = args[i]->argumentName();
      if(args[i]->greedy)
        a += "...";
      synopsis << a;
      descs += QString("  * %1: %2\n").
        arg(args[i]->argumentName()).
        arg(args[i]->description());
    }
  }

  if(cmd->commandOptions()) {
    const ArgumentList & args = *cmd->commandOptions();
    for(int i = 0; i < args.size(); i++) {
      QString a = args[i]->argumentName();
      synopsis << "/" + a + "=" ;
      descs += QString("  * /%1: %2\n").
        arg(args[i]->argumentName()).
        arg(args[i]->description());
    }
  }

  Terminal::out << "Command: " << command << " -- "
                << cmd->publicName() << "\n\n"
                << "  " << command << " " 
                << synopsis.join(" ") << "\n" 
                << descs << "\n"
                << cmd->longDescription() << endl;
}

static ArgumentList 
helpA(QList<Argument *>() 
      << new ChoiceArgument(&Command::allCommands,
                            "command", "Command",
                            "The command on which to give help"));


static Command 
hlpc("help", // command name
     optionLessEffector(helpCommand), // action
     "help",  // group name
     &helpA, // arguments
     NULL, // options
     "Help on...",
     "Give help on command",
     "Gives all help available on the given command",
     "?");

//////////////////////////////////////////////////////////////////////
