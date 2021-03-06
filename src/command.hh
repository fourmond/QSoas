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
protected:

  QString cmdName;

  QString shortCmdName;

  QString pubName;

  QString shortDesc;
  
  QString longDesc;

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
          const QString & longDescription = "", 
          CommandContext * context = NULL,
          bool autoRegister = true); 

  Command(const QString & commandName, 
          CommandEffector * effector,
          const QString & groupName, 
          const ArgumentList & arguments,
          const ArgumentList & options,
          const QString & publicName,
          const QString & shortDescription = "", 
          const QString & longDescription = "", 
          CommandContext * context = NULL,
          bool autoRegister = true); 

  /// Unregisters the command upon deletion
  ~Command();

  /// The command name, the one that will be used from the command
  /// prompt.
  ///
  /// This name will not be translated.
  ///
  /// \warning If you reimplement this function, you should set the
  /// the autoRegister parameter to false and do the registration
  /// yourself.
  QString commandName() const {
    return cmdName;
  };

  /// A short command name to be used quickly from the prompt. Most
  /// commands may leave this field empty. This name will not be
  /// translated.
  ///
  /// \warning If you reimplement this function, you should set the
  /// the autoRegister parameter to false and do the registration
  /// yourself.
  QString shortCommandName() const {
    return shortCmdName;
  };


  /// The public name, the one to be used in the menus. This one gets
  /// translated, which means that one should use QT_TRANSLATE_NOOP
  /// macro for setting it.
  QString publicName() const {
    return pubName;
  };

  /// A short description, typically to be used for the status bar.
  QString shortDescription() const {
    return shortDesc;
  };

  /// A long informative description, such as a full help text,
  /// possibly with examples too.
  QString longDescription() const {
    return longDesc;
  };

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

  /// Sets the long description
  void setDocumentation(const QString & str);

  /// Returns a simple string describing the command, all its
  /// arguments and options and if it is interactive or not, in a
  /// specification-like fashion (ie, things that can be
  /// compared). Not for use for a help string. Should be \b stable.
  QString commandSpec(bool full) const;
};

#endif
