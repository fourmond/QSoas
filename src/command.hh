/**
   \file command.hh
   Command handling for QSoas.
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

#ifndef __COMMAND_HH
#define __COMMAND_HH

class Group;

/// An abstract class representing a command. All commands will be
/// instances of children of this class, either instances of generic
/// classes or derived classes written explicitly.
///
/// @todo Type-safe arguments:
///
/// @li create an Argument class, with information (name, public name,
/// description, and most importantly type).
/// 
/// @li Argument should have virtual functions to prompt
/// (widgets/dialog boxes), possibly returning both a QString and a
/// typed argument ?
///
/// @li Create a typed argument class that can marshall types
///
/// @li Provide a virtual function in Argument that converts a QString
/// to the typed argument
///
/// @li Possibly the reverse (Argument to QString ?) for logging ?
///
/// @li Create an ArgumentList possibly taking a Command pointer to
/// initialize the correct thing there (to avoid huge constuctor
/// arguments).
///
/// @li Add an ArgumentList pointer here, initialized to NULL.
///
/// @li Provide alternative setup functions ?
class Command {
protected:

  QString cmdName;

  QString shortCmdName;

  const char * pubName;

  const char * shortDesc;
  
  const char * longDesc;

  const char * groupName;

 
  /// A global hash holding a correspondance name->command
  ///
  /// @todo This could be turned into (or coupled with) a trie to have
  /// automatic completion ?
  static QHash<QString, Command*> * availableCommands;

  /// Registers the given command to the static registry
  static void registerCommand(Command * cmd);

public:

  /// The group to which this command belongs.
  Group * group;


  /// Specifies the various elements linked to the Command.
  ///
  /// \warning Command doesn't take ownership of the three last
  /// strings, which should therefore point to locations that will not
  /// move, ideally constant strings.
  Command(const char * cn, const char * pn,
          const char * sd = "", const char * ld = "", 
          const char * sc = "", 
          bool autoRegister = true) : 
    cmdName(cn), shortCmdName(sc), pubName(pn), 
    shortDesc(sd), longDesc(ld), group(NULL) {
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
  virtual QString commandName() const {
    return cmdName;
  };

  /// A short command name to be used quickly from the prompt. Most
  /// commands may leave this field empty. This name will not be
  /// translated.
  ///
  /// \warning If you reimplement this function, you should set the
  /// the autoRegister parameter to false and do the registration
  /// yourself.
  virtual QString shortCommandName() const {
    return shortCmdName;
  };


  /// The public name, the one to be used in the menus. This one gets
  /// translated, which means that one should use QT_TRANSLATE_NOOP
  /// macro for setting it.
  virtual QString publicName() const {
    return QObject::tr(pubName);
  };

  /// A short description, typically to be used for the status bar.
  virtual QString shortDescription() const {
    return QObject::tr(shortDesc);
  };

  /// A long informative description, such as a full help text,
  /// possibly with examples too.
  virtual QString longDescription() const {
    return QObject::tr(longDesc);
  };

  /// Categorize the commands within groups. This function \b must be
  /// called at the beginning of main.
  static void crosslinkCommands();

  /// Returns the named command.
  static Command * namedCommand(const QString & cmd);

  /// Runs the command with the given arguments.
  virtual void runCommand(const QString & commandName, 
                          const QStringList & arguments);
};

#endif
