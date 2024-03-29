/**
   \file command.hh
   Command handling for QSoas.
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2018 by CNRS/AMU

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
#ifndef __COMMAND_HH
#define __COMMAND_HH

#include <argumentmarshaller.hh>

class Group;
class CommandEffector;
class ArgumentList;
class CommandContext;

/// The class representing a command.
class Command {
  Command & operator=(const Command & cmd) = delete;
protected:

  QString cmdName;

  /// The list of aliases for the command
  QStringList aliases;

  QString pubName;

  QString shortDesc;

  QString groupName;

  /// The arguments list. Can be NULL if no arguments are expected.
  ArgumentList * arguments;

  /// The options, also in the form of an ArgumentList. NULL if no
  /// options.
  ArgumentList * options;

  /// Whether the command is custom (i.e. created after the creation
  /// of the main window or not)
  bool custom;

  /// Whether

  /// The CommandContext for the Command
  CommandContext * context;

  /// Registers this command
  void registerMe();


  friend class CommandContext;
private:
  // Disable copy constructor
  Command(const Command & cmd) = delete;

public:

  /// Parse the arguments, possibly prompting for them if the given
  /// widget isn't NULL
  ///
  /// If @a defaultOption isn't NULL, then the first extra argument
  /// will end up there.
  CommandArguments parseArguments(const QStringList & arguments,
                                  QStringList * defaultOption = NULL,
                                  QWidget * base = NULL,
                                  bool * prompted = NULL) const;

  /// Parse the options. Doesn't prompt.
  CommandOptions parseOptions(const QMultiHash<QString, QString> & opts, 
                              QStringList * defaultOption = NULL) const;

  /// The effector, ie the code that will actually run the command.
  CommandEffector * effector;

  /// The group to which this command belongs.
  Group * group;

  /// Whether the command is custom or built-in
  bool isCustom() const { return custom; };
  


  /// Specifies the various elements linked to the Command.
  ///
  /// @b Ownership:
  /// @li Command takes ownership of the @a effector
  Command(const QString & commandName, 
          CommandEffector * effector,
          const QString & groupName, 
          const ArgumentList * arguments,
          const ArgumentList * options,
          const QString & publicName,
          const QString & shortDescription = "", 
          const QString & alias = "", 
          CommandContext * context = NULL,
          bool autoRegister = true); 

  Command(const QString & commandName,
          CommandEffector * effector,
          const QString & groupName,
          const ArgumentList * arguments,
          const ArgumentList * options,
          const QString & publicName,
          const QString & shortDescription,
          const QStringList & aliases, 
          CommandContext * context = NULL,
          bool autoRegister = true); 

  Command(const QString & commandName, 
          CommandEffector * effector,
          const QString & groupName, 
          const ArgumentList & arguments,
          const ArgumentList & options,
          const QString & publicName,
          const QString & shortDescription = "", 
          const QString & alias = "", 
          CommandContext * context = NULL,
          bool autoRegister = true); 

  /// Unregisters the command upon deletion
  ~Command();

  /// The command name, the one that will be used from the command
  /// prompt.
  QString commandName() const;
  
  /// All the aliases of the command
  QStringList commandAliases() const;

  /// The public name, the one to be used in the menus.
  QString publicName() const;

  /// A short description, typically to be used for the status bar.
  QString shortDescription() const;


  /// Returns the arguments to this command. This can be NULL !
  const ArgumentList * commandArguments() const {
    return arguments;
  };

  /// Returns the options to this command. This can be NULL !
  const ArgumentList * commandOptions() const {
    return options;
  };

  /// Returns the CommandContext for this command
  const CommandContext * commandContext() const {
    return context;
  };

  /// Runs the command with the given arguments.
  ///
  /// This function possibly can prompt for missing arguments, if base
  /// isn't NULL.
  void runCommand(const QString & commandName, 
                  const QStringList & arguments,
                  QWidget * base = NULL);

  /// Just parses the arguments and options. Returns true if prompting
  /// was necessary.
  bool parseArgumentsAndOptions(const QStringList & arguments,
                                CommandArguments * args,
                                CommandOptions * opts,
                                QWidget * base = NULL);

  /// Runs the command with parsed arguments and options
  void runCommand(const QString & commandName,
                  const CommandArguments & arguments,
                  const CommandOptions & options);

  /// Runs the command from a Ruby command-line
  void runCommand(int nb, mrb_value * args);

  /// Converts the parsed arguments back to a string representation
  /// that should be parsed back to the same arguments.
  ///
  /// The string representation is @b not @b guaranteed to be the same
  /// as the original parsed QStringList, <b> including for already
  /// parsed arguments </b> !
  QStringList rebuildCommandLine(const CommandArguments & args,
                                 const CommandOptions & opts) const;
  


  /// Returns an action for this Command parented by the given parent.
  QAction * actionForCommand(QObject * parent) const;

  /// This function takes a list of word-splitted command-line
  /// arguments, and splits them into arguments and options,
  /// regardless of the command used.
  ///
  /// The syntax is the following:
  /// 
  /// \li options are in the format /option = thing, where the spaces
  /// and the = sign are optional, and option matches [a-zA-Z-]+
  /// 
  /// \li a /! at the beginning of anything but the argument to an
  /// option is stripped, and the resulting string is added to the
  /// arguments, for the cases when one would wish to have an argument
  /// in the form of /something
  ///
  /// \li everything else is as argument.
  ///
  /// If the optional argument \p annotate isn't NULL, then it is
  /// filled with a correspondance argument -> argument number or -1
  /// if it is an option.
  ///
  /// \p pendingOption is set to true if an option is still pending to
  /// be parsed.
  static QPair<QStringList, QMultiHash<QString, QString> > 
  splitArgumentsAndOptions(const QStringList & rawArgs,
                           QList<int> * annotate = NULL,
                           bool * pendingOption = NULL);

  /// Splits the given command-line into words.
  ///
  /// If \p wordBegin isn't NULL, the target is cleared and filled
  /// with the position of the first character of each word returned
  /// in QStringList. Same goes for \p wordEnd, that points to the
  /// char \b after the end of the word.
  static QStringList wordSplit(const QString & args, 
                               QList<int> * wordBegin = NULL,
                               QList<int> * wordEnd = NULL);

  /// Quotes the given string so that it won't be split by wordSplit().
  static QString quoteString(const QString & str);

  /// Does the reverse of wordSplit, (or almost), while trying to be
  /// clever on the quoting side.
  static QString unsplitWords(const QStringList & cmdline);

  
  /// Makes up a text synopsis for the command
  QString synopsis(bool markup = false) const;

  /// Returns true when the command requires user interaction (such as
  /// the event loop, for instance)
  bool isInteractive() const;

  /// Returns true when the command has neither arguments nor options
  /// (so that prompting does not need to occur)
  bool hasNoArgsNorOpts() const;


  /// Updates a QString containing the documentation of the command
  /// (in kramdown format). This documentation is split into two
  /// parts:
  /// @li first, the synopsis, which is always updated from to
  /// program to the documentation file
  /// @li second, the long description, updated from the documentation
  /// file to the program (or for the first time, the other way
  /// around)
  ///
  /// The headings says which level of nesting is used for commands.
  QString & updateDocumentation(QString & str, int headings = 3) const;

  /// Returns a simple string describing the command, all its
  /// arguments and options and if it is interactive or not, in a
  /// specification-like fashion (ie, things that can be
  /// compared). Not for use for a help string. Should be \b stable.
  QString commandSpec(bool full) const;


  /// Checks that the options are consistent -- in particular, raise
  /// an exception when using an option with two times the name.
  void checkOptions() const;

  /// @name Progress report and interruption
  ///
  /// Series of and functions related to the progress report and
  /// possible interruption
  ///
  /// @{
protected:

  /// If true, the current command should be interrupted, going back
  /// to the main QSoas prompt.
  ///
  /// That happens in the progress report
  static bool shouldStop;


  /// The current step in the progress
  static int currentStep;

  /// The current target
  static int currentTarget;

  /// Time to the last call to currentProgress()
  static qint64 timeLastCall;

  /// Last call to the event loop
  static qint64 timeLastLoop;

  /// The current command being run
  static Command * currentCommand;

public:

  /// Records the progress of the current command. It also does the
  /// following things:
  /// * @li record the current time
  /// * @li run the event loop if the last call to the event loop was
  ///   too far away
  /// * interrupts the current command back to the main QSoas loop if
  /// interruption has been requested
  ///
  /// When the "duration" of the task is known both step and target
  /// should be positive
  ///
  /// When the target duraction isn't known, step should be positive
  /// and increase and target should be negative
  ///
  /// When the command is interactive, the two should be negative.
  /// This is called automatically from the command loop.
  ///
  /// That function is called at the beginning of each function with
  /// 0,0 as arguments. This will just trigger the check for cancellation.
  ///
  /// @todo We should have a way to know how much time has elapsed
  /// since the last time the command prompt was available.
  static void currentProgress(int step, int target);

  /// Returns the currently running command.
  /// Should be NULL when in the main loop
  static Command * runningCommand();

  /// Request to stop the current command stack and go back to the
  /// main QSoas loop.
  static void requestStop();

  /// @}

};

#endif
