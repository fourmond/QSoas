/**
   \file mainwin.hh
   Main window for QSoas
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

#ifndef __MAINWIN_HH
#define __MAINWIN_HH

class CommandWidget;

/// The main window
class MainWin : public QMainWindow {

  Q_OBJECT;

  void setupFrame();

  CommandWidget * commandWidget;

  /// The MainWin that will receive messages
  static MainWin * theMainWindow;


public:
  MainWin();
  ~MainWin();

  /// Displays a message on the status bar of theMainWindow;
  static void showMessage(const QString & str);

protected slots:
  void menuActionTriggered(QAction * action);

};

#endif
