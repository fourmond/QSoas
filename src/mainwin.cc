/*
  mainwin.cc: Main window for QSoas
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

/**
   \mainpage Projects for QSoas

   \section various-todo Various things to do

   Now, mainly, what I need to do, is to massage the functionalities
   of the old Soas back into this one
   \li cd/pwd
   \li browse ;-)...
   \li dataset selection
   \li peak detection (along with a CurvePeak CurveItem child to display
   it neatly)
   \li catalytic baseline
   \li FFT, based on GSL too (no need to pull in another
   dependency)
   \li filter/derivative
   \li convolution, that was quite useful
   \li step detection, dead useful too !
   

   And other things too:
   \li history save/restore upon exit => probably not...
   \li resampling
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


   \section requests Requests



   \section arch Architectural things

   (nothing for now)
   
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

  Terminal::out << "This is Soas version " << SOAS_VERSION << " for Qt\n"
                << "Copyright 2011 by Vincent Fourmond\n"
                << "Based on Christophe Leger's original Soas\n\n"
                << "This program is free software, released under the terms of \n"
                << "the GNU general public license (see http://www.gnu.org/copyleft/gpl.html)" 
                << endl;
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

  
  setCentralWidget(mainSplitter);

  commandWidget->setFocus();
}

void MainWin::menuActionTriggered(QAction * action)
{
  QStringList cmd = action->data().toStringList();
  commandWidget->runCommand(cmd);
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
