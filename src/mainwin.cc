/*
  mainwin.cc: Main window for QSoas
  Copyright 2011, 2012 by Vincent Fourmond

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

   \li find a way to display current directory on the printed stuff
   \li add a whole bunch of functionalities to the stack/data browser
   \li add arbitrary text to print ? (could this just be an annotate
   command adding text to the current plot ? It would be nice to
   choose its position, then ? -> no, just a waste of time)
   \li use a third column for the fits (but an interface needs to be
   clearly defined for that...); that also means to provide ways to
   tweak the Z values...
   \li make the fits formulas available (LaTeX ?) -> try to design a
   clever way to do so... (embedded LaTeX would be cool, but hard to
   handle => no: compile first, have a method produce the LaTeX
   sources, and make a PDF from that -- and a CHTML file ? Look at the
   help brower)
   \li include (or not) some data when using multifit (weight globally
   a buffer ?)

   \section bugs Known bugs
   @li hard crashes on cut/deldp (index out of bounds)
   @li fix bounding box calculation on infinite/NaN values.

   \section various-todo Various things to do

   Now, mainly, what I need to do, is to massage the functionalities
   of the old Soas back into this one

   \li pwd
   \li finalize peak detection (along with a CurvePeak CurveItem child
   to display it neatly)
   \li catalytic baseline
   \li convolution, that was quite useful
   \li step detection, dead useful too !
   \li put back the temperature settings, and the values that depend
   on it (such as values of n on linear regressions and so on)

   And other things too:

   \li improve distance-to-dataset
   \li dataset selection
   \li add a "basic string" fallback for parameters/options
   \li come up with a full dialog box to edit @b all arguments to a
   command. (including options !)
   \li history save/restore upon exit => probably not...
   \li resampling (two possibilities: first, as a side-effect of the
   filtering, second as a distinct function)
   \li definition of new fits based on ruby subclasses of Fit,
   PerDatasetFit and FunctionFit (much less useful now that we have
   custom fit loading)
   \li keyboard shortcuts for navigation in FitDialog: quick goto
   label editor, navigation between buffers, shortcuts for export/save
   and so on...
   \li an apply-formula command spanning several datasets (using x[0],
   x[1] and so on...)
   \li go further with the steps system, ie allow one to add the steps
   in a permanent fashion to a DataSet (including setting the steps
   manually -- or with a user-specified index list), and to use this
   information to perform steps-aware subtractions/divisions (padding
   with last values/first values ?)

   @b Qt-related @b stuff:

   @li maybe one day Utils::registerShortCut could be used to
   store/annotate the shortcuts available in a window ?

   @li I should come up with button-like widget containers that would
   help selection of datasets in DatasetBrowser, and use these
   selection for various operations...

   @b Conditions @b files:
   \li annotate the buffers with information gotten from the
   conditions.dat files I use so often, ie tab-separated
   (customizable) files with a ##-starting header line (customizable
   too). Ideally the conditions metadata handling would be written in
   pure Ruby, with a simple interface from C++ (but not the other way
   around), so that conditions files would be parseable in Ruby.


   I need to setup a neat data browser to replace the old browse
   command, and something that could also be used to display datasets
   we want to choose from. Ideas:
   \li use a popup dialog like the fit dialog box
   \li it would take a list of DataSet as input, (but not necessarily
   DataSet which belong to the DataStack)
   \li navigate through pages using arrows (make up a utility function
   for creating arrows in Utils !)
   \li use a given number of CurveView on each, possibly customizable
   \li make provisions for checkboxes  (right
   under the corresponding CurveView ?)
   \li offer the possibility to add actions, most probably through
   callbacks (that take the list of checked things and a pointer to
   the current list as well, so that deletion of things from the stack
   is a possibility for instance without closing the dialog box --
   and the list of currently displayed CurveViews ? (that would allow 
   printing)

   Allow customization for load:
   \li columns (X be 2, Y be 1 and so on)
   \li comment chars
   \li separator for text files ?
   \li gracefully handling column with text ?

   Actually, each instance of backend should register its own command
   ('backend'-load) with a whole bunch of additional options (such as
   the aforementioned), that would only be available when directly
   using the appropriate command (and would bypass backend detection)

   Find a way to prompt for additional arguments for greedy parameters
   (using an additional button in the dialog box ?)


   \section fits Fit-related things

   \li options to automatically apply a bijection to the Y values too.
   \li possibility to add user-specified parameters, when the formula
   system for fit parameters is ready.
   \li detection of "weird" parameters, ie a parameter that suddenly
   takes INF or NAN values while it didn't take before. This is a
   clear sign of something going wrong.

   @b Fits @b dialogs:

   Some useability improvements:

   @li It should be possible to build a "stack" of FitParameters
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

   @b Fit @b drivers

   For now, the only way to perform fits is through the use of the
   GSL. However, it may be desirable to implement other fit
   "backends", ie objects that just initialize data based on a FitData
   object and conduct the fit, iteration by iteration. Maybe the
   driver could be changed at the FitData level (ie allocation not at
   allocation time, but at the time of "binding" with FitData).  That
   may eventually allow orthogonal fits later on, although this may
   require strong structural changes, which may not be desirable at
   all. The fit driver should be switchable at run-time within the
   dialog.

   This is necessary since the GSL fit engine is showing some of its
   shortcomings.

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

   @b Expressions

   In many places I could use a decent expressions system, ie
   something à la SCalc that would allow for easy evaluations, but
   based on Ruby. Bonus points if I can come up with something that
   can evaluate derivatives, but at this point it doesn't matter so
   much.

   In any case, this must be a C++ object that doesn't require
   external manipulations of VALUE, with introspection capacities
   (which parameters, evaluation with a hash, with a const double *)

   This would warrant a decent rewrite of MultiBufferArbitraryFit

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
   do find for most types.

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
   
*/

