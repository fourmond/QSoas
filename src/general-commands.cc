/**
   \file general-commands.cc various general purpose commands and groups
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

static Group file("file", 0,
                  QT_TRANSLATE_NOOP("Groups", "File"),
                  QT_TRANSLATE_NOOP("Groups", "General purpose commands"));


static void quitCommand(const QString & name)
{
  /// @todo prompt if the short name was used.
  qApp->quit();
}

static Command 
quit("quit", // command name
     CommandEffector::functionEffectorOptionLess(quitCommand), // action
     "file",  // group name
     NULL, // arguments
     NULL, // options
     QT_TRANSLATE_NOOP("Commands", "Quit"),
     QT_TRANSLATE_NOOP("Commands", "Quit QSoas"),
     QT_TRANSLATE_NOOP("Commands", 
                       "Exits QSoas, losing all the current session"),
     "q");

static ArgumentList 
loadArgs(QList<Argument *>() 
         << new FileArgument("file", 
                             QT_TRANSLATE_NOOP("Arguments", "File")));
                             

static void loadCommand(const QString & name, QString file)
{
  Terminal::out << "Should load file '" << file << "'" << endl;
}


static Command 
load("load", // command name
     CommandEffector::functionEffectorOptionLess(loadCommand), // action
     "file",  // group name
     &loadArgs, // arguments
     NULL, // options
     QT_TRANSLATE_NOOP("Commands", "Quit"),
     QT_TRANSLATE_NOOP("Commands", "Quit QSoas"),
     QT_TRANSLATE_NOOP("Commands", 
                       "Exits QSoas, losing all the current session"),
     "l");



