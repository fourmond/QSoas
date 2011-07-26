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

/// An abstract class representing a command. All commands will be
/// instances of children of this class, either instances of generic
/// classes or derived classes written explicitly.
class Command {
protected:

  QString cmdName;

  const char * pubName;

  const char * shortDesc;
  
  const char * longDesc;

  
  /// A global hash holding a correspondance name->command
  ///
  /// @todo This could be turned into (or coupled with) a trie to have
  /// automatic completion ?
  static QHash<QString, Command*> * availableCommands;

  /// Registers the given command to the static registry
  static void registerCommand(Command * cmd);

public:

  /// Specifies the various elements linked to the Command.
  ///
  /// \warning Command doesn't take ownership of the three last
  /// strings, which should therefore point to locations that will not
  /// move, ideally constant strings.
  Command(const char * cn, const char * pn,
          const char * sd = "", const char * ld = "", 
          bool autoRegister = true) : 
    cmdName(cn), pubName(pn), shortDesc(sd), longDesc(ld) {
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


};

#endif