#include <headers.hh>
#include <mainwin.hh>
#include <group.hh>
#include <command.hh>

#include <soas.hh>

#include <commandwidget.hh>
#include <terminal.hh>
#include <curveview.hh>
#include <datastack.hh>

#include <settings-templates.hh>

#include <build.hh>
#include <ducksim.hh>

static SettingsValue<QSize> mainWinSize("mainwin/size", QSize(700,500));

static SettingsValue<QByteArray> splitterState("mainwin/splitter", 
                                               QByteArray());

MainWin::MainWin()
{
  soasInstance = new Soas(this);
  setupFrame();
  resize(mainWinSize);
  if(! splitterState->isEmpty())
    mainSplitter->restoreState(splitterState);

  Terminal::out << "This is QSoas version " << SOAS_VERSION << "\n"
                << SOAS_BUILD_INFO 
                << "Copyright 2011-2012 by Vincent Fourmond\n"
                << "Based on Christophe Leger's original Soas\n\n"
                << "This program is free software, released under the terms of \n"
                << "the GNU general public license (see http://www.gnu.org/copyleft/gpl.html)\n\n" 
                << "Starting at " << QDateTime::currentDateTime().toString()
                << "\nCurrent temperature is: " << soasInstance->temperature() 
                << " K\n\n" 
                << "To list available commands, type 'commands'\n"
                << "To get help on a specific command, type 'help command'\n"
                << endl;
  DuckSimFit::initialize();
}

void MainWin::setupFrame()
{
  statusBar();
  Group::fillMenuBar(menuBar());
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
    commandWidget->runCommand(cmd);
  }
  catch(const ControlFlowException & flow) {
    Terminal::out << Terminal::bold("Error: ") << "control flow command " 
                  << flow.message()
                  << " cannot be used outside of a script" << endl;
  }

}

void MainWin::updateWindowName()
{
  setWindowTitle(QString("QSoas: %1").arg(QDir::currentPath()));
}


MainWin::~MainWin()
{
  mainWinSize = size();
  splitterState = mainSplitter->saveState();
  delete soasInstance;
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
