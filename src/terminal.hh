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
  /// ownership of when using addSpy with a QIODevice argume,t
  QList<QIODevice *> ownedDevices;
  
public:

  Terminal();
  ~Terminal();

  template<typename T> Terminal & operator<<(const T& t) {
    (*internalStream) << t;
    return *this;
  };

  Terminal & operator<<(QTextStreamFunction t);

  /// Add a spy to the stream. Terminal takes ownership of the spy.
  void addSpy(QTextStream * spy);

  /// Add a spy to the stream. Terminal takes ownership of the spy.
  void addSpy(QIODevice * spy);

  /// An alway open TextStream
  static Terminal out;

  /// @name Formatting functions
  ///
  /// A whole bunch of formatting functions used in conjuction with
  /// TextStream
  ///
  /// @todo Formatting is disabled for now, so long as I haven't found
  /// out a way to pass it around.
  ///
  /// @{

  static inline QString bold(const QString & str) {
    // return QString("<b>%1</b>").arg(str);
    return str;
  };

  /// @}

};


#endif
