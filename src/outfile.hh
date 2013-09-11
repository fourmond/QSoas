/**
   \file outfile.hh
   A thin wrapper around QTextStream to write data to the outfile
   Copyright 2010, 2011, 2013 by Vincent Fourmond

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
#ifndef __OUTFILE_HH
#define __OUTFILE_HH

class ValueHash;

/// A very thin wrapper around a text file stream to handle various
/// details, such as:
/// \li headers
/// \li context ?
class OutFile {
  /// The internal stream used to build up the string to be forwarded
  /// to Log.
  QTextStream * internalStream;

  /// The file name
  QString name;

  /// The full file path
  QString fullFilePath;

  /// The underlying file.
  QFile * output;

  /// Ensures the stream is opened
  void ensureOpened();

  /// The current header.
  QString currentHeader;
public:

  /// If this flag is on, output file is truncated
  bool truncate;

  OutFile(const QString & name);

  /// Change the underlying file
  void setFileName(const QString & name);

  /// Returns the current output file name
  QString fileName() const {
    return name;
  };

  /// Returns the file path
  QString filePath() const {
    return fullFilePath;
  };

  /// Sets a header for the currently output data.
  void setHeader(const QString & head);

  /// The global output file
  static OutFile out;

  /// For some curious reason, endl doesn't work, and "\n" << flush
  /// should be used instead.
  template<typename T> OutFile & operator<<(const T& t) {
    ensureOpened();
    if(internalStream)
      (*internalStream) << t;
    return *this;
  };


  ~OutFile();

  /// Whether the output file currently is opened or not.
  bool isOpened() const;

  /// Writes a full ValueHash to the output file, making sure the
  /// headers are set correctly.
  ///
  /// @warning The only headers we're interested in are the ones in
  void writeValueHash(const ValueHash & hsh);


};


#endif
