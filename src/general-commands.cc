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
#include <datastack.hh>

#include <curveitems.hh>
#include <curvemarker.hh>
#include <curveview.hh>
#include <soas.hh>

#include <utils.hh>

#include <debug.hh>
#include <commandwidget.hh>
#include <mainwin.hh>

#include <file.hh>
#include <outfile.hh>

#include <idioms.hh>
#include <helpbrowser.hh>

#include <linereader.hh>

#include <argument-templates.hh>


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

      OutFile::out.mkPath = false;
      updateFromOptions(opts, "mkpath", OutFile::out.mkPath);
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
                   // << new BoolArgument("mkpath", 
                   //                     "Make path",
                   //                     "if on, creates all the directories necessary for creating the file (default: false)")
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

static Command 
outpt2("output", // command name
       effector(outputCommand), // action
       "file",  // group name
       NULL, // arguments
       &oOpts, // options
       "Change output file",
       "Change the name of the current output file", "",
       CommandContext::fitContext());

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


static void memCommand(const QString &,
                       const CommandOptions & opts)
{
  int kb = Utils::memoryUsed();
  MRuby * mr = MRuby::ruby();
  Terminal::out << "Memory used: " << kb << " kB\n"
                << "Ruby memory used: " << mr->memoryUse() << endl;

  Terminal::out << "Stack: " << soas().stack().textSummary() << endl;

  long ut, kt;
  Utils::processorUsed(&ut, &kt);
  Terminal::out << "Total time used: " << (ut+kt)*0.001 << endl;

  int fls, dss, size, maxf;
  DataBackend::cacheStats(&fls, &dss, &size, &maxf);
  Terminal::out << "Cache: " << fls << " files (out of "
                << maxf << "), " << dss
                << " buffers, for a total size of "
                << (size >> 10) << " kB" << endl;
  int ncs = -1;
  updateFromOptions(opts, "cached-files", ncs);
  if(ncs >= 0) {
    Terminal::out << "Setting new cache size to " << ncs << endl;
    DataBackend::setCacheSize(ncs);
  }
}

static ArgumentList 
memOpts(QList<Argument *>()
        << new IntegerArgument("cached-files", "Number of cached files")
        );

static Command 
m("mem", // command name
  effector(memCommand), // action
  "file",  // group name
  NULL, // arguments
  &memOpts, // options
  "Memory",
  "Informations about QSoas memory usage"
  );



//////////////////////////////////////////////////////////////////////

static ArgumentList 
pops(QList<Argument *>()
     << File::fileOptions(File::OverwriteOption/*|File::MkPathOption*/)
     << new FileSaveArgument("file", 
                             "Save as file",
                             "Save as file", "soas.pdf", false, true)
     << new StringArgument("title", 
                           "Page title",
                           "Sets the title of the page as printed")
     << new StringArgument("page-size", 
                           "Page size",
                           "Sets the page size, like 9x6 for 9cm by 6cm")
     << new IntegerArgument("nominal-height", 
                            "Nominal height",
                            "Correspondance of the height of the page in terms of points")
     );


#include <printpreviewhelper.hh>
#include <graphicoutput.hh>

