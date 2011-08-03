/**
   \file terminal.hh
   A thin wrapper around QTextStream to write data to the terminal
   Copyright 2010, 2011 by Vincent Fourmond

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


#ifndef __TERMINAL_HH
#define __TERMINAL_HH

/// This namespace contains all classes and utility functions
/// necessary to interact with the terminal. All text should go to the
/// terminal.
///
/// @todo I can think of two ways to do formatting:
/// @li use the generic
namespace Terminal {

  /// This class is a thin wrapper around a QTextStream that
  /// automatically sends lines to the terminal.
  class TextStream {

    /// The internal stream used to build up the string to be forwarded
    /// to Log.
    QTextStream * internalStream;

    /// A temporary buffer string.
    QString buffer;

    /// Flushes to the terminal.
    void flushToTerminal();
  public:

    TextStream();
    ~TextStream();

    template<typename T> TextStream & operator<<(const T& t) {
      (*internalStream) << t;
      return *this;
    };

    TextStream & operator<<(QTextStreamFunction t);

  };

  /// An alway open TextStream
  extern TextStream out;

  /// @name Formatting functions
  ///
  /// A whole bunch of formatting functions used in conjuction with
  /// TextStream
  ///
  /// @todo Formatting is disabled for now, so long as I haven't found
  /// out a way to pass it around.
  ///
  /// @{

  inline QString bold(const QString & str) {
    // return QString("<b>%1</b>").arg(str);
    return str;
  };

  /// @}

};


#endif
