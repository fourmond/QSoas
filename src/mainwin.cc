/*
  mainwin.cc: Main window for QSoas
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014, 2015 by CNRS/AMU

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

/**
   \mainpage Projects for QSoas

   \section requests Requests

   @li manually tweak/set the baseline for FFT

   \section various-todo Various things to do

   Now, mainly, what I need to do, is to massage the functionalities
   of the old Soas back into this one

   \li finalize peak detection (along with a CurvePeak CurveItem child
   to display it neatly)
   \li convolution, that was quite useful
   \li step detection, dead useful too !

   And other things too:

   \li improve distance-to-dataset
   \li dataset selection
   \li come up with a full dialog box to edit @b all arguments to a
   command. (including options !)
   \li definition of new fits based on ruby subclasses of Fit,
   PerDatasetFit and FunctionFit (much less useful now that we have
   custom fit loading)
   \li keyboard shortcuts for navigation in FitDialog: quick goto
   label editor, navigation between buffers, shortcuts for export/save
   and so on...
   \li an apply-formula command spanning several datasets (using x[0],
   x[1] and so on...)

   When comparing datasets, it is irksome to lose overlays by
   loading/manipulating datasets. Maybe what we need is either to
   @li make some overlays persistent (ie, they stay until an explicit
   call to clear ?). This may even be the default ?
   @li link datasets one to the other (and every time one dataset with
   links is displayed, all others are displayed too !
   
   While both approaches have their merit, 
   
   

   @b Qt-related @b stuff:

   @li maybe one day Utils::registerShortCut could be used to
   store/annotate the shortcuts available in a window ?

   @li I should come up with button-like widget containers that would
   help selection of datasets in DatasetBrowser, and use these
   selection for various operations...

   Find a way to prompt for additional arguments for greedy parameters
   (using an additional button in the dialog box ?)

   @section segments Segments

   There are a lot of operations that could benefit from potentially
   setting automatically segments, such as:

   @li concatenation
   @li unwrapping
   @li splitting into monotonic parts could instead set the segments
   @li more ideas ?


   \section fits Fit-related things

   \li options to automatically apply a bijection to the Y values too.
   \li detection of "weird" parameters, ie a parameter that suddenly
   takes INF or NAN values while it didn't take before. This is a
   clear sign of something going wrong.
   @li All parameters should be saved somewhere in memory in a
   session, in order to recover from a fit one quits too fast ;-)...
   @li Ideally, fit iterations should be done in a cancellable
   parallel thread to make sure one can abort a very-long computation.

   @b Fits @b dialogs:

   Some useability improvements:

   @li It should be possible to build a "stack" of FitWorkspace
   without interference between them, and choose at the right time.
   @li display the (scaled !) covariance matrix in terms of colors
   (white uncorrelated, red positively correlated and blue negatively
   ?)
   @li keep track of all the recent parameters along with their
   respective residuals, to allow for an easy fallback to the "last
   valid/reasonable parameters"
   @li on a similar note, it should be possible to "mark" the
   resulting parameters of a fit and store them on a stack to browse
   later (and possibly export all in one go ?), and come back to if
   necessary. The main purpose of that would be the comparison between
   different conditions...
   @li possibility of partially loading parameters from a file

   \section arch Architectural things

   \b Documentation

   Although the current way to do documentation isn't that bad, it
   doesn't scale well, and it really is a pain to maintain for
   now. What I need is 

   @li a simple plain text file (with an appropriate markup system --
   why not maruku or kramdown ? -- most probably kramdown) containing
   all the commands along with comment wrappers to position the
   "generated part" (ie name, arguments, options) and the "description
   part" (full description, in plain text).
   @li extra text could be inserted in this at no cost (outside of the
   designated areas), in order to turn that into a real useful
   documentation
   @li it should allow easy hyperlinking to other commands
   @li I should be able to somehow process the output of the program
   to get inline documentation (possibly included as qrc resource ?)

   @b Expressions and derivatives

   The expressions system could be rewritten to take advantage of the
   Ruby 1.9.1 ripper extension for the detection of parameters and/or
   for the making of derivatives.

   @b Differential @b systems
   
   As of now, there are two frameworks that are making use of (linear)
   differential systems: LinearKineticSystem and LinearWave. It would
   be very interesting to merge that functionality into Linear and
   Non-Linear systems... (which would allow simplification of the fits
   as well...)

   \b Ruby \b interface 

   All commands should be interfaced to Ruby. That would allow really
   powerful scripting, possibly requiring as little as possible user
   interaction. This requires a new function in Argument to convert
   from Ruby's VALUE (but that isn't much of a problem). Possibly a
   first conversion to string followed by conversion from string would
   do fine for most types.

   This would however rise the question of how to actually get data
   from Soas to Ruby ? Possibly an interesting point would be to allow
   access to the output file both as strings and as series of
   tab-separated data (properly converted using template
   specialization)

   \b Headless \b mode

   It would be very interesting to try to run Soas in headless mode
   and/or from a Ruby script, ie without an X connection starting.
   This however requires a careful rewrite of many places. Maybe it
   would be possible to use a child and/or superclass of CurveView
   that just would trash everything and abort on interactive requests
   ? Maybe the trick is simply to use xvfb and modify interactive
   commands so that they abort in non-interactive mode (that is
   CurveEventLoop, argument prompting and fit dialogs -- any dialog in
   fact -- maybe we need a subclass of QDialog that aborts on exec()
   in non-interactive mode ?).

   The bad thing about xvfb, though, is that this isn't portable at
   all to win/mac (which is very bad !).
   
*/

