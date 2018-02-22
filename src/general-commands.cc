/**
   \file general-commands.cc various general purpose commands and groups
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014, 2016 by CNRS/AMU

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
#include <commandcontext.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <terminal.hh>
#include <exceptions.hh>
#include <curve-effectors.hh>

#include <vector.hh>
#include <dataset.hh>
#include <databackend.hh>

#include <curveitems.hh>
#include <curvemarker.hh>
#include <curveview.hh>
#include <soas.hh>

#include <utils.hh>

#include <debug.hh>
#include <commandwidget.hh>
#include <mainwin.hh>

#include <outfile.hh>

#include <idioms.hh>


static Group file("file", 0,
                  "File",
                  "General purpose commands");

static Group load("load", 0,
                  "Load...",
                  "Loading data", &file);

static Group save("save", 0,
                  "Save...",
                  "Save data", &file);

static void quitCommand(const QString & name)
{
  if(name != "quit") {
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
       "Break from script");

//////////////////////////////////////////////////////////////////////


// Changes the output file to the named one.
static void outputCommand(const QString &, 
                          const CommandOptions & opts)
{
  if(opts.isEmpty()) {
    Terminal::out << "Current output file: '" << OutFile::out.filePath()
                  << "', " << (OutFile::out.isOpened() ? "currently open" : "not open at the moment") << endl;
  }
  else {
    QString file;
    updateFromOptions(opts, "file", file);
    if(! file.isEmpty()) {
      OutFile::out.setFileName(file);

      OutFile::out.truncate = false;
      updateFromOptions(opts, "overwrite", OutFile::out.truncate);
    }

    bool reopen = false;
    updateFromOptions(opts, "reopen", reopen);
    if(reopen)
      OutFile::out.forceReopen();
  
    // Now, we process meta-data
    QStringList metaNames;
    if(opts.contains("meta")) {
      updateFromOptions(opts, "meta", OutFile::out.desiredMeta);
      if(OutFile::out.desiredMeta.size() == 1 && OutFile::out.desiredMeta[0].isEmpty())
        OutFile::out.desiredMeta.clear();
      Terminal::out << "Now systematically writing meta-data to output file: '" 
                    << OutFile::out.desiredMeta.join("', '") << "'" << endl;
    }
  }
}

ArgumentList oOpts(QList<Argument*>() 
                   << new FileArgument("file", 
                                       "New output file",
                                       "name of the new output file", false, 
                                       true)
                   << new BoolArgument("overwrite", 
                                       "Overwrite",
                                       "if on, overwrites the file instead of appending (default: false)")
                   << new BoolArgument("reopen", 
                                       "Reopen",
                                       "if on, forces reopening the file (default: false)")
                   << new SeveralStringsArgument(QRegExp("\\s*,\\s*"), "meta", 
                                                 "Meta-data",
                                                 "when writing to output file, also prints the listed meta-data")
                   );


static Command 
output("output", // command name
       effector(outputCommand), // action
       "file",  // group name
       NULL, // arguments
       &oOpts, // options
       "Change output file",
       "Change the name of the current output file");

//////////////////////////////////////////////////////////////////////


// Changes the output file to the named one.
static void commentCommand(const QString &, QString cmt)
{
  OutFile::out.setHeader("");
  OutFile::out << cmt << "\n" << flush;
}

ArgumentList cArgs(QList<Argument*>() 
                   << new StringArgument("comment", 
                                         "Comment",
                                         "Comment line added to output file")
                   );


static Command 
comment("comment", // command name
       optionLessEffector(commentCommand), // action
       "file",  // group name
       &cArgs, // arguments
       NULL, // options
       "Write line to output",
       "Writes a comment line to the current output file");


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
            "T"
            );

//////////////////////////////////////////////////////////////////////

static ArgumentList 
pops(QList<Argument *>() 
     << new BoolArgument("overwrite", 
                         "Overwrite",
                         "Overwrite the output file")
     << new FileSaveArgument("file", 
                             "Save as file",
                             "Save as file", "soas.pdf", false, true)
     << new StringArgument("title", 
                           "Page title",
                           "Sets the title of the page as printed")
     );

static void printCommand(const QString &, 
                         const CommandOptions & opts)
{
  std::unique_ptr<QPaintDevice> p;
  int height = 500;
  QRect rect;

  QString file;
  bool overwrite = false;

  updateFromOptions(opts, "overwrite", overwrite);
  updateFromOptions(opts, "file", file);

  if(! file.isEmpty()) {
    if(! overwrite)
      Utils::confirmOverwrite(file);
    if(file.endsWith("svg")) {
      QSvgGenerator * gen = new QSvgGenerator;
      p = std::unique_ptr<QPaintDevice>(gen);
      rect = QRect(0, 0, 1000,1000);
      gen->setFileName(file);
    }
  }
  if(! p) {
    QPrinter * printer = new QPrinter;
    p = std::unique_ptr<QPaintDevice>(printer);
    if(! file.isEmpty())
      printer->setOutputFileName(file);
    else {
      QPrintDialog printDialog(printer);
      if(printDialog.exec() != QDialog::Accepted)
        return;
    }
    printer->setOrientation(QPrinter::Landscape);
    rect = printer->pageRect();
  }

  
  QString title;
  updateFromOptions(opts, "title", title);

  QPainter painter;
  painter.begin(p.get());
  soas().view().render(&painter, height, rect, title);
}

static Command 
p("print", // command name
  effector(printCommand), // action
  "file",  // group name
  NULL, // arguments
  &pops, // options
  "Print",
  "Print current view (almost)",
  "p"
  );

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
  for(int i = history.size() - 1 ; i > 0; i--)
    s << history[i] << "\n";
}

static Command 
sh("save-history", // command name
   optionLessEffector(saveHistoryCommand), // action
   "file",  // group name
   &saveHistoryArgs, // arguments
   NULL, // options
   "Save history",
   "Save command history");

//////////////////////////////////////////////////////////////////////
  

static void runCommand(const QString &, QStringList args, 
                       const CommandOptions &opts)
{
  bool addToHistory = false;
  updateFromOptions(opts, "add-to-history", addToHistory);
  bool silent = false;
  updateFromOptions(opts, "silent", silent);
  bool cd = false;
  updateFromOptions(opts, "cd-to-script", cd);
  
  WDisableUpdates eff(& soas().view(), silent);

  QString cmdfile = args.takeFirst();

  QString nd;
  if(cd) {
    QFileInfo info(cmdfile);
    nd = info.path();
    cmdfile = info.fileName();
  }
  TemporarilyChangeDirectory c(nd);
  soas().prompt().runCommandFile(cmdfile, args, addToHistory);
}

static ArgumentList
runOpts(QList<Argument *>() 
        << new BoolArgument("silent", 
                            "Silent processing",
                            "whether or not to switch off display updates "
                            "during the script (off by default)")
        << new BoolArgument("add-to-history", 
                            "Add commands to history",
                            "whether the commands run are added to the "
                            "history (defaults to false)")
        );

static ArgumentList
rcO(QList<Argument *>(runOpts) 
        << new BoolArgument("cd-to-script", 
                            "cd to script",
                            "If on, automatically change the directory to that oof the script")
        );

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
    &rcO, 
    "Run commands",
    "Run commands from a file",
    "@");

// The same, but for the fit context
static Command 
frun("run", // command name
     effector(runCommand), // action
     "file",  // group name
     &runArgs, // arguments
     &rcO, 
     "Run commands",
     "Run commands from a file",
     "@", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////
  

static void noopCommand(const QString &, QStringList /*args*/, 
                        const CommandOptions &/*opts*/)
{
}

