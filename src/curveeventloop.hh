/**
   \file curveeventloop.hh
   The CurveEventLoop, to handle small private event loops for user interaction.
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

#ifndef __CURVEEVENTLOOP_HH
#define __CURVEEVENTLOOP_HH

class CurveView;

/// Inner event loop for use with CurveView
///
/// This class permits a relatively easy interaction with the user
/// like in the old Soas (but with a great number of improvements). It
/// gets used this way:
///
///   CurveEventLoop loop;
///
///   while(! loop.finished()) {
///     switch(loop.type()) {
///     case QEvent::MouseMove:
///       // handle mouse move
///       break;
///     case QEvent::KeyPress:
///       //
///       break;
///     default:
///     }
///   }
class CurveEventLoop {

  friend class CurveView;
  CurveView * view;

  /// These functions parse the events
  void processInputEvent(QInputEvent * ie);
  void processKeyEvent(QKeyEvent * event);
  void processMouseEvent(QMouseEvent * event);

  /// While these ones just store them for later use...
  void receiveKeyEvent(QKeyEvent * event);
  void receiveMouseEvent(QMouseEvent * event);

  QList<QInputEvent *> pendingEvents;

  /// Last event attributes
  QEvent::Type lastEventType;
  int k;
  Qt::KeyboardModifiers mods;
  QPoint pos;

  bool done;
public:

  CurveEventLoop(CurveView * v = NULL);
  ~CurveEventLoop();


  /// Signal the loop is finished.
  void terminate() {
    done = true;
  };

  /// Queries the next event, and return true if the loop is finished.
  bool finished();

  /// Returns the type of the last event (mouse press, key press, and
  /// so on...)
  QEvent::Type type() {
    return lastEventType;
  };

  /// Returns the value of the last key typed.
  int key() {
    return k;
  }

  /// Returns the position of the last mouse event, in curve
  /// coordinates
  QPointF position() const;

};


#endif
