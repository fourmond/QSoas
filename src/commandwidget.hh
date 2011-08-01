/**
   \file commandwidget.hh
   The widget handling all "terminal" interaction
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

class CommandPrompt;

/// A widget that accepts commands, and display their result, a
/// successor for the terminal in the old Soas.
///
/// @todo This CommandWidget should hanlde gracefully the interaction
/// during the curve event loop mode:
/// @li a label could go on the right-hand-side of the terminal
/// display, to serve as a reminder of the key bindings and the
/// purpose of the loop mode
/// @li the prompt would be disabled, and potentially replaced by
/// another line edit when prompting for strings.
/// @li entering and going out of that mode should be simple
/// @li during the prompt, all events excepted escape would be
/// forwarded to the prompt line edit
/// 
class CommandWidget : public QWidget {

  Q_OBJECT;

  /// The terminal display
  QTextEdit * terminalDisplay;

  /// The label on the right of the terminal
  QLabel * sideBarLabel;

  /// The line prompt
  CommandPrompt * commandLine;

  /// The prompt
  QLabel * promptLabel;

  /// The restricted prompt:
  QLineEdit * restrictedPrompt;

  /// The CommandWidget that will receive log (Terminal) messages, ie
  /// the first one to be created.
  static CommandWidget * theCommandWidget;

public:

  CommandWidget();
  virtual ~CommandWidget();

  /// Logs the given string to the application-wide CommandWidget
  /// terminal, or to standard output in the case we don't have one of
  /// those ready.
  static void logString(const QString & str);


  /// Switch to loop mode (and back)
  void setLoopMode(bool b);

  /// Sets the text of the prompt
  void setPrompt(const QString & str);

  /// Sets the text of the side bar label (but doesn't show it if it
  /// is hidden)
  void setSideBarLabel(const QString & str);

public slots:

  void runCommand(const QString & str);
  void runCommand(const QStringList & raw);

  /// Appends the given (HTML) text to the log output.
  void appendToTerminal(const QString & str);


protected slots:
  void commandEntered();

};

#endif