static ArgumentList 
noopArgs(QList<Argument *>() 
         << new SeveralStringsArgument("ignored", 
                                       "Ignored arguments",
                                       "Ignored arguments", true));

static ArgumentList 
noopOpts(QList<Argument *>() 
         << new StringArgument("*", "Ignored options",
                               "Ignored options",
                               true));


static Command 
noop("noop", // command name
     effector(noopCommand), // action
     "file",  // group name
     &noopArgs, // arguments
     &noopOpts, // options
     "No op",
     "Does nothing");


//////////////////////////////////////////////////////////////////////
  

static ArgumentList 
argList(QList<Argument *>() 
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
        << new FileArgument("arg6",
                            "Sixth argument",
                            "Sixth argument to the script"));

static void runForDatasetsCommand(const QString &, QString script,
                                  QList<const DataSet*> dss, 
                                  const CommandOptions & opts)
{
  // First, copy

  QList<DataSet * > datasets;
  for(int i = 0; i < dss.size(); i++)
    datasets << new DataSet(*dss[i]);

  bool addToHistory = false;
  updateFromOptions(opts, "add-to-history", addToHistory);
  bool silent = false;
  updateFromOptions(opts, "silent", silent);
  WDisableUpdates eff(& soas().view(), silent);

  QStringList a;
  for(int i = 1; i <= argList.size()+1; i++) {
    QString on = QString("arg%1").arg(i);
    if(opts.contains(on))
      a << opts[on]->value<QString>();
    else
      break;
  }  

  while(datasets.size() > 0) {
    soas().pushDataSet(new DataSet(*datasets.takeLast()));
    soas().prompt().runCommandFile(script, a, addToHistory);
  }
}



static ArgumentList 
rfdArgs(QList<Argument *>()
        << new FileArgument("script", 
                            "Script",
                            "The script file")
        << new SeveralDataSetArgument("datasets", 
                                      "Arguments",
                                      "All the arguments for the script file "
                                      "to loop on", true));



