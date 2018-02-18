/**
   \file commandcontext.hh
   Context for Command
   Copyright 2018 by CNRS/AMU

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
#ifndef __COMMANDCONTEXT_HH
#define __COMMANDCONTEXT_HH

class Command;

/// This class represent an execution context for the commands, that
/// is a list of commands to be executed in a given context.
///
/// All the "regular" commands belong to the 
class CommandContext {
protected:

  /// The hash containing all the available contexts.
  static QHash<QString, CommandContext * > * availableContexts;

  /// Returns the named context. Create it if missing.
  static CommandContext * namedContext(const QString & name,
                                       const QString & prefix,
                                       const QString & rubyClass);

  /// The commands available in the context
  QHash<QString, Command *> commands;

  friend class Command;


  /// The prefix for the commands. When this is not empty, the command
  /// can either be given as named, or as prefix+command.
  QString prefix;

  /// The Ruby class name
  QString rubyClass;

  void crosslinkCommands();

  /// Writes the specs of the context
  void writeSpec(QTextStream & out, bool full);
public:

  static void crosslinkAllCommands();

  static bool finishedLoading;

  /// Loads the documentation from the given string, and returns a
  /// list of commands for which the documentation was missing.
  ///
  /// Loads for all the contexts
  static QStringList loadDocumentation(const QString & str);


  /// Writes out a specification for all commands, in the alphabetic
  /// order.
  static void writeSpecFile(QTextStream & out, bool full);


  CommandContext(const QString & prefix, const QString & cls);

  /// Registers the given command within the context
  void registerCommand(Command * command);

  /// Unregisters the given command.
  void unregisterCommand(Command * command);

  /// Returns the names of all the commands registered in this
  /// context, including aliases
  QStringList allCommands() const;


  /// Returns the list of all the command names, prefixed with the
  /// correct prefix
  static QStringList listAllCommands();
  
  QStringList interactiveCommands() const;

  QStringList nonInteractiveCommands() const;

  static QStringList allNonInteractiveCommands();

  /// Returns the list of the commands available in the context
  QSet<Command *> availableCommands() const;

  /// Returns the list of all the commands available
  static QSet<Command *> allAvailableCommands();

  /// Returns the named command, or NULL if there is no command of
  /// that name.
  ///
  /// If @a rubyConversion is true, then underscores are converted to
  /// dashes.
  Command * namedCommand(const QString & name,
                         bool rubyConversion = false) const;


  /// Runs a command from the command prompt, already split into
  /// words. The first word is therefore the command name.
  void runCommand(const QStringList & args, 
                  QWidget * base = NULL);

  /// @name Available contexts
  ///
  /// @{
  
  /// Returns the global context, i.e. the context for global commands
  static CommandContext * globalContext();

  /// Returns the context for fit commands
  static CommandContext * fitContext();

  /// @}
};

#endif