static void printCommand(const QString &, 
                         const CommandOptions & opts)
{
  std::unique_ptr<QPaintDevice> p;
  int height = 500;
  QRect rect;

  QString file;
  bool test = false;

  QString title;
  updateFromOptions(opts, "title", title);

  updateFromOptions(opts, "file", file);
  updateFromOptions(opts, "nominal-height", height);

  QString pageSize;
  updateFromOptions(opts, "page-size", pageSize);

  if(! file.isEmpty() && test) {
    // Handle output separately for now...
    GraphicOutput out(title);
    out.setOutputFile(file);
    if(! pageSize.isEmpty())
      out.setOutputSize(pageSize);
    out.shipOut(&soas().view());
    return;
  }


  double w, h;

  bool preview = true;
  updateFromOptions(opts, "preview", preview);

  if(! file.isEmpty()) {
    File::checkOpen(file, opts);
    if(file.endsWith("svg")) {
      QSvgGenerator * gen = new QSvgGenerator;
      p = std::unique_ptr<QPaintDevice>(gen);
      if(pageSize.isEmpty()) {
        w = 1000;
        h = 1000;
      }
      rect = QRect(0, 0, w, h);
      gen->setSize(QSize(w,h));
      gen->setFileName(file);
      preview = false;
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
    if(pageSize.isEmpty()) {
      printer->setOrientation(QPrinter::Landscape);
    }
    else {
      printer->setPaperSize(QSizeF(w*10, h*10), QPrinter::Millimeter);
      printer->setFullPage(true);
    }
    rect = printer->pageRect();
  }

  PrintPreviewHelper helper(height, rect, title);
  if(/*preview*/ false) {
    QPrinter * printer = dynamic_cast<QPrinter *>(p.get());
    if(! printer)
      throw InternalError("Should not preview when generating SVG");
    QPrintPreviewDialog dlg(printer);
    helper.connect(&dlg, SIGNAL(paintRequested(QPrinter *)),
                   SLOT(paintOnPrinter(QPrinter *)));
  }
  else
    helper.paint(p.get());
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
  


static void saveTerminalCommand(const QString &, QString out,
                                const CommandOptions & opts)
{
  File file(out, File::TextWrite,
            opts);
  file.ioDevice()->write(soas().prompt().terminalContents().toLocal8Bit());
}

static ArgumentList 
sta(QList<Argument *>() 
    << new FileSaveArgument("file", 
                            "File",
                            "Output file",
                            "soas-output.txt"));

static ArgumentList 
sto(QList<Argument *>() 
    << File::fileOptions(File::OverwriteOption));

static Command 
st("save-output", // command name
   effector(saveTerminalCommand), // action
   "file",  // group name
   &sta, // arguments
   &sto, // options
   "Save output",
   "Save all output from the terminal");

//////////////////////////////////////////////////////////////////////
  
static ArgumentList 
saveHistoryArgs(QList<Argument *>() 
                << new FileSaveArgument("file", 
                                        "File",
                                        "Output file",
                                        "soas.cmds"));


static void saveHistoryCommand(const QString &, QString out,
                               const CommandOptions & opts)
{
  File file(out, File::TextWrite,
            opts);
  QStringList history = soas().prompt().history();
  QTextStream s(file);
  for(int i = history.size() - 1 ; i > 0; i--)
    s << history[i] << "\n";
}

static Command 
sh("save-history", // command name
   effector(saveHistoryCommand), // action
   "file",  // group name
   &saveHistoryArgs, // arguments
   &sto, // options, the same as save-terminal
   "Save history",
   "Save command history");

static Command 
sH("save-history", // command name
   effector(saveHistoryCommand), // action
   "file",  // group name
   &saveHistoryArgs, // arguments
   &sto, // options
   "Save history",
   "Save command history", "", CommandContext::fitContext());

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
  CommandWidget::ScriptErrorMode mode = CommandWidget::Abort;
  updateFromOptions(opts, "error", mode);

  QString condition;
  updateFromOptions(opts, "only-if", condition);

  if(!condition.isEmpty()) {
    MRuby * mr = MRuby::ruby();
    DataSet * ds = soas().stack().currentDataSet(true);
    bool cond = true;

    // Script name/arguments
    mrb_value ary = mr->newArray();
    for(const QString & s : args)
      mr->arrayPush(ary, mr->fromQString(s));

    SaveGlobal sg("$args");
    mr->setGlobal("$args", ary);

    if(ds)
      cond = ds->matches(condition);
    else {
      mrb_value v = mr->eval(condition);
      cond = mrb_test(v);
    }
    if(! cond) {
      Terminal::out << "Not running " << args.first()
                    << " since condition '" << condition
                    << "' is not verified" << endl;
      return;
    }
  }
  
  WDisableUpdates eff(& soas().view(), silent);

  QString cmdfile = args.takeFirst();

  QString nd;
  if(cd) {
    QFileInfo info(cmdfile);
    nd = info.path();
    cmdfile = info.fileName();
  }
  TemporarilyChangeDirectory c(nd);
  soas().prompt().runCommandFile(cmdfile, args, addToHistory, mode);
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
        << new TemplateChoiceArgument<CommandWidget::ScriptErrorMode>(CommandWidget::errorModeNames(),
                                                                      "error", 
                                                                      "on error",
                                                                      "Behaviour to adopt on error")
        );



static ArgumentList
rcO(ArgumentList()
    << runOpts
    << new BoolArgument("cd-to-script", 
                        "cd to script",
                        "If on, automatically change the directory to that oof the script")
    << new CodeArgument("only-if", 
                        "Condition",
                        "If specified, the script is only run when the condition is true")
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
  

static void noopCommand(const QString &, 
                        const CommandOptions &/*opts*/)
{
}

// static ArgumentList 
// noopArgs(QList<Argument *>() 
//          << new SeveralStringsArgument("ignored", 
//                                        "Ignored arguments",
//                                        "Ignored arguments", true));

static ArgumentList 
noopOpts(QList<Argument *>()
         << new SeveralStringsArgument("*", "Ignored options",
                                       "Ignored options",
                                       true, true));


static Command 
noop("noop", // command name
     effector(noopCommand), // action
     "file",  // group name
     NULL, // arguments
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
                                  QList<const DataSet*> datasets, 
                                  const CommandOptions & opts)
{
  // First, copy

  bool addToHistory = false;
  updateFromOptions(opts, "add-to-history", addToHistory);
  bool silent = false;
  updateFromOptions(opts, "silent", silent);

  CommandWidget::ScriptErrorMode mode = CommandWidget::Abort;
  updateFromOptions(opts, "error", mode);

  
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
    const DataSet * ds = datasets.takeLast();
    Terminal::out << "Running '" << script << "' using dataset: '"
                  << ds->name
                  << QString("', %1 cols, %2 rows, %3 segments").
      arg(ds->nbColumns()).arg(ds->nbRows()).
      arg(ds->segments.size() + 1)
                  << endl;

  
    soas().pushDataSet(new DataSet(*ds));
    soas().prompt().runCommandFile(script, a, addToHistory, mode);
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
rfdOpts(ArgumentList()
        << runOpts
        << new FileArgument("arg1", 
                            "First argument",
                            "First argument to the script")
        << argList);

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
  CommandWidget::ScriptErrorMode mode = CommandWidget::Abort;
  updateFromOptions(opts, "error", mode);
  
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
    soas().prompt().runCommandFile(script, a, addToHistory, mode);
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
rfeOpts(ArgumentList()
        << argList
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

static Command 
rfef("run-for-each", // command name
     effector(runForEachCommand), // action
     "file",  // group name
     &rfeArgs, // arguments
     &rfeOpts, 
     "Runs a script for several arguments",
     "Runs a script file repetitively with the given arguments",
     "", CommandContext::fitContext());

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

void timerCommand(const QString &, const CommandOptions & opts)
{
  static QDateTime time;
  static long kTime, uTime;
  QString name;
  updateFromOptions(opts, "name", name);
  if(time.isValid()) {
    qint64 dt = time.msecsTo(QDateTime::currentDateTime());
    time = QDateTime();

    long ut, kt;
    Utils::processorUsed(&ut, &kt);

    

    QString message;
    if(! name.isEmpty())
      message = name + ": %1 tot, %2 proc, %3 us, %4 sys";
    else
      message = "%1 seconds elapsed since timer start, (%2 total processor time, %3 user, %4 system)";
    message = message.
      arg(dt*0.001).arg((ut-uTime + kt - kTime)*0.001).
      arg((ut-uTime)*0.001).
      arg((kt - kTime)*0.001);
    Terminal::out << message << endl;
    Debug::debug() << message << endl;
  }
  else {
    time = QDateTime::currentDateTime();
    Utils::processorUsed(&uTime, &kTime);
    Terminal::out << "Starting timer " << name <<  endl;
    if(name.isEmpty())
      Debug::debug() << "Starting timer" << endl;
  }
}

static ArgumentList 
tmO(QList<Argument *>() 
    << new StringArgument("name",
                          "Name",
                          "name for the timer")
    );


static Command 
tm("timer", // command name
   effector(timerCommand), // action
   "file",  // group name
   NULL, // arguments
   &tmO, // options
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

static Command 
fsy("system", // command name
    effector(systemCommand), // action
    "file",  // group name
    &syA, // arguments
    &syO, // options
    "System",
    "Execute system commands", "", CommandContext::fitContext());


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
  bool dump = false;
  updateFromOptions(opts, "dump-sysinfo", dump);

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
  if(dump) {
    // Writes information to standard output
    QTextStream o(stdout);
    o  << Soas::versionString()
       << "\n * application dir: "
       << QCoreApplication::applicationDirPath()
       << "\n * application path: "
       << QCoreApplication::applicationFilePath()
       << "\n * library paths: ";
    for(const QString & p: QCoreApplication::libraryPaths())
      o << "\n    - " << p;
    o << "\n * documentation file: " << HelpBrowser::collectionFile()
      << endl;
    Terminal::out << " * application dir: "
                  << QCoreApplication::applicationDirPath()
                  << "\n * application path: "
                  << QCoreApplication::applicationFilePath()
                  << "\n * library paths: ";
    for(const QString & p: QCoreApplication::libraryPaths())
      Terminal::out << "\n    - " << p;
    Terminal::out << "\n * documentation file: "
                  << HelpBrowser::collectionFile()
                  << endl;
  }
}


static ArgumentList 
verO(QList<Argument *>() 
     << new BoolArgument("show-features", 
                         "Show features",
                         "If true, show detailed informations about the capacities of QSoas (defaults to false)")
     << new BoolArgument("dump-sysinfo", 
                         "Dumps sysinfo",
                         "If true, writes system specific information to standard output")
     );

static Command 
ver("version", // command name
    effector(versionCommand), // action
    "file",  // group name
    NULL, // arguments
    &verO, // options
    "Version",
    "Show version number");



//////////////////////////////////////////////////////////////////////

void headCommand(const QString &, QString file,
                 const CommandOptions & opts)
{
  File f(file, File::TextRead);

  int number = 10;
  updateFromOptions(opts, "number", number);
  int skipped = 0;
  updateFromOptions(opts, "skip", skipped);

  Terminal::out << "Reading file: " << file << endl;
  LineReader s(f);
  while(! s.atEnd()) {
    QString l = s.readLine(true);
    if(skipped > 0)
      --skipped;
    else {
      number--;
      Terminal::out << l;
      if(number == 0)
        break;
    }
  }
  Terminal::out << flush;

}

static ArgumentList 
headA(QList<Argument *>() 
      << new FileArgument("file", 
                          "File",
                          "name of the file to show")
     );


static ArgumentList 
headO(QList<Argument *>() 
      << new IntegerArgument("number", 
                             "Number of lines",
                             "number of lines to show")
      << new IntegerArgument("skip", 
                             "Skipped lines",
                             "number of lines to skip")
      );

static Command 
head("head", // command name
     effector(headCommand), // action
     "file",  // group name
     &headA, // arguments
     &headO, // options
     "Head");



//////////////////////////////////////////////////////////////////////

void pauseCommand(const QString &,
                 const CommandOptions & opts)
{
  double time = -1;
  QString text = "Click OK to continue";
  updateFromOptions(opts, "time", time);
  updateFromOptions(opts, "message", text);
  QDateTime d = QDateTime::currentDateTime();
  QMessageBox mb(QMessageBox::Information, "Pause", text);
  mb.setModal(false);
  
  if(time < 0) {
    mb.show();
    mb.raise();
    /*mb.activateWindow();*/
  }
  else {
    Terminal::out << "Waiting for " << time << " seconds" << endl;
    time *= 1000;
  }
  while(true) {
    if(time >= 0) {
      QDateTime d2 = QDateTime::currentDateTime();
      if(d.msecsTo(d2) >= time) {
        break;
      }
    }
    else {
      if(! mb.isVisible())
        break;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  }
  
}

static ArgumentList 
pauseO(QList<Argument *>() 
       << new NumberArgument("time", 
                             "Pause time",
                             "time to pause for, in seconds")
       << new StringArgument("message", 
                             "Text message",
                             "the message to display", true)
       );

static Command 
pause("pause", // command name
      effector(pauseCommand), // action
      "file",  // group name
      ArgumentList(), // arguments
      pauseO, // options
     "Pause");


