/**
   \file commandeffector.hh
   The core command handling for QSoas
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
#ifndef __COMMANDEFFECTOR_HH
#define __COMMANDEFFECTOR_HH

#include <argumentlist.hh>
#include <argumentmarshaller.hh>

class CurveEventLoop;

/// This abstract base class serves as the base for all the code for
/// the commands.
class CommandEffector {
  
  bool interactive;
public:

  CommandEffector(bool inter) : interactive(inter) {;};
  
  /// Runs the command with the given \b typed arguments.
  ///
  /// This function must be reimplemented by children, and is run
  /// internally.
  virtual void runCommand(const QString & commandName, 
                          const CommandArguments & arguments,
                          const CommandOptions & options) = 0;

  /// Runs the command using an event loop
  virtual void runWithLoop(CurveEventLoop & loop,
                           const QString & commandName, 
                           const CommandArguments & arguments,
                           const CommandOptions & options);

  /// Whether or not the effector needs an event loop
  virtual bool needsLoop() const {
    return false;
  };

  /// If this returns true, the command should be considered an
  /// interactive one.
  virtual bool isInteractive() const {
    return needsLoop() || interactive;
  };

};

#endif
