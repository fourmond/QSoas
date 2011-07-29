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

#include <datastack.hh>
#include <databackend.hh>

static Group file("stack", 1,
                  QT_TRANSLATE_NOOP("Groups", "Data Stack"),
                  QT_TRANSLATE_NOOP("Groups", "Data stack manipulation"));

//////////////////////////////////////////////////////////////////////

static ArgumentList 
loadArgs(QList<Argument *>() 
         << new SeveralFilesArgument("file", 
                                     QT_TRANSLATE_NOOP("Arguments", "File"),
                                     QT_TRANSLATE_NOOP("Arguments", "Files to load !"), true
                                     ));
                             


static void loadCommand(const QString & name, QStringList files)
{
  /// @todo add the possibility to select the backend
  for(int i = 0; i < files.size(); i++) {
    Terminal::out << "Loading file '" << files[i] << "'" << endl;
    try {
      DataSet * s = DataBackend::loadFile(files[i]);
      DataStack::dataStack()->pushDataSet(s);
      /// @todo Display loaded files !
    }
    catch (const std::runtime_error & e) {
      Terminal::out << e.what() << endl;
    }
  }
}


static Command 
load("load", // command name
     CommandEffector::functionEffectorOptionLess(loadCommand), // action
     "stack",  // group name
     &loadArgs, // arguments
     NULL, // options
     QT_TRANSLATE_NOOP("Commands", "Load"),
     QT_TRANSLATE_NOOP("Commands", "Loads one or several files"),
     QT_TRANSLATE_NOOP("Commands", 
                       "Loads the given files and push them onto the data stack"),
     "l");

//////////////////////////////////////////////////////////////////////

static void showStackCommand(const QString & name)
{
  DataStack::dataStack()->showStackContents();
}


static Command 
showStack("show-stack", // command name
          CommandEffector::functionEffectorOptionLess(showStackCommand), // action
          "stack",  // group name
          NULL, // arguments
          NULL, // options
          QT_TRANSLATE_NOOP("Commands", "Show stack"),
          QT_TRANSLATE_NOOP("Commands", "Shows the stack contents"),
          QT_TRANSLATE_NOOP("Commands", 
                            "Shows a small summary of what the stack is made of"));

