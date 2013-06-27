/**
   \file general-commands.cc various general purpose commands and groups
   Copyright 2011,2012,2013 by Vincent Fourmond

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
#include <exceptions.hh>

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
#include <mainwin.hh>

#include <outfile.hh>


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


static void breakCommand(const QString & )
{
  throw ControlFlowException("break");
}

static Command 
breakc("break", // command name
       optionLessEffector(breakCommand), // action
       "file",  // group name
       NULL, // arguments
       NULL, // options
       "Break",
       "Break from script",
       "Interrupts the run of the current script");


//////////////////////////////////////////////////////////////////////

static void temperatureCommand(const QString &,
                               const CommandOptions & opts)
{
  Terminal::out << "Temperature is currently " 
                << soas().temperature() << " K" << endl;
  if(opts.contains("set")) {
    soas().setTemperature(opts["set"]->value<double>());
    Terminal::out << "Setting it to " 
                  << soas().temperature() << " K" << endl;
  }
}

static ArgumentList 
tempOpts(QList<Argument *>() 
         << new NumberArgument("set", 
                               "Sets the temperature",
                               "Sets the temperature", true));

static Command 
temperature("temperature", // command name
            effector(temperatureCommand), // action
            "file",  // group name
            NULL, // arguments
            &tempOpts, // options
            "Temperature",
            "Reads/sets temperature",
            "Shows or sets the temperature (using option)",
            "T"
            );

//////////////////////////////////////////////////////////////////////

static ArgumentList 
pops(QList<Argument *>() 
     << new FileSaveArgument("file", 
                             "Save as file",
                             "Save as file", "biniou.ps")
     << new StringArgument("title", 
                           "Page title",
                           "Sets the title of the page as printed")
     );

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
  QString title;
  updateFromOptions(opts, "title", title);

  QPainter painter;
  painter.begin(&p);
  soas().view().render(&painter, 500,
                       p.pageRect(), title);
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

static void testELoopCommand(CurveEventLoop & loop, const QString &)
{
  // Debug::dumpCurrentFocus("Focus before creation: ");
  // CurveEventLoop loop;
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
  

static void runCommand(const QString &, QStringList args, 
                       const CommandOptions &opts)
{
  if(testOption<QString>(opts, "silent", "yes")) {
    QTextStream o(stdout);
    o << "Running in silent mode" << endl;
    soas().view().disableUpdates();
  }
  QString cmdfile = args.takeFirst();

  soas().prompt().runCommandFile(cmdfile, args);
  if(testOption<QString>(opts, "silent", "yes"))
    soas().view().enableUpdates();
}

static ArgumentList
runOpts(QList<Argument *>() 
        << new ChoiceArgument(QStringList() << "yes" << "no",
                              "silent", 
                              "Silent processing",
                              "Whether or not display is updated "
                              "during the load"));

static ArgumentList 
runArgs(QList<Argument *>() 
        << new SeveralFilesArgument("file", 
                                    "Files",
                                    "First is the command files, "
                                    "following are arguments", true));


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
  

/// @todo Choose the number of arguments (ie one by one, two by two,
/// and so on)
static void runForEachCommand(const QString &, QString script,
                              QStringList args, 
                              const CommandOptions & opts)
{
  int nb = 1;

  QStringList additionalArgs;

  for(int i = 2; i <= 5; i++) {
    QString on = QString("arg%1").arg(i);
    if(opts.contains(on))
      additionalArgs << opts[on]->value<QString>();
    else
      break;
  }  
  while(args.size() > 0) {
    QStringList a;
    for(int i = 0; i < nb && args.size() > 0; i++)
      a << args.takeFirst();
    a << additionalArgs;
    soas().prompt().runCommandFile(script, a);
  }
}

static ArgumentList 
rfeArgs(QList<Argument *>() 
        << new FileArgument("script", 
                            "Script",
                            "The script file")
        << new SeveralFilesArgument("arguments", 
                                    "Arguments",
                                    "All the arguments for the script file "
                                    "to loop on", true));

static ArgumentList 
rfeOpts(QList<Argument *>() 
        << new FileArgument("arg2", 
                            "Second argument",
                            "Second argument to the script")
        << new FileArgument("arg3", 
                            "Third argument",
                            "Third argument to the script")
        << new FileArgument("arg4", 
                            "Fourth argument",
                            "Fourth argument to the script")
        << new FileArgument("arg5", 
                            "Fifth argument",
                            "Fifth argument to the script")
        );



static Command 
rfe("run-for-each", // command name
    effector(runForEachCommand), // action
    "file",  // group name
    &rfeArgs, // arguments
    &rfeOpts, 
    "Loop run a script",
    "Runs a script file repetitively with the given arguments",
    "...");

//////////////////////////////////////////////////////////////////////

// This contains the list of all directories so far.
static QStringList previousDirectories;

  
static ArgumentList 
cda(QList<Argument *>() 
    << new FileArgument("directory",  
                        "Directory",
                        "New directory", true));

static ArgumentList 
cdo(QList<Argument *>() 
    << new BoolArgument("from-home",  
                        "From home",
                        "If on, relative from the home directory")
    << new BoolArgument("from-script",  
                        "From script",
                        "If on, cd relative from the current script directory")
    );

static void cdCommand(const QString &, QString dir, 
                      const CommandOptions & opts)
{
  QRegExp back("^(-+)$");
  bool fromHome = false;
  updateFromOptions(opts, "from-home", fromHome);

  bool fromScript = false;
  updateFromOptions(opts, "from-script", fromScript);
  if(back.indexIn(dir) == 0) {
    // We must go back in the history list
    int nb = back.cap(1).size();
    if(nb > previousDirectories.size())
      throw RuntimeError("There are not %1 directories in the cd history").
        arg(nb);
    dir = previousDirectories[nb-1];
  }
  else if(fromHome) {
    dir = QDir::home().absoluteFilePath(dir);
  }
  else if(fromScript) {
    QString n = soas().prompt().scriptFileName();
    if(n.isEmpty())
      throw RuntimeError("Cannot cd relative to script dir outsite "
                         "of a script !");
    dir = QFileInfo(n).dir().path();
  }
  else
    dir = Utils::expandTilde(dir);

  QString prev = QDir::currentPath();

  if(! QDir::setCurrent(dir))
    throw RuntimeError("Could not cd to '%1'").
      arg(dir);
  previousDirectories.insert(0,prev);
  soas().mainWin().updateWindowName();


  // Change the output file, if it is opened already
  if(OutFile::out.isOpened()) {
    QFileInfo inf(OutFile::out.fileName());
    QString f = inf.fileName();
    // Relative to current dir.
    OutFile::out.setFileName(f);
    Terminal::out << "Closing the previous output file at: '" << 
      OutFile::out.filePath() << "'" << endl;
  }

  Terminal::out << "Current directory now is: " << QDir::currentPath() 
                << endl;
}

static Command 
cd("cd", // command name
   effector(cdCommand), // action
   "file",  // group name
   &cda, // arguments
   &cdo, // options
   "Change directory",
   "Change current directory",
   "Change current directory",
   "G");

//////////////////////////////////////////////////////////////////////

static void pwdCommand(const QString &)
{
  Terminal::out << "Current directory: " << QDir::currentPath()
                << endl;
}

static Command 
pwd("pwd", // command name
   optionLessEffector(pwdCommand), // action
   "file",  // group name
   NULL, // arguments
   NULL, // options
   "Working directory",
   "Print working directory");
  


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

//////////////////////////////////////////////////////////////////////

// Startup files commands

void startupFilesCommand(const QString &, const CommandOptions & opts)
{
  // Without anything, just displays the commands
  bool run = false;
  updateFromOptions(opts, "run", run);
  if(run) {
    CommandWidget::runStartupFiles();
    return;
  }


  QString file;
  QStringList &sf = CommandWidget::startupFiles();
  updateFromOptions(opts, "add", file);
  if(! file.isEmpty()) {
    Terminal::out << "Adding file " << file << " to startup files" << endl;
    sf << file;
    return;
  }
  int del = -1;
  updateFromOptions(opts, "rm", del);
  if(del >= 0) {
    if(del < sf.size()) {
      Terminal::out << "Removing file numbered #" << del << endl;
      sf.removeAt(del);
    }
    else {
      Terminal::out << "No startup file numbered #" << del << endl;
    }
    return;
  }
  
  Terminal::out << "Displaying the list of current startup files: " 
                << endl;
  for(int i = 0; i < sf.size(); i++)
    Terminal::out << " * [" << i << "]: " << sf[i] << endl;
}

static ArgumentList 
sfO(QList<Argument *>() 
    << new FileArgument("add", 
                        "Startup file",
                        "Adds the given startup file", false, 
                        true)
    << new IntegerArgument("rm",
                           "Remove",
                           "Removes the numbered file")
    << new BoolArgument("run",
                        "Run",
                        "If on, runs all the startup files right now")
    );

static Command 
sf("startup-files", // command name
   effector(startupFilesCommand), // action
   "file",  // group name
   NULL, // arguments
   &sfO, // options
   "Startup files",
   "Handle startup files");