static ArgumentList 
rfdOpts(QList<Argument *>(runOpts)
        << new FileArgument("arg1", 
                            "First argument",
                            "First argument to the script")
        << QList<Argument *>(argList));

static Command 
rfd("run-for-datasets", // command name
    effector(runForDatasetsCommand), // action
    "file",  // group name
    &rfdArgs, // arguments
    &rfdOpts, 
    "Runs a script for several datasets",
    "Runs a script file repetitively with the given buffers");

//////////////////////////////////////////////////////////////////////

/// @todo Choose the number of arguments (ie one by one, two by two,
/// and so on)
static void runForEachCommand(const QString &, QString script,
                              QStringList args, 
                              const CommandOptions & opts)
{
  bool addToHistory = false;
  updateFromOptions(opts, "add-to-history", addToHistory);

  bool silent = false;
  updateFromOptions(opts, "silent", silent);
  WDisableUpdates eff(& soas().view(), silent);

  int nb = 1;

  QStringList additionalArgs;

  for(int i = 2; i <= argList.size()+1; i++) {
    QString on = QString("arg%1").arg(i);
    if(opts.contains(on))
      additionalArgs << opts[on]->value<QString>();
    else
      break;
  }  

  QStringList oa = args;
  QString type;
  updateFromOptions(opts, "range-type", type);
  // If range-type is specified, we parse the arguments into number
  // ranges.
  if(! type.isEmpty()) {
    bool lg = (type == "log");
    args.clear();
    QRegExp rangeRE("^(.*)\\.\\.(.*)\\s*:\\s*(\\d+)$");
    for(int i = 0; i < oa.size(); i++) {
      const QString & a = oa[i];
      if(rangeRE.indexIn(a) == 0) {
        double s = rangeRE.cap(1).toDouble();
        double e = rangeRE.cap(2).toDouble();
        if(lg) {
          s = log(s);
          e = log(e);
        }
        Terminal::out << "s = " << s << "\te = " << e << endl;
        int nb = rangeRE.cap(3).toInt();
        for(int j = 0; j < nb; j++) {
          double x = s + ((e - s) * j)/(nb - 1);
          if(lg)
            x = exp(x);
          args << QString::number(x);
        }
      }
      else
        args << a;
    }
  }


  while(args.size() > 0) {
    QStringList a;
    for(int i = 0; i < nb && args.size() > 0; i++)
      a << args.takeFirst();
    a << additionalArgs;
    soas().prompt().runCommandFile(script, a, addToHistory);
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
rfeOpts(QList<Argument *>(argList)
        << new ChoiceArgument(QStringList() << "lin" << "log",
                              "range-type",
                              "Numerical range type",
                              "If on, transform arguments into ranged numbers")
        << runOpts
        );



static Command 
rfe("run-for-each", // command name
    effector(runForEachCommand), // action
    "file",  // group name
    &rfeArgs, // arguments
    &rfeOpts, 
    "Runs a script for several arguments",
    "Runs a script file repetitively with the given arguments");

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


  // DO NOT CHANGE THE OUTPUT FILE !
  
  // // Change the output file, if it is opened already
  // if(OutFile::out.isOpened()) {
  //   QFileInfo inf(OutFile::out.fileName());
  //   QString f = inf.fileName();
  //   // Relative to current dir.
  //   OutFile::out.setFileName(f);
  //   Terminal::out << "Closing the previous output file at: '" << 
  //     OutFile::out.filePath() << "'" << endl;
  // }

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
                        "adds the given startup file", false, 
                        true)
    << new IntegerArgument("rm",
                           "Remove",
                           "removes the numbered file")
    << new BoolArgument("run",
                        "Run",
                        "if on, runs all the startup files right now (off by default)")
    );

static Command 
sf("startup-files", // command name
   effector(startupFilesCommand), // action
   "file",  // group name
   NULL, // arguments
   &sfO, // options
   "Startup files",
   "Handle startup files");

//////////////////////////////////////////////////////////////////////

// A rudimentary timer

void timerCommand(const QString &, const CommandOptions & )
{
  static QDateTime time;
  if(time.isValid()) {
    qint64 dt = time.msecsTo(QDateTime::currentDateTime());
    time = QDateTime();
    Terminal::out << dt*0.001 << " seconds elapsed since timer start" << endl;
    Debug::debug() << dt*0.001 << " seconds elapsed since timer start" << endl;
  }
  else {
    time = QDateTime::currentDateTime();
    Terminal::out << "Starting timer" << endl;
    Debug::debug() << "Starting timer" << endl;
  }
}


static Command 
tm("timer", // command name
   effector(timerCommand), // action
   "file",  // group name
   NULL, // arguments
   NULL, // options
   "Timer",
   "Start/stop timer");

//////////////////////////////////////////////////////////////////////

// Shell execution !

