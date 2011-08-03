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
   \li an application wide template-based (?) way to save/restore
   settings, based on a class registering all settings. That would be
   a worty addition to eThunes too, come to think of it. That even
   could result in data wrappers.
   

   Now, mainly, what I need to do, is to massage the functionalities
   of the old Soas back into this one:
   \li undo/redo
   \li filter/derivative
   \li reglin
   \li subtract
   \li splita/b
   \li cut
   \li cd/pwd
   \li div
   \li browse ;-)...
   

   I also need to come up with a decent way to handle "output files"
   (peak data, but also cursor positions, and the like). Then:
   \li peak detection
   \li cursor manipulation

   
*/

#include <headers.hh>
#include <mainwin.hh>
#include <group.hh>
#include <command.hh>

#include <soas.hh>

#include <commandwidget.hh>
#include <curveview.hh>
#include <datastack.hh>

MainWin::MainWin()
{
  soasInstance = new Soas(this);
  setupFrame();
}

void MainWin::setupFrame()
{
  statusBar();
  Group::fillMenuBar(menuBar());
  connect(menuBar(), SIGNAL(triggered(QAction *)),
          SLOT(menuActionTriggered(QAction *)));

  QSplitter * s = new QSplitter(Qt::Vertical);
  // QVBoxLayout * layout = new QVBoxLayout(w);
  curveView = new CurveView;
  s->addWidget(curveView);
  commandWidget = new CommandWidget;
  s->addWidget(commandWidget);
  curveView->setFocusProxy(commandWidget);
  s->setFocusProxy(commandWidget);

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

  
  setCentralWidget(s);

  commandWidget->setFocus();
}

void MainWin::menuActionTriggered(QAction * action)
{
  QStringList cmd = action->data().toStringList();
  commandWidget->runCommand(cmd);
}


MainWin::~MainWin()
{
}

void MainWin::showMessage(const QString & str, int ms)
{
  statusBar()->showMessage(str, ms);
}
