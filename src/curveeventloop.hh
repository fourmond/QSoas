/**
   \file curveeventloop.hh
   The CurveEventLoop, to handle small private event loops for user interaction.
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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
#ifndef __CURVEEVENTLOOP_HH
#define __CURVEEVENTLOOP_HH

class CurveView;
class DataSet;
class CurvePanel;
class LineEdit;
#include <command.hh>

/// Inner event loop for use with CurveView
///
/// This class permits a relatively easy interaction with the user
/// like in the old Soas (but with a great number of improvements). It
/// gets used this way:
///
/// \code
///
/// CurveEventLoop loop;
///
/// while(! loop.finished()) {
///   switch(loop.type()) {
///   case QEvent::MouseMove:
///     // handle mouse move
///     break;
///   case QEvent::KeyPress:
///     //
///     break;
///   default:
///   }
/// }
///
/// \endcode
///
/// @todo I should add a way to easily customize the message shown on
/// the status bar during the even loop.
class CurveEventLoop : public QObject {
  Q_OBJECT;

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
  Qt::MouseButton bt;

  bool done;

  LineEdit * prompt;

  bool promptOK;
  QString inputText;

private:

  /// The constructor is private so that only the friend class can
  /// build it (ie we can count the number of functions that need an
  /// event loop...)
  CurveEventLoop(CurveView * v = NULL);

  friend void Command::runCommand(const QString & commandName,
                                  const CommandArguments & arguments,
                                  const CommandOptions & options);

public:

  ~CurveEventLoop();


  /// Signal the loop is finished.
  void terminate() {
    done = true;
  };

  /// Queries the next event, and return true if the loop is finished.
  bool finished();

  /// Returns the type of the last event (mouse press, key press, and
  /// so on...)
  QEvent::Type type() const {
    return lastEventType;
  };

  /// Returns the value of the last key typed.
  int key() const {
    return k;
  }

  /// Returns the last button pressed/released
  Qt::MouseButton button() const {
    return bt;
  };

  CurveView * getView() const {
    return view;
  };

  /// Whether the current action is 'q' 'Q' or middle click, ie the
  /// conventional "accept" event.
  bool isConventionalAccept() const;
  

  /// Returns the position of the last mouse event, in curve
  /// coordinates
  QPointF position(CurvePanel * panel = NULL);

  /// Returns the distance of the current position to the dataset, in
  /// screen units.
  QPair<double, int> distanceToDataSet(const DataSet * ds, 
                                       const QPointF & scale = QPointF());

  /// Sets the help string for the loop (ie the text displayed at the
  /// right of the terminal)
  void setHelpString(const QString & str);
  
  /// Prompts for a string input
  QString promptForString(const QString & prompt, bool * ok = NULL,
                          const QString & init = "");

  /// This function gets installed as application-wide event filter
  /// upon entering the loop.
  virtual bool eventFilter(QObject * watched, QEvent * event);
  
  /// The panel under which the current point is.
  CurvePanel * currentPanel();

  /// Message to display on the status bar. This one is internally
  /// used by PointPicker, but you can use it if you don't use
  /// PointPicker.
  QString ppMessage;

  /// Message to display on the status bar (on the right of the other
  /// one)
  QString message;

  /// Updates the message in the status bar
  void updateMessage();

  /// Whether printing is allowed (default) or not (in the case you
  /// choose to redefine the behavior of the print screen key)
  bool printingAllowed;

protected slots:

  /// Called when the last window of the application was closed, ie
  /// this loop should finish.
  void onLastWindowClosed();
  
};

#endif
