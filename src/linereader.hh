/**
   \file linereader.hh
   The LineReader class, a utility class to read text files line by line
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
#ifndef __LINEREADER_HH
#define __LINEREADER_HH

/// Reads a file line-by-line. It may sound ridiculous, but it is
/// actually rather painful.
class LineReader {
protected:

  /// The stream we're reading from
  QTextStream * source;

  /// Wether source should be deleted at end.
  bool ownSource;

  QChar getc();

  QChar peekc();


  bool hasPending;
  QChar pending;

  /// Size of the longest line so far
  int longestLine;

public:
  LineReader(QIODevice * dev);
  LineReader(QTextStream * src);
  ~LineReader();

  QString readLine(bool keepCR = false);

  bool atEnd() const;
  
};



#endif
