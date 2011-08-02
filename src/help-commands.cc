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

/// Help commands
namespace HelpCommands {
  static Group help("help", 1000,
                    QT_TR_NOOP("Help"),
                    QT_TR_NOOP("Help"));

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
    Terminal::out << "<b>All commands:</b>"
                  << "<table>";
    for(int i = 0; i < cmds.size(); i++) {
      Command * cmd = Command::namedCommand(cmds[i]);
      Terminal::out << QString("<tr><td>%1 </td><td> %2</td></tr>").
        arg(cmds[i], len).arg(cmd->shortDescription());
      t[cmd] = 1;
    }
    Terminal::out << "</table>" 
                  << "In total " << t.size() << " commands "
                  << "(and " << cmds.size() - t.size() << " aliases)"
                  << endl;
  }

  static Command 
  quit("commands", // command name
       optionLessEffector(commandsCommand), // action
       "help",  // group name
       NULL, // arguments
       NULL, // options
       QT_TR_NOOP("Commands"),
       QT_TR_NOOP("List commands"),
       QT_TR_NOOP("List all available commands, along with a little help"));

  //////////////////////////////////////////////////////////////////////
}

