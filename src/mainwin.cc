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
   \li automatic completion and history from the QLineEdit (or a child
   thereof)
   \li slurping (or greedy) arguments
   \li their use for files (using a new argument)
   \li backends infrastructure for data loading (with automatic
   detection)
   \li String argument with fixed list (for completion, useful for
   options, such as an option to force to use a given backend)
   \li data display (based on QGraphicsScene)
   \li a way to enter a "private event loop" for the graphics display
   (point selection and the like)

*/

#include <headers.hh>
#include <mainwin.hh>
#include <group.hh>
#include <command.hh>

#include <commandwidget.hh>

MainWin::MainWin()
{
  setupFrame();
}

void MainWin::setupFrame()
{
  statusBar();
  Group::fillMenuBar(menuBar());
  connect(menuBar(), SIGNAL(triggered(QAction *)),
          SLOT(menuActionTriggered(QAction *)));
  commandWidget = new CommandWidget;
  setCentralWidget(commandWidget);
}

void MainWin::menuActionTriggered(QAction * action)
{
  QStringList cmd = action->data().toStringList();
  Command::runCommand(cmd);
  /// @todo exception handling here. Or maybe somewhere else ?
  /// Maybe the command-line widget should handle that ?
}


MainWin::~MainWin()
{
}

