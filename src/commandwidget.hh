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

#include <headers.hh>
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

  /// A file to the terminal output.
  QIODevice * watcherDevice;

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
  static QPointer<CommandWidget> theCommandWidget;

  /// The name of the script currently being run. Empty if not running
  /// a script.
  QString scriptFile;

  /// Within a command, this holds the full command-line
  QString curCmdline;

  /// Whether we add current commands to the history. Enabled
  /// everywhere but scripts.
  bool addToHistory;

  /// Runs the command coming from the given device
  void runCommandFile(QIODevice * source, 
                      const QStringList & args = QStringList(),
                      bool addToHist = false);

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

  /// Enter the inner prompting mode
  QLineEdit * enterPromptMode(const QString & prompt);

  /// Leave the inner prompting mode
  void leavePromptMode();

  /// Returns the whole string contained by the terminal (as plain text)
  QString terminalContents() const;

  /// Returns the history of all commands run so far.
  ///
  /// @todo if I do history loading from settings, I'll have to deal
  /// with that.
  QStringList history() const;

  /// Returns a modifiable list of startup files, ie a list of files
  /// that are read at startup.
  static QStringList & startupFiles();

  /// Returns the name of the script currently being run. Returns an
  /// empty string should no script be running.
  const QString & scriptFileName() const;

  /// Returns the full command line being run
  const QString & currentCommandLine() const;


  /// Runs all the startup files
  static void runStartupFiles();

public slots:


  bool runCommand(const QString & str);

  /// Runs the given command (already split into words). Returns true
  /// if everything went fine, or false if it finished with an error
  /// (or a control flow exception).
  bool runCommand(const QStringList & raw);

  /// Appends the given (HTML) text to the log output.
  void appendToTerminal(const QString & str);

  /// Scrolls the terminal by that many half screens.
  void scrollTerminal(int nb);

  /// Adds a small description of the current dataset onto the
  /// terminal.
  void printCurrentDataSetInfo(); 

  /// Runs the commands contained in a file.
  void runCommandFile(const QString & fileName, 
                      const QStringList & args = QStringList(),
                      bool addToHist = false);

protected slots:
  void commandEntered();

  void onMenuRequested(const QPoint & pos);
};

#endif
