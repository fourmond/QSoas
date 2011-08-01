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

#include <vector.hh>
#include <dataset.hh>
#include <databackend.hh>

#include <curveeventloop.hh>
#include <curveitems.hh>
#include <curveview.hh>
#include <soas.hh>

#include <debug.hh>


/// The namespace holding all the groups.
namespace Groups {
  static Group file("file", 0,
                    QT_TR_NOOP("File"),
                    QT_TR_NOOP("General purpose commands"));
}

/// A private namespace holding general-purpose commands
namespace GeneralCommands {
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
       QT_TR_NOOP("Quit"),
       QT_TR_NOOP("Quit QSoas"),
       QT_TR_NOOP("Exits QSoas, losing all the current session"),
       "q");

  //////////////////////////////////////////////////////////////////////

  static void antialiasCommand(const QString & name)
  {
    soas().setAntiAlias(! soas().antiAlias());
    Terminal::out << "Antialiasing now " 
                  << (soas().antiAlias() ? "on" : "off") 
                  << endl;
    if(! soas().openGL())
      Terminal::out << "You may want to use the command opengl "
                    << "to speed up rendering"
                    << endl;
  }

  static Command 
  aa("antialias", // command name
     CommandEffector::functionEffectorOptionLess(antialiasCommand), // action
     "file",  // group name
     NULL, // arguments
     NULL, // options
     QT_TR_NOOP("Antialias"),
     QT_TR_NOOP("Toggle anti-aliasing"),
     QT_TR_NOOP("Toggles anti-aliasing for graphics rendering."),
     "AA");

  //////////////////////////////////////////////////////////////////////

  static void openglCommand(const QString & name)
  {
    soas().setOpenGL(! soas().openGL());
    Terminal::out << "The use of OpenGL is now " 
                  << (soas().openGL() ? "on" : "off") 
                  << endl;
  }

  static Command 
  ogl("opengl", // command name
      CommandEffector::functionEffectorOptionLess(openglCommand), // action
      "file",  // group name
      NULL, // arguments
      NULL, // options
      QT_TR_NOOP("OpenGL"),
      QT_TR_NOOP("Toggle OpenGL"),
      QT_TR_NOOP("Toggles the use of OpenGL for graphics rendering.")
      );

  //////////////////////////////////////////////////////////////////////

  static void testELoopCommand(const QString & name)
  {
    Debug::dumpCurrentFocus("Focus before creation: ");
    CurveEventLoop loop;
    Debug::dumpCurrentFocus("Focus after creation: ");
    CurveLine l;
    CurveMarker m;
    CurvePanel p;
    CurveView & view = soas().view();
    l.pen = QPen("black");
    view.addItem(&l);
    view.addItem(&m);
    view.addPanel(&p);
    QTextStream o(stdout);
    int i = 0;
    m.size = 5;
    m.pen = QPen(QColor("red"), 2);
    m.brush = QBrush("blue");
    Debug::dumpCurrentFocus("Focus before loop: ");
    while(! loop.finished()) {
      Debug::dumpCurrentFocus("Current focus: ");
      o << "Event: " << loop.type()
        << ", key " << QString("0x%1").arg(loop.key(), 8, 16, 
                                           QChar('0')) << endl;
      if(loop.type() == QEvent::MouseButtonPress) {
        QPointF p = loop.position();
        o << "Press event at " << p.x() << "," << p.y() << endl;
      
        if(i % 2)
          l.p2 = p;
        else
          l.p1 = p;
        m.p = p;
        i++;
      }
    }
    Debug::dumpCurrentFocus("Focus after loop: ");
  }

  static Command 
  tel("test-event-loop", // command name
      CommandEffector::functionEffectorOptionLess(testELoopCommand), // action
      "file",  // group name
      NULL, // arguments
      NULL, // options
      QT_TR_NOOP("Test event loop"),
      QT_TR_NOOP("Test event loop"),
      QT_TR_NOOP("Exits QSoas, losing all the current session"));

  //////////////////////////////////////////////////////////////////////
  
  static ArgumentList 
  dummyArgs(QList<Argument *>() 
            << new SeveralFilesArgument("file", 
                                        QT_TR_NOOP("File"),
                                        QT_TR_NOOP("Files to load !"),
                                        true));


  static ArgumentList 
  dummyOptions(QList<Argument *>() 
               << new FileArgument("file", 
                                   QT_TR_NOOP("File"),
                                   QT_TR_NOOP("Files to load !"))
               << new StringArgument("fiddle",
                                     QT_TR_NOOP("File"),
                                     QT_TR_NOOP("Files to load !"))
               << new StringArgument("stuff",
                                     QT_TR_NOOP("File"),
                                     QT_TR_NOOP("Files to load !"))
               );
                             

  static void dummyCommand(const QString & name, QStringList args, 
                           const CommandOptions & opts)
  {
    for(int i = 0; i < args.size(); i++)
      Terminal::out << "Arg #" << i << ": '" << args[i] << "'" << endl;
    for(CommandOptions::const_iterator i = opts.begin();
        i != opts.end(); i++)
      Terminal::out << "Option: " << i.key() << ": '" 
                    << i.value()->value<QString>() << "'" << endl;
  
  }


  static Command 
  dummy("dummy", // command name
        CommandEffector::functionEffector(dummyCommand), // action
        "file",  // group name
        &dummyArgs, // arguments
        &dummyOptions, // options
        QT_TR_NOOP("Dummy"),
        QT_TR_NOOP("Dummy test command"),
        QT_TR_NOOP("Dummy command for testing purposes"));

}

