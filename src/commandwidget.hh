/**
   \file commandwidget.hh
   The widget handling all "terminal" interaction
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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
class CommandContext;
class LineEdit;

// A private class to display the label
class SideBarLabel;

/// This class embeds the context in which a command was run (which
/// command file, which location in the file)
class ScriptContext {
public:
  /// The name of the script file (empty for no script)
  QString scriptFile;

  /// The line number (or command number)
  int lineNumber = 0;

  /// Converts the context to a proper location indication.
  /// The offset gets added to the line number
  QString toString(int offset = 1) const;
};

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
  SideBarLabel * sideBarLabel;

  /// The line prompt
  CommandPrompt * commandLine;

  /// The prompt
  QLabel * promptLabel;

  /// The restricted prompt:
  LineEdit * restrictedPrompt;

  /// The "Stop" button
  QPushButton * stopButton;

  /// The stop button to the right

  /// The CommandWidget that will receive log (Terminal) messages, ie
  /// the first one to be created.
  static QPointer<CommandWidget> theCommandWidget;

  /// The name of the script currently being run. Empty if not running
  /// a script.
  QString scriptFile;

  /// Within a command, this holds the full command-line
  QStringList curCmdline;

  /// Whether we add current commands to the history. Enabled
  /// everywhere but scripts.
  bool addToHistory;

  /// We need Terminal to access the cursors
  friend class Terminal;

public:
  typedef enum {
    Success,
    Error,
    ControlOut
  } ScriptStatus;

  /// The mode of 
  typedef enum {
    Ignore,              // Ignore errors, just go to the next command
    Abort,               // Aborts the script with an error (default)
    Delete,              // Aborts the script as with Abort, and
                         // delete all generated datasets.
    Except               // Aborts the script with an exception, which
                         // may stop the outer script too.
  } ScriptErrorMode;

protected:

  static QHash<QString, ScriptErrorMode> * errorModeNamesHash;

public:

  static const QHash<QString, ScriptErrorMode> & errorModeNames();

protected:


  /// Runs the command coming from the given device. Returns status of
  /// the command.
  ScriptStatus runCommandFile(QIODevice * source, 
                              const QStringList & args = QStringList(),
                              bool addToHist = false,
                              ScriptErrorMode mode = Abort);


  /// Current Ruby string to be executed. Not in ruby mode if the
  /// string is empty. This is handled at the runCommand(const QString
  /// &) level.
  QString rubyCode;

  /// The stack of contexts, gaining a level every time one enters
  /// inside a script
  QList<ScriptContext> contexts;


  /// The command context for the prompt
  CommandContext * commandContext;


  /// Defined parameters. ONLY AVAILABLE IN THE SCRIPTS for now
  QHash<QString, QString> parameters;

public:

  explicit CommandWidget(CommandContext * context = NULL);
  virtual ~CommandWidget();

  /// Returns the context that was used
  CommandContext * promptContext() const;


  /// @name Functions related to ScriptContext
  ///
  /// @{


  /// Enters a new context file
  void enterContext(const QString & file);

  /// Leaves the current context
  void leaveContext();

  /// Advances the current context
  void advanceContext();

  /// Returns the current context
  ScriptContext currentContext() const;
  

  /// @}

  /// Creates a QTextEdit suitable to display the terminal.
  ///
  /// Uses the document of the own terminal if there is already one.
  QTextEdit * createTerminalDisplay();
  

  /// The name of the log file.
  static QString logFileName;


  /// Switch to loop mode (and back)
  void setLoopMode(bool b);

  /// Sets the text of the prompt
  void setPrompt(const QString & str);

  /// Returns the text of the current prompt
  QString currentPrompt() const;

  /// Resets the prompt according to the current context
  void resetPrompt();

  /// Sets the text of the side bar label (but doesn't show it if it
  /// is hidden)
  void setSideBarLabel(const QString & str);

  /// Enter the inner prompting mode
  LineEdit * enterPromptMode(const QString & prompt, 
                             const QString & init = "");

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
  QStringList currentCommandLine() const;

  /// Sets the given named parameter
  void setParameter(const QString & name, const QString & value);


  /// Runs all the startup files
  static void runStartupFiles();

public slots:


  /// Runs the given command-line.
  /// 
  /// This command also handles inline Ruby code between
  /// ruby and ruby end lines.
  ///
  /// Launching a command from the menu bypasses this.
  bool runCommand(const QString & str);

  /// Adds text to the prompt.
  void copyToPrompt(const QString & str);

  /// Runs the given command (already split into words). Returns true
  /// if everything went fine, or false if it finished with an error
  /// (or a control flow exception).
  ///
  /// If @a doFullPrompt is true, then launch a dialog box for
  /// prompting for all the arguments/options.
  bool runCommand(const QStringList & raw, bool doFullPrompt = false);

  /// Scrolls the terminal by that many half screens.
  void scrollTerminal(int nb);

  /// Adds a small description of the current dataset onto the
  /// terminal.
  void printCurrentDataSetInfo(); 

  /// Runs the commands contained in a file.
  ScriptStatus runCommandFile(const QString & fileName, 
                              const QStringList & args = QStringList(),
                              bool addToHist = false,
                              ScriptErrorMode mode = Abort);

protected slots:
  void commandEntered();

  void onMenuRequested(const QPoint & pos);
};

#endif