#include <headers.hh>
#include <mainwin.hh>
#include <group.hh>
#include <command.hh>

#include <commandcontext.hh>

#include <soas.hh>

#include <commandwidget.hh>
#include <terminal.hh>
#include <curveview.hh>
#include <datastack.hh>

#include <settings-templates.hh>

#include <build.hh>
#include <hook.hh>

#include <graphicssettings.hh>

#include <credits.hh>

#include <commandlineparser.hh>
#include <helpbrowser.hh>



static SettingsValue<QSize> mainWinSize("mainwin/size", QSize(700,500),
                                        "size of the main window");

static SettingsValue<QByteArray> splitterState("mainwin/splitter", 
                                               QByteArray(),
                                               "position of the main vertical splitter");

/// an array of commands provided on the command line
/// @todo not very object-oriented, but, well, for now it works
static QStringList cmdlineCommands;



static CommandLineOption cmd("--run", [](const QStringList & args) {
    cmdlineCommands << args.first();
  }, 1, "runs the command after QSoas has started up");

static bool exitAfterRunning = false;

static CommandLineOption ext("--exit-after-running", [](const QStringList &) {
    exitAfterRunning = true;
  }, 0, "exits QSoas after running the commands");

/// A script to be run along with its arguments
static QStringList runScript;

static CommandLineOption rs("--run-script",
                            [](const QStringList & args) {
                              runScript = args;
                              QRegExp re("(.*)\\.[^.]+$");
                              QString lg;
                              if(re.indexIn(args[0]) == 0)
                                lg = re.cap(1) + ".log";
                              if(lg.isEmpty() || lg == args[0])
                                lg = args[0] + ".log";
                              CommandWidget::logFileName = lg;
                              /// @todo Disable interactive commands when that becomes possible.
                              
                            }, -1, "runs a single script with possible arguments and exits");


MainWin::MainWin(Soas * theSoas, bool runStartupFiles)
{
  soasInstance = theSoas;
  theSoas->setMainWindow(this);
  setupFrame();
  resize(mainWinSize);
  if(! splitterState->isEmpty())
    mainSplitter->restoreState(splitterState);

  // We load the icon
  QIcon appIcon(":QSoas-logo.png");
  setWindowIcon(appIcon);

  Terminal::out << Soas::versionString()<< endl;
  Credits::displayStartupMessage();
  Terminal::out << "PID " << QCoreApplication::applicationPid()
                << " starting on " << soasInstance->startupTime().toString()
                << "\nCurrent directory is: " << QDir::currentPath() 
                << "\nCurrent temperature is: " << soasInstance->temperature() 
                << " K\n";
  soasInstance->graphicsSettings().initialSetup();
  
  Terminal::out << "To list available commands, type 'commands'\n"
                << "To get help on a specific command, type 'help command'\n"
                << endl;

  // Now, running all the startup files
  if(runStartupFiles)
    CommandWidget::runStartupFiles();
  else
    Terminal::out << "Not loading any startup file as requested" << endl;

  Hook::runHooks();

  for(int i = 0; i < cmdlineCommands.size(); i++)
    commandWidget->runCommand(cmdlineCommands[i]);
    
  if(runScript.size() > 0) {
    QStringList cmd = runScript;
    QFileInfo info(cmd[0]);
    if(QDir::setCurrent(info.dir().path())) {
      cmd[0] = info.fileName();
      cmd.insert(0, "@");
      commandWidget->runCommand(cmd);
    }
    else {
      Terminal::out << "Could not cd to script directory: '"
                    << info.dir().path() << "'" << endl;
    }
  }

  if((cmdlineCommands.size() > 0 && exitAfterRunning)
     || (runScript.size() > 0)) {
    connect(this, SIGNAL(windowReady()),
            qApp,SLOT(quit()), Qt::QueuedConnection);
  }
  if(cmdlineCommands.size() == 0 && runScript.size() == 0)
    connect(this, SIGNAL(windowReady()),
            this, SLOT(showStartupTips()), Qt::QueuedConnection);
  
  emit(windowReady());
}

