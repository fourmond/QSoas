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
   \li data display (based on QGraphicsScene, it handles everything I need)
   \li a way to enter a "private event loop" for the graphics display
   (point selection and the like)

   \li an application wide template-based (?) way to save/restore
   settings, based on a class registering all settings. That would be
   a worty addition to eThunes too, come to think of it. That even
   could result in data wrappers.

*/

#include <headers.hh>
#include <mainwin.hh>
#include <group.hh>
#include <command.hh>

#include <commandwidget.hh>
#include <curvedisplaywidget.hh>

MainWin * MainWin::theMainWindow = NULL;

MainWin::MainWin()
{
  setupFrame();
  theMainWindow = this;
}

void MainWin::setupFrame()
{
  statusBar();
  Group::fillMenuBar(menuBar());
  connect(menuBar(), SIGNAL(triggered(QAction *)),
          SLOT(menuActionTriggered(QAction *)));

  QSplitter * s = new QSplitter(Qt::Vertical);
  // QVBoxLayout * layout = new QVBoxLayout(w);
  curveDisplayWidget = new CurveDisplayWidget;
  s->addWidget(curveDisplayWidget);
  commandWidget = new CommandWidget;
  s->addWidget(commandWidget);
  curveDisplayWidget->setFocusProxy(commandWidget);
  s->setFocusProxy(commandWidget);
  
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

void MainWin::showMessage(const QString & str)
{
  theMainWindow->statusBar()->showMessage(str,  3000);
}
