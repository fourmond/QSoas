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
   \li FFT filtering/derivation
   \li make the fits formulas available (LaTeX ?) -> try to design a
   clever way to do so... (embedded LaTeX would be cool, but hard to
   handle => no: compile first, have a method produce the LaTeX
   sources, and make a PDF from that -- and a CHTML file ? Look at the
   help brower)
   \li option for k to limit the number of stuff displayed
   \li include (or not) some data when using multifit (weight globally
   a buffer ?)
   @li a command to terminate the execution of a script.

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
   \li find a way to have a "default" optional parameter whose name
   could be omitted: that would allow for an "optional" parameter to
   be specified as "default optional parameter"
   \li add a "basic string" fallback for parameters/options
   \li come up with a full dialog box to edit @b all arguments to a
   command. (including options !)
   \li history save/restore upon exit => probably not...
   \li resampling (two possibilities: first, as a side-effect of the
   filtering, second as a distinct function)
   \li definition of new fits based on ruby subclasses of Fit,
   PerDatasetFit and FunctionFit
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

   Conditions files:
   \li annotate the buffers with information gotten from the
   conditions.dat files I use so often, ie tab-separated
   (customizable) files with a ##-starting header line (customizable
   too). Ideally the conditions metadata handling would be written in
   pure Ruby, with a simple interface from C++ (but not the other way
   around), so that conditions files would be parseable in Ruby.

   Fits:
   \li annotate buffers
   \li option to pass certain parameters in the form of logarithms
   (this could be done at the FitData level ?)
   \li options to automatically apply a bijection to the Y values too.
   \li optionnally add user-specified parameters, when the formula system
   for fit parameters is ready.

   Generally speaking, I should setup a whole way to transform
   parameters. We have two parameter space: one natural parameter
   space in which the problem is written. A fit parameter space, which
   is is general a subset of the natural parameters (with provisions
   made to factor out the fixed and/or common parameters). I think
   this could be extended greatly:

   \li FitData or FitParameters could provide generic transformations
   (use the logarithm of the natural parameter as fit parameter, for
   instance). This would also provide a way to generally restrict the
   parameter space, by using for instance a hyperbolic tangent
   transform or something like that.

   For that, I need a better abstraction for the fit parameters as
   there is now. I should come up with a design that:

   \li interface between GSL fit parameters, and the parameters as
   seen from Fit children (though FitData::unpackParameters) \li
   provide classes for handling normal, fixed, global, formula-based,
   constrained parameters...
   \li use a single parameter list using all those classes (ideas:
   functions to tell if the parameter belongs or not into the GSL fit
   parameters, and a way to unpack the parameters, possibly with
   dependencies)
   \li provide a decent editor for a single parameter (which means
   handling the fact that we are dealing with derived classes ? - and
   potential problems when switching from buffer-local to global
   parameters...)
   

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


   \section arch Architectural things

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
   ?
   
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

  Terminal::out << "This is Soas version " << SOAS_VERSION
                << " for Qt\n"
                << SOAS_BUILD_INFO 
                << "Copyright 2011-2012 by Vincent Fourmond\n"
                << "Based on Christophe Leger's original Soas\n\n"
                << "This program is free software, released under the terms of \n"
                << "the GNU general public license (see http://www.gnu.org/copyleft/gpl.html)\n\n" 
                << "Current temperature is: " << soasInstance->temperature() 
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
