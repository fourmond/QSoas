/**
   \file eventhandler.hh
   The EventHandler, a helper to use CurveEventLoop
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
#ifndef __EVENTHANDLER_HH
#define __EVENTHANDLER_HH

class CurveEventLoop;

/// This helper class allows one to transform input from a command
/// loop into a simple number (ie an action, but not the QAction
/// meaning).
///
/// @todo static registration to provide program-wide documentation
/// facilities.
///
/// @todo Handle more complex stuff, such as Ctrl modifiers for clicks
///
/// @question Should this class handle PointPicker ??
///
/// @todo The structure should be extended to handle more complex
/// stuff: maybe a hash Eventtype -> hashes of int to action ? Or more
/// complex filters ? Maybe even event objects (or subclasses ?)
///
///
/// @todo Add commands with "optional" help text, ie things that won't
/// get displayed on the help text unless we ask for it. This may
/// become necessary for interactive commands with many shortcuts.
///
/// @todo Handle double clicks
class EventHandler{
protected:

  /// The command name with which this handler is used. Of no use for
  /// now.
  QString commandName;

  /// Help text for the given action.
  QHash<int, QString> helpTexts;

  /// Actions for the given mouse press
  QHash<Qt::MouseButton, int> clickActions;
  
  /// Actions for the given keys
  QHash<int, int> keyActions;

  /// A string representing the key press
  static QString keyString(int key);

  /// A string representing the mouse click
  static QString clickString(Qt::MouseButton button);
  
public:

  EventHandler(const QString & cmd = "");
  ~EventHandler();


  /// Adds a keypress event to the handler. Help text doesn't override
  /// existing, though you can override a key press handler this way.
  EventHandler & addKey(int key, int action, const QString & help = "");


  /// Adds a mouse click event to the handler
  EventHandler & addClick(Qt::MouseButton button, int action,
                          const QString & help = "");

  /// Adds the action corresponding to a conventional accept.
  EventHandler & conventionalAccept(int action, const QString & help = "");

  /// Builds a help string (in the order of the int of the actions)
  QString buildHelpString(bool useHTML = false) const;

  /// Handles the current loop event, and returns the corresponding
  /// action, or -1 if the event matches no action.
  int nextAction(const CurveEventLoop & loop) const;

};

#endif