void systemCommand(const QString &, QStringList args, const CommandOptions & opts)
{
  bool useShell =
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    true;
#else
  false;
#endif
  updateFromOptions(opts, "shell", useShell);

  int timeout = -1;
  updateFromOptions(opts, "timeout", timeout);
  

  QString prog = useShell ? "sh" : args.takeFirst();
  QStringList a;
  if(useShell)
    a << "-c" << args.join(" ");
  else
    a = args;

  QProcess prc;
  prc.setProcessChannelMode(QProcess::QProcess::MergedChannels);
  Terminal::out << "Running " << prog << " '" << a.join("' '") << "'" << endl;
  prc.start(prog, a, QIODevice::ReadOnly);
  prc.closeWriteChannel();
  bool killed = false;
  if(! prc.waitForFinished(timeout)) {
    prc.terminate();
    if(! prc.waitForFinished(timeout/10))
      prc.kill();
    killed = true;
  }
  Terminal::out << prc.readAll() << endl;
  if(killed)
    Terminal::out << "Process killed (timeout)" << endl;  
}

static ArgumentList 
syA(QList<Argument *>() 
    << new SeveralFilesArgument("command", 
                                "Command",
                                "Arguments of the command ", true, 
                                false)
    );

static ArgumentList 
syO(QList<Argument *>() 
    << new BoolArgument("shell", 
                        "Shell",
                        "use shell (on by default on Linux/Mac, off in windows)")
    << new IntegerArgument("timeout", 
                           "Timeout",
                           "timeout (in milliseconds)")
    );


static Command 
sy("system", // command name
   effector(systemCommand), // action
   "file",  // group name
   &syA, // arguments
   &syO, // options
   "System",
   "Execute system commands");


//////////////////////////////////////////////////////////////////////

// A command to setup debug output

void debugCommand(const QString &, const CommandOptions & opts)
{
  if(opts.isEmpty()) {
    Terminal::out << "Closing all debug outputs" << endl;
    Debug::debug().closeDirectory();
    return;
  }
  QString str;
  updateFromOptions(opts, "directory", str);
  if(! str.isEmpty()) {
    Terminal::out << "Setting up debug output in " << str << endl;
    Debug::debug().openDirectory(str);
  }
  if(opts.contains("level")) {
    int lvl;
    updateFromOptions(opts, "level", lvl);
    Debug::setDebugLevel(lvl);
  }
}


static ArgumentList 
dbgO(QList<Argument *>() 
     << new FileArgument("directory",
                         "Debug directory",
                         "Directory in which the debug output is saved",
                         true, true)
     << new IntegerArgument("level",
                            "Debug level",
                            "Sets the debug level")
    );


static Command 
dbg("debug", // command name
    effector(debugCommand), // action
    "file",  // group name
    NULL, // arguments
    &dbgO, // options
    "Debug",
    "Setup debug output");

//////////////////////////////////////////////////////////////////////

// A version command

static QString join(const QVariant& lst, const QString & jn = ", ")
{
  QList<QVariant> v = lst.toList();
  QList<QString> l;
  for(QVariant s : v)
    l << s.toString();
  return Utils::joinSortedList(l, jn);
}

void versionCommand(const QString &, const CommandOptions & opts)
{
  Terminal::out << Soas::versionString() << endl;
  Debug::debug() << Soas::versionString() << endl;

  bool specs = false;
  updateFromOptions(opts, "show-features", specs);
  /// Return somewhere the specs as a hash ? A Ruby-available hash, heh ?
  if(specs) {
    ValueHash info = Soas::versionInfo();
    Terminal::out
      << "Capacities built in your version of QSoas:\n\n"
      << "Time-dependent parameters: "
      << join(info["time-dependent-parameters"])
      << endl
      << "Integrators for ODEs: "
      << join(info["ode-steppers"])
      << endl
      << "Plain integrators: "
      << join(info["integrators"])
      << endl
      << "Vector integrators: "
      << join(info["multi-integrators"])
      << endl
      << "Fit engines: "
      << join(info["fit-engines"])
      << endl
      << "Statistics: "
      << join(info["statistics"])
      << endl
      << "Many-buffers-display styles: "
      << join(info["color-styles"])
      << endl
      << "Distributions for fit parameters: "
      << join(info["parameter-distributions"])
      << endl
      << "Special functions: "
      << join(info["functions"])
      << endl
      << "Constants: "
      << join(info["constants"])
      << endl;
  }
}


static ArgumentList 
verO(QList<Argument *>() 
     << new BoolArgument("show-features", 
                         "Show features",
                         "If true, show detailed informations about the capacities of QSoas (defaults to false)")
     );

static Command 
ver("version", // command name
    effector(versionCommand), // action
    "file",  // group name
    NULL, // arguments
    &verO, // options
    "Version",
    "Show version number");

