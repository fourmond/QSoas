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

   \section project-overview How the new Soas is/will be organized

   \section various-todo Various things to do
   \li String argument with fixed list (for completion, useful for
   options, such as an option to force to use a given backend)

   Now, mainly, what I need to do, is to massage the functionalities
   of the old Soas back into this one:
   \li filter/derivative
   \li cd/pwd
   \li browse ;-)...
   \li dataset selection
   \li peak detection (along with a CurvePeak CurveItem child to display
   it neatly)
   \li catalytic baseline
   \li FFT, probably based on GSL too (no need to pull in another
   dependency)
   

   And other things too:
   \li history save/restore upon exit
   \li resampling

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
   
   \li One day, I will need to reimplement the exceptions to use my
   own QString-carrying exceptions. This probably isn't of high
   priority, but that would save quite a bit of code.
   
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
