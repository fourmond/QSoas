/**
   \file hook.hh
   Easy-to-setup hooks
   Copyright 2014 by CNRS/AMU

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
#ifndef __HOOK_HH
#define __HOOK_HH

/// Hooks
///
/// @todo Several hook location (at window startup, somewhere else ?)
/// @todo Name them ?
class Hook {
protected:

  std::function<void ()> function;

  /// The overall list of hooks
  static QList<Hook *> * hooks;
  
  /// Register a Hooks object globally.
  static void registerHook(Hook * hook);

public:
  
  Hook(const std::function<void ()> & fnc);

  static void runHooks();
};


#endif
