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
#include <curveview.hh>
#include <soas.hh>

namespace Groups {
  static Group stack("stack", 1,
                     QT_TR_NOOP("Data Stack"),
                     QT_TR_NOOP("Data stack manipulation"));
}

//////////////////////////////////////////////////////////////////////

namespace DataStackCommands {

  static void loadFilesAndDisplay(int nb, QStringList files)
  {
    for(int i = 0; i < files.size(); i++) {
      Terminal::out << "Loading file '" << files[i] << "'" << endl;
      try {
        DataSet * s = DataBackend::loadFile(files[i]);
        soas().stack().pushDataSet(s);
        if(nb > 0)
          soas().view().addDataSet(s);
        else
          soas().view().showDataSet(s);
        nb++;
      }
      catch (const std::runtime_error & e) {
        Terminal::out << e.what() << endl;
      }
    }
  }

  static ArgumentList 
  loadArgs(QList<Argument *>() 
           << new SeveralFilesArgument("file", 
                                       QT_TR_NOOP("File"),
                                       QT_TR_NOOP("Files to load !"), true
                                       ));
                             


  static void loadCommand(const QString & name, QStringList files)
  {
    /// @todo add the possibility to select the backend
    loadFilesAndDisplay(0, files);
  }


  static Command 
  load("load", // command name
       CommandEffector::functionEffectorOptionLess(loadCommand), // action
       "stack",  // group name
       &loadArgs, // arguments
       NULL, // options
       QT_TR_NOOP("Load"),
       QT_TR_NOOP("Loads one or several files"),
       QT_TR_NOOP("Loads the given files and push them onto the data stack"),
       "l");
  //////////////////////////////////////////////////////////////////////

  static void overlayFilesCommand(const QString & name, QStringList files)
  {
    loadFilesAndDisplay(1, files);
  }


  static Command 
  ovl("overlay", // command name
      CommandEffector::functionEffectorOptionLess(overlayFilesCommand), // action
      "stack",  // group name
      &loadArgs, // arguments
      NULL, // options
      QT_TR_NOOP("Overlay"),
      QT_TR_NOOP("Loads files and overlay them"),
      QT_TR_NOOP("Loads the given files and push them onto the data "
                 "stack, adding them to the display at the same time"),
      "v");

  //////////////////////////////////////////////////////////////////////

  static void showStackCommand(const QString & name)
  {
    soas().stack().showStackContents();
  }


  static Command 
  showStack("show-stack", // command name
            CommandEffector::functionEffectorOptionLess(showStackCommand), // action
            "stack",  // group name
            NULL, // arguments
            NULL, // options
            QT_TR_NOOP("Show stack"),
            QT_TR_NOOP("Shows the stack contents"),
            QT_TR_NOOP("Shows a small summary of what the stack is made of"));

}
