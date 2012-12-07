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

#include <headers.hh>
#ifndef __MAINWIN_HH
#define __MAINWIN_HH

class CommandWidget;
class CurveView;
class Soas;

/// The main window
class MainWin : public QMainWindow {

  Q_OBJECT;

  void setupFrame();

  friend class Soas;

  CommandWidget * commandWidget;

  CurveView * curveView;

  /// The instance of Soas run
  Soas * soasInstance;

  /// The main splitter
  QSplitter * mainSplitter;

public:
  MainWin(Soas * theSoas);
  ~MainWin();

  /// Displays a message on the status bar
  void showMessage(const QString & str, int milliseconds = 3000);

public slots:
  /// Updates the windows name to have the current directory shown.
  void updateWindowName();
protected slots:
  void menuActionTriggered(QAction * action);

protected:
  virtual void focusInEvent(QFocusEvent * evn);

};

#endif
