/**
   \file command.hh
   Command handling for QSoas.
   Copyright 2011 by Vincent Fourmond
             2012, 2013 by CNRS/AMU

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
#include <ruby-wrappers.h>

class Group;
class CommandEffector;
class ArgumentList;

/// The class representing a command.
class Command {
protected:

  QString cmdName;

  QString shortCmdName;

  QByteArray pubName;

  QByteArray shortDesc;
  
  QString longDesc;

  QByteArray groupName;

  /// A global hash holding a correspondance name->command
  ///
  /// @todo This could be turned into (or coupled with) a trie to have
  /// automatic completion ?
  static QHash<QString, Command*> * availableCommands;

  /// Registers the given command to the static registry
  static void registerCommand(Command * cmd);


  /// The arguments list. Can be NULL if no arguments are expected.
  ArgumentList * arguments;

  /// The options, also in the form of an ArgumentList. NULL if no
  /// options.
  ArgumentList * options;

  /// Whether the command is custom (i.e. created after the or not.
  bool custom;

  /// A static flag to tell whether we have finished the initial load
  /// or not. Set during the call to crosslinkCommands().
  static bool finishedLoading;

public:

  /// Parse the arguments, possibly prompting for them if the given
  /// widget isn't NULL
  ///
  /// If @a defaultOption isn't NULL, then the first extra argument
  /// will end up there.
  CommandArguments parseArguments(const QStringList & arguments,
                                  QString * defaultOption = NULL,
                                  QWidget * base = NULL) const;

  /// Parse the options. Doesn't prompt.
  CommandOptions parseOptions(const QMultiHash<QString, QString> & opts, 
                              QString * defaultOption = NULL) const;

  /// The effector, ie the code that will actually run the command.
  CommandEffector * effector;

  /// The group to which this command belongs.
  Group * group;

  /// Whether the command is custom or built-in
  bool isCustom() const { return custom; };
  


  /// Specifies the various elements linked to the Command.
  Command(const char * cn, 
          CommandEffector * eff,
          const char * gn, 
          ArgumentList * ar,
          ArgumentList * op,
          const char * pn,
          const char * sd = "", 
          const char * ld = "", 
          const char * sc = "", 
          bool autoRegister = true) : 
    cmdName(cn), shortCmdName(sc), pubName(pn), 
    shortDesc(sd), longDesc(ld), groupName(gn), 
    arguments(ar), options(op), custom(finishedLoading),
    effector(eff), 
    group(NULL) {
    if(autoRegister)
      registerCommand(this);
  }; 

  Command(const char * cn, 
          CommandEffector * eff,
          const char * gn, 
          ArgumentList * ar,
          ArgumentList * op,
          const QByteArray & pn,
          const QByteArray & sd = "", 
          const QByteArray & ld = "", 
          const QByteArray & sc = "", 
          bool autoRegister = true) : 
    cmdName(cn), shortCmdName(sc), pubName(pn), 
    shortDesc(sd), longDesc(ld), groupName(gn), 
    arguments(ar), options(op), custom(finishedLoading),
    effector(eff), 
    group(NULL) {
    if(autoRegister)
      registerCommand(this);
  }; 
  

  /// The command name, the one that will be used from the command
  /// prompt.
  ///
  /// This name will not be translated.
  ///
  /// \warning If you reimplement this function, you should set the
  /// the autoRegister parameter to false and do the registration
  /// yourself.
  /*virtual*/ QString commandName() const {
    return cmdName;
  };

  /// A short command name to be used quickly from the prompt. Most
  /// commands may leave this field empty. This name will not be
  /// translated.
  ///
  /// \warning If you reimplement this function, you should set the
  /// the autoRegister parameter to false and do the registration
  /// yourself.
  /*virtual*/ QString shortCommandName() const {
    return shortCmdName;
  };


  /// The public name, the one to be used in the menus. This one gets
  /// translated, which means that one should use QT_TRANSLATE_NOOP
  /// macro for setting it.
  /*virtual*/ QString publicName() const {
    return pubName;
  };

  /// A short description, typically to be used for the status bar.
  /*virtual*/ QString shortDescription() const {
    return shortDesc;
  };

  /// A long informative description, such as a full help text,
  /// possibly with examples too.
  /*virtual*/ QString longDescription() const {
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

  /// Categorize the commands within groups. This function \b must be
  /// called at the beginning of main.
  static void crosslinkCommands();

  /// Returns the named command. If rubyConversion is true, then
  /// underscores are converted to dashes.
  static Command * namedCommand(const QString & cmd, bool rubyConversion = false);

  /// Runs the command with the given arguments.
  ///
  /// This function possibly can prompt for missing arguments, if base
  /// isn't NULL.
  /*virtual*/ void runCommand(const QString & commandName, 
                          const QStringList & arguments,
                          QWidget * base = NULL);

  /// Runs the command with parsed arguments and options
  /*virtual*/ void runCommand(const QString & commandName,
                          const CommandArguments & arguments,
                          const CommandOptions & options);

  /// Runs a command from the command prompt, already split into
  /// words. The first word is therefore the command name.
  static void runCommand(const QStringList & args, 
                         QWidget * base = NULL);

  /// Runs the command from a Ruby command-line
  void runCommand(int nb, RUBY_VALUE * args);

  /// Returns an action for this Command parented by the given parent.
  /*virtual*/ QAction * actionForCommand(QObject * parent) const;

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

  /// All available commands
  static QStringList allCommands();

  /// A set of all the commands
  static QSet<Command *> uniqueCommands();

  /// All interactive commands
  static QStringList interactiveCommands();

  /// All non-interactive commands
  static QStringList nonInteractiveCommands();

  /// Returns a LaTeX string documenting the command (using
  /// subsection)
  ///
  /// @deprecated
  QString latexDocumentation() const;

  
  /// Makes up a text synopsis for the command
  QString synopsis(bool markup = false) const;

  bool isInteractive() const;


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

  /// Loads the documentation from the given string, and returns a
  /// list of commands for which the documentation was missing.
  static QStringList loadDocumentation(const QString & str);

  /// Returns a simple string describing the command, all its
  /// arguments and options and if it is interactive or not, in a
  /// specification-like fashion (ie, things that can be
  /// compared). Not for use for a help string. Should be \b stable.
  QString commandSpec(bool full) const;

  /// Writes out a specification for all commands, in the alphabetic
  /// order.
  static void writeSpecFile(QTextStream & out, bool full);


  /// Unregisters a given command. This does not delete it.
  static void unregisterCommand(Command * cmd);
};

#endif
