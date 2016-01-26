/**
   \file debug.hh
   Code useful for debugging.
   Copyright 2011 by Vincent Fourmond
             2016 by CNRS/AMU

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
#ifndef __DEBUG_HH
#define __DEBUG_HH

/// Class handling all the debugging in QSoas.
///
/// It handles:
/// @li all debug output, written to standard output (anyway), and to
/// a dedicated log file if available
/// @li saving and rotating stacks at each command (and each dataset
/// addition).
class Debug {

  /// The current debug level
  int level;

  /// The current debug directory, or NULL if there isn't such a thing
  ///
  /// This is the directory in which the stacks are saved and the
  /// debug output is.
  QDir * directory;

  /// The underlying output device
  QFile * outputDevice;

  /// The output log file.
  QTextStream * output;

  /// This is the standard output.
  QTextStream out;

  /// The instance of debug.
  static Debug * theDebug;

protected:
  void timeStamp();
  
public:

  Debug();

  /// Returns the static instance
  static Debug & debug();

  /// Returns the current debug level
  static int debugLevel();

  /// Sets up the debug object to work with the given directory.
  ///
  /// If @a directory is empty, then the directory is just closed, and
  /// a new one is not opened.
  ///
  /// If the directory does not exist, it is created.
  void openDirectory(const QString & directory);

  /// Returns the full path of the target directory, or an empty
  /// string if there isn't.
  QString directoryPath() const;

  /// Closes the current directory
  void closeDirectory();

  /// Marks the beginnings of the processing of a command.
  void startCommand(const QStringList & cmdline);

  /// Marks the end of the given command.
  void endCommand(const QStringList & cmdline);

  /// Saves the stack, if the debug level is above 0 and there is a
  /// target directory.
  void saveStack();


  template<class T> Debug& operator<<(T t);
  
  /// @name Helper functions
  ///
  /// A series of useful functions for debugging.
  ///
  /// @{

  /// Writes some information about the widget currently holding the
  /// focus to standard output.
  void dumpCurrentFocus(const QString & str = "Current focus");


  /// Returns a string used to represent a QPointF
  inline QString dumpPoint(const QPointF & point) {
    return QString("x: %1, y: %2").arg(point.x()).arg(point.y());
  };

  /// Returns a string used to represent a QRectF
  inline QString dumpRect(const QRectF & rect) {
    return QString("[%1 to %2, w: %3, h: %4, %5, %6]").
      arg(dumpPoint(rect.topLeft())).
      arg(dumpPoint(rect.bottomRight())).
      arg(rect.width()).arg(rect.height()).
      arg(rect.isValid() ? "valid" : "invalid").
      arg(rect.isEmpty() ? "empty" : "non-empty");
  };

  /// @}
};

template<class T> inline Debug& Debug::operator<<(T t) {
  out << t;
  if(output)
    *output << t;
  return *this;
}

#endif