void MainWin::showStartupTips()
{
  TipsDisplay::showStartupTips();
}
  

void MainWin::setupFrame()
{
  statusBar();
  menus = Group::fillMenuBar(menuBar(), CommandContext::globalContext());
  connect(menuBar(), SIGNAL(triggered(QAction *)),
          SLOT(menuActionTriggered(QAction *)));

  mainSplitter = new QSplitter(Qt::Vertical);
  // QVBoxLayout * layout = new QVBoxLayout(w);
  curveView = new CurveView;
  mainSplitter->addWidget(curveView);
  commandWidget = new CommandWidget;
  mainSplitter->addWidget(commandWidget);
  curveView->setFocusProxy(commandWidget);
  mainSplitter->setFocusProxy(commandWidget);

  
  mainSplitter->setStretchFactor(0, 3);
  mainSplitter->setStretchFactor(1, 1);


  // This is not so nice, but the splitters are invisible with many
  // themes.
  //
  // Adapted from http://stackoverflow.com/questions/2545577/qsplitter-becoming-undistinguishable-between-qwidget-and-qtabwidget
  {
    QSplitterHandle *handle = mainSplitter->handle(1);
    QVBoxLayout *layout = new QVBoxLayout(handle);
    layout->setSpacing(0);
    layout->setMargin(0);
    
    QFrame *line = new QFrame(handle);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);
    layout->addStretch(1);
    line = new QFrame(handle);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);
  }
  

  // We use a queued connection to avoid that a command that displays
  // something and pushes something to the datastack at the end
  // doesn't end up performing double frees...
  //
  // In principle, using queued connections should guarantee that the
  // updates are performed *after* the destruction. This means that
  // the addition should be the last one before leaving the context.
  connect(&(soasInstance->stack()),
          SIGNAL(currentDataSetChanged()),
          curveView,SLOT(showCurrentDataSet()), Qt::QueuedConnection);

  connect(&(soasInstance->stack()),
          SIGNAL(currentDataSetChanged()),
          SLOT(updateWindowName()), Qt::QueuedConnection);

  commandWidget->connect(&soasInstance->stack(), 
                         SIGNAL(currentDataSetChanged()),
                         SLOT(printCurrentDataSetInfo()));

  updateWindowName();

  
  setCentralWidget(mainSplitter);

  commandWidget->setFocus();
}

void MainWin::menuActionTriggered(QAction * action)
{
  QStringList cmd = action->data().toStringList();
  try {
    commandWidget->runCommand(cmd, true);
  }
  catch(const ControlFlowException & flow) {
    Terminal::out << Terminal::bold << "Error: " << flush 
                  << "control flow command " 
                  << flow.message()
                  << " cannot be used outside of a script" << endl;
  }

}

void MainWin::updateWindowName()
{
  setWindowTitle(QString("QSoas (%4): %1 (%2 buffers, %3 kB)").
                 arg(QDir::currentPath()).
                 arg(soasInstance->stack().totalSize()).
                 arg(soasInstance->stack().byteSize() >> 10).
                 arg(SOAS_VERSION)
                 );
}


MainWin::~MainWin()
{
  Terminal::out << "QSoas PID " << QCoreApplication::applicationPid()
                << " closing on " << QDateTime::currentDateTime().toString()
                << endl;

  mainWinSize = size();
  splitterState = mainSplitter->saveState();
  for(QMenu * m : menus)
    delete m;
  menus.clear();
}

void MainWin::showMessage(const QString & str, int ms)
{
  statusBar()->showMessage(str, ms);
}

void MainWin::focusInEvent(QFocusEvent * ev)
{
  QMainWindow::focusInEvent(ev);
  commandWidget->setFocus();    // This doesn't seem to work
                                // systematically.
}

void MainWin::closeEvent(QCloseEvent *event)
{
  qApp->quit();
}
