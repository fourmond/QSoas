/**
   \file commandwidget.hh
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

#ifndef __COMMANDWIDGET_HH
#define __COMMANDWIDGET_HH

/// A widget that accepts commands, and display their result, a
/// successor for the terminal in the old Soas.
class CommandWidget : public QWidget {

  Q_OBJECT;

  /// The line edition, to be later blessed with expansion
  /// capabilities.
  QLineEdit * commandLine;

  /// The log display
  ///
  /// @todo I need to design decent log facilities...
  QTextEdit * logDisplay;

  // /// The unique CommandWidget !
  // ///
  // /// There can be several, but only one will be the target
  // static CommandWidget * theCommandWidget;

public:

  CommandWidget();
  virtual ~CommandWidget();

public slots:

  void runCommand(const QString & str);
  void runCommand(const QStringList & raw);

protected slots:
  void commandEntered();

};

#endif
