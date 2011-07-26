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
class CommandEffector;

/// An abstract class representing a command. All commands will be
/// instances of children of this class, either instances of generic
/// classes or derived classes written explicitly.
///
/// @todo Maybe there should be a way to enable/disable the actions,
/// using proper virtual functions ? The question is: when should
/// their status be requested again ?
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

  /// The effector, ie the code that will actually run the command.
  CommandEffector * effector;

  /// The group to which this command belongs.
  Group * group;


  /// Specifies the various elements linked to the Command.
  ///
  /// \warning Command doesn't take ownership of the three last
  /// strings, which should therefore point to locations that will not
  /// move, ideally constant strings.
  ///
  /// @todo this is cumbersome to mix description and code.
  Command(const char * cn, 
          CommandEffector * eff,
          const char * gn, 
          const char * pn,
          const char * sd = "", 
          const char * ld = "", 
          const char * sc = "", 
          bool autoRegister = true) : 
    cmdName(cn), shortCmdName(sc), pubName(pn), 
    shortDesc(sd), longDesc(ld), groupName(gn), 
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
  ///
  /// This function possibly can prompt for missing arguments, if base
  /// isn't NULL.
  virtual void runCommand(const QString & commandName, 
                          const QStringList & arguments,
                          QWidget * base = NULL);

  /// Runs a command from the command prompt, already split into
  /// words. The first word is therefore the command name.
  static void runCommand(const QStringList & args, 
                         QWidget * base = NULL);

  /// Returns an action for this Command parented by the given parent.
  virtual QAction * actionForCommand(QObject * parent) const;
};

#endif
