/**
   \file file.hh
   File handling
   Copyright 2020 by CNRS/AMU

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
#ifndef __FILE_HH
#define __FILE_HH

#include <argumentmarshaller.hh>

class Argument;

/// This class handles the details of opening/closing a file, offering
/// the following capacities:
/// @li prompt for overwriting
/// @li file rotation
/// @li file (soft) lock (@todo)
/// @li error handlin
/// @li redirecting from within already read files if applicable (i.e. the LaTeX style "file definition" @todo)
/// @li automatic deletion of this object if the QIODevice has been deleted too ?? This would require making it a QObject.
class File {
  QString fileName;

  /// The underlying device.
  QIODevice * device;

public:

  /// Opening options. Not all combinations make sense.
  enum OpenMode {
    /// The opening mode.
    ReadOnlyMode = 0x01,
    SimpleWriteMode = 0x02,
    AppendMode = 0x03,
    RotateMode = 0x04,
    /// Whether we write to a temporary file we move at closing time
    MoveAtCloseMode = 0x05,
    IOMask = 0x07,
    /// Whether it is a text file
    Text = 0x08,
    PromptOvewrite = 0x10,
    ExpandTilde = 0x20
  };

  Q_DECLARE_FLAGS(OpenModes,OpenMode);


  
private:

  /// The various opening modes for the file.
  OpenModes mode;
  
public:

  /// Constructs a file with the given mode, taking into account the
  /// options given in @a opts.
  ///
  /// @todo Maybe another constructor with a string for errors ?
  File(const QString & fn, OpenModes mode = ReadOnlyMode|Text|ExpandTilde,
       const CommandOptions & opts = CommandOptions());

  /// Does the pre-open manipulations, i.e. rotation, and check for
  /// overwriting. Do not call both preOpen and open()
  void preOpen();

  /// Opens the file. This should most often not be done explicitly,
  /// but rather through the first call to the QIODevice automatic
  /// cast.
  void open();

  /// This allows the use of a File where a QIODevice would
  /// do. Automatically calls open().
  operator QIODevice*();

  /// These are options to handle the generation of options.
  enum Option {
    Overwrite = 0x1,
    Rotation = 0x2
  };

  Q_DECLARE_FLAGS(Options,Option);

  static QList<Argument *> fileOptions(Options & options);

};

Q_DECLARE_OPERATORS_FOR_FLAGS(File::OpenModes)
Q_DECLARE_OPERATORS_FOR_FLAGS(File::Options)

#endif
