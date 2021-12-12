/**
   \file terminal.hh
   A thin wrapper around QTextStream to write data to the terminal
   Copyright 2010, 2011 by Vincent Fourmond
             2012, 2016 by CNRS/AMU

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
#ifndef __TERMINAL_HH
#define __TERMINAL_HH

/// This class embeds all the interaction with the Terminal.
class Terminal {


  /// The internal stream used to build up the string to be forwarded
  /// to Log.
  QTextStream * internalStream;

  /// A temporary buffer string.
  QString buffer;

  /// Flushes to the terminal.
  void flushToTerminal();

  /// A list of spies, i.e. streams that get a copy of the text sent
  /// to the terminal.
  QList<QTextStream *>  spies;

  /// A list of IO devices owned by the object. They are taken
  /// ownership of when using addSpy with a QIODevice argument
  QList<QIODevice *> ownedDevices;

  /// We maintain two cursors: an "append" cursor that will always be
  /// used for text insert, and a "delete" cursor that will be at the
  /// beginning, slowly deleting stuff to avoid accumulation of too
  /// many lines.

  /// The append cursor
  QTextCursor * appendCursor;

  /// The delete cursor.
  QTextCursor * deleteCursor;

  /// Sets up the cursors for the first time.
  void initializeCursors();

  QTextCharFormat currentFormat;

  /// The total number of lines since beginning
  int totalLines;

  /// The number of deleted lines
  int deletedLines;
  
  
public:

  Terminal();
  ~Terminal();

  template<typename T> Terminal & operator<<(const T& t) {
    (*internalStream) << t;
    return *this;
  };

  Terminal & operator<<(QTextStreamFunction t);

  Terminal & operator<<(Terminal & fnc(Terminal &) );

  /// Add a spy to the stream. Terminal takes ownership of the spy.
  void addSpy(QTextStream * spy);

  /// Add a spy to the stream. Terminal takes ownership of the spy.
  void addSpy(QIODevice * spy);

  /// An alway open TextStream
  static Terminal out;

  /// @name Formatting functions
  ///
  /// std::endl-like manipulators
  ///
  /// Specifiers cancel each other, unless they follow immediately
  /// each other.
  ///
  /// They last until the next use of the flush stream function,
  /// either via directly flush or via endl.
  ///
  /// @{

  /// Sets bold until flush.
  void setBold();

  /// Sets to italic until flush
  void setItalic();

  /// Sets the color of the font
  void setColor(const QColor & col);

  /// Sets the background color of the font
  void setBackgroundColor(const QColor & col);

  /// Sets the background to white or gray depending on the parity of
  /// the total number of lines.
  void setAlternateBackground();

  /// Sets to bold until flush or until another specifier
  static Terminal & bold(Terminal & term);

  /// Sets the text to red until flush
  static Terminal & red(Terminal & term);

  /// Sets to italics until flush or until another specifier
  static Terminal & italic(Terminal & term);


  /// Manipulator corresponding to  setAlternateBackground();
  static Terminal & alternate(Terminal & term);

  /// @}

};


#endif
