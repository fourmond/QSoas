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
#include <file-arguments.hh>
#include <terminal.hh>

#include <vector.hh>
#include <dataset.hh>
#include <databackend.hh>

#include <curveeventloop.hh>
#include <curveitems.hh>
#include <curvemarker.hh>
#include <curveview.hh>
#include <soas.hh>

#include <utils.hh>

#include <debug.hh>
#include <commandwidget.hh>


static Group file("file", 0,
                  "File",
                  "General purpose commands");


static void quitCommand(const QString & name)
{
  if( name != "quit") {
    if(! Utils::askConfirmation(QObject::tr("Are you sure you "
                                            "want to quit ?"))) {
      Terminal::out << "Great !" << endl;
      return;
    }
  }
  qApp->quit();
}

static Command 
quit("quit", // command name
     optionLessEffector(quitCommand), // action
     "file",  // group name
     NULL, // arguments
     NULL, // options
     "Quit",
     "Quit QSoas",
     "Exits QSoas, losing all the current session",
     "q");

//////////////////////////////////////////////////////////////////////

static void antialiasCommand(const QString &)
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
   optionLessEffector(antialiasCommand), // action
   "file",  // group name
   NULL, // arguments
   NULL, // options
   "Antialias",
   "Toggle anti-aliasing",
   "Toggles anti-aliasing for graphics rendering.",
   "AA");

//////////////////////////////////////////////////////////////////////

static void openglCommand(const QString &)
{
  soas().setOpenGL(! soas().openGL());
  Terminal::out << "The use of OpenGL is now " 
                << (soas().openGL() ? "on" : "off") 
                << endl;
}

static Command 
ogl("opengl", // command name
    optionLessEffector(openglCommand), // action
    "file",  // group name
    NULL, // arguments
    NULL, // options
    "OpenGL",
    "Toggle OpenGL",
    "Toggles the use of OpenGL for graphics rendering."
    );

//////////////////////////////////////////////////////////////////////

static ArgumentList 
pops(QList<Argument *>() 
     << new FileSaveArgument("file", 
                             "Save as file",
                             "Save as file", "biniou.ps"));

static void printCommand(const QString &, 
                         const CommandOptions & opts)
{
  QPrinter p;
  p.setOrientation(QPrinter::Landscape);
  if(opts.contains("file"))
    p.setOutputFileName(opts["file"]->value<QString>());
  else {
    QPrintDialog printDialog(&p);
    if(printDialog.exec() != QDialog::Accepted)
      return;
  }
  QPainter painter;
  painter.begin(&p);
  soas().view().render(&painter, 500,
                       p.pageRect());
}

static Command 
p("print", // command name
  effector(printCommand), // action
  "file",  // group name
  NULL, // arguments
  &pops, // options
  "Print",
  "Print current view (almost)",
  "Prints the current main panel of the current view",
  "p"
  );

//////////////////////////////////////////////////////////////////////

static void testELoopCommand(const QString &)
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
    // Debug::dumpCurrentFocus("Current focus: ");
    // o << "Event: " << loop.type()
    //   << ", key " << QString("0x%1").arg(loop.key(), 8, 16, 
    //                                      QChar('0')) << endl;
    if(loop.key() == Qt::Key_Escape)
      return;

    if(loop.type() == QEvent::MouseButtonPress) {
      QPointF p = loop.position();
      Terminal::out << "Focus window: " << qApp->focusWidget() << endl;
      Terminal::out << "Press event at " << p.x() << "," << p.y() 
                    << " -- " << loop.button() << " #" << i << endl;
      
      if(i % 2)
        l.p2 = p;
      else
        l.p1 = p;
      m.p = p;
      i++;
      if(loop.button() == Qt::MiddleButton)
        return;                 // As a escape route in mac
      if(i >= 10)
        return;                 // As a escape route in mac (too)
    }
    else if(loop.type() == QEvent::KeyPress) {
      Terminal::out << "Key press: " << loop.key() 
                    << " -- " << QChar(loop.key()) << endl;
      if(loop.key() == 's') {
        Terminal::out << "Prompting: " << endl;
        QString str = loop.promptForString("what ?");
        Terminal::out << "Got string: " << str << endl;
      }
    }
  }
  Debug::dumpCurrentFocus("Focus after loop: ");
}

static Command 
tel("test-event-loop", // command name
    optionLessEffector(testELoopCommand), // action
    "file",  // group name
    NULL, // arguments
    NULL, // options
    "Test event loop",
    "Test event loop",
    "Exits QSoas, losing all the current session");

//////////////////////////////////////////////////////////////////////
  
