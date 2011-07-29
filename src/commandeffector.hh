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

#ifndef __COMMANDEFFECTOR_HH
#define __COMMANDEFFECTOR_HH

#include <argumentlist.hh>
#include <argumentmarshaller.hh>

/// This abstract base class serves as the base for all the code for
/// the commands.
class CommandEffector {
public:
  /// Runs the command with the given \b typed arguments.
  ///
  /// This function must be reimplemented by children, and is run
  /// internally.
  virtual void runCommand(const QString & commandName, 
                          const CommandArguments & arguments,
                          const CommandOptions & options) = 0;

  

  /// A series of type-safe function calls to member functions
  /// automatically converting arguments and checking the number of
  /// arguments.
  template<class C, typename A1> void 
  runFunction(void (C::*f)(const QString &, A1), 
              const QString & name, 
              const QList<ArgumentMarshaller *> & arguments);

  /// @name Utility functions
  /// 
  /// A series of overloaded functions returning appropriate
  /// CommandEffector children wrappers.
  ///
  /// \warning C++ templating mechanism is such that you have to pass
  /// arguments BY VALUE and not by reference to the callbacks
  /// provided to these functions.
  /// 
  /// @{
  static CommandEffector * functionEffectorOptionLess(void (*f)(const QString &)); 

  template<class A1> static CommandEffector * functionEffectorOptionLess(void (*f)(const QString &, A1));

  template<class A1> static CommandEffector * functionEffector(void (*f)(const QString &, A1, const CommandOptions &));

  /// @}
};

#endif