static ArgumentList 
sta(QList<Argument *>() 
    << new FileSaveArgument("file", 
                            "File",
                            "Output file",
                            "soas-output.txt"));


static void saveTerminalCommand(const QString &, QString out)
{
  QFile o(out);
  Utils::open(&o, QIODevice::WriteOnly);
  o.write(soas().prompt().terminalContents().toLocal8Bit());
  o.close();
}

static Command 
st("save-output", // command name
   optionLessEffector(saveTerminalCommand), // action
   "file",  // group name
   &sta, // arguments
   NULL, // options
   "Save output",
   "Save all output from the terminal",
   "Save all output from the terminal");

//////////////////////////////////////////////////////////////////////
  
static ArgumentList 
saveHistoryArgs(QList<Argument *>() 
                << new FileSaveArgument("file", 
                                        "File",
                                        "Output file",
                                        "soas.cmds"));


static void saveHistoryCommand(const QString &, QString out)
{
  QFile o(out);
  Utils::open(&o, QIODevice::WriteOnly);

  QStringList history = soas().prompt().history();
  QTextStream s(&o);
  for(int i = history.size() - 1 ; i >= 0; i--)
    s << history[i] << "\n";
}

static Command 
sh("save-history", // command name
   optionLessEffector(saveHistoryCommand), // action
   "file",  // group name
   &saveHistoryArgs, // arguments
   NULL, // options
   "Save history",
   "Save command history",
   "Saves all the command history to a plain text file, "
   "to be used again with run");

//////////////////////////////////////////////////////////////////////
  
static ArgumentList 
runArgs(QList<Argument *>() 
        << new FileArgument("file", 
                            "Command file",
                            "File containing the commands to run"));


static void runCommand(const QString &, QString cmdfile, 
                       const CommandOptions &opts)
{
  if(testOption<QString>(opts, "silent", "yes")) {
    QTextStream o(stdout);
    o << "Running in silent mode" << endl;
    soas().view().disableUpdates();
  }

  soas().prompt().runCommandFile(cmdfile);
  soas().view().enableUpdates();
}

static ArgumentList
runOpts(QList<Argument *>() 
        << new ChoiceArgument(QStringList() << "yes" << "no",
                              "silent", 
                              "Silent processing",
                              "Whether or not display is updated "
                              "during the load"));


static Command 
run("run", // command name
   effector(runCommand), // action
   "file",  // group name
   &runArgs, // arguments
    NULL, //&runOpts, // options doesn't seem to work anyway...
   "Run commands",
   "Run commands from a file",
   "Run commands saved in a file",
   "@");

//////////////////////////////////////////////////////////////////////
  
static ArgumentList 
cda(QList<Argument *>() 
    << new FileArgument("directory",  /// @todo use 
                        "Directory",
                        "New directory"));

static void cdCommand(const QString &, QString dir)
{
  if(! QDir::setCurrent(dir))
    throw RuntimeError(QObject::tr("Could not cd to '%1'").
                       arg(dir));
}

static Command 
cd("cd", // command name
   optionLessEffector(cdCommand), // action
   "file",  // group name
   &cda, // arguments
   NULL, // options
   "Change directory",
   "Change current directory",
   "Change current directory"
   "G");
  


//////////////////////////////////////////////////////////////////////
  
static ArgumentList 
dummyArgs(QList<Argument *>() 
          << new FileSaveArgument("file", 
                                  "File",
                                  "Files to load !",
                                  "bidule.dat"));


static ArgumentList 
dummyOptions(QList<Argument *>() 
             << new FileArgument("file", 
                                 "File",
                                 "Files to load !")
             << new StringArgument("fiddle",
                                   "File",
                                   "Files to load !")
             << new StringArgument("stuff",
                                   "File",
                                   "Files to load !")
             );
                             

static void dummyCommand(const QString & , QString arg, 
                         const CommandOptions & opts)
{
  // for(int i = 0; i < args.size(); i++)
  //   Terminal::out << "Arg #" << i << ": '" << args[i] << "'" << endl;
  Terminal::out << "Arg is: " << arg << endl;
  for(CommandOptions::const_iterator i = opts.begin();
      i != opts.end(); i++)
    Terminal::out << "Option: " << i.key() << ": '" 
                  << i.value()->value<QString>() << "'" << endl;
  
}


static Command 
dummy("dummy", // command name
      effector(dummyCommand), // action
      "file",  // group name
      &dummyArgs, // arguments
      &dummyOptions, // options
      "Dummy",
      "Dummy test command",
      "Dummy command for testing purposes");
