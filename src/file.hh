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
#include <fileinfo.hh>

class Argument;

/// This class handles the details of opening/closing a file, offering
/// the following capacities:
/// @li prompt for overwriting
/// @li file rotation
/// @li file (soft) lock (@todo)
/// @li error handling
/// @li redirecting from within already read files if applicable (i.e. the LaTeX style "file definition" @todo)
/// @li automatic deletion of this object if the QIODevice has been deleted too ?? This would require making it a QObject.
/// @li finding a writable place to write the file to
/// @li rename at write (@todo)
/// @li transparent reading of ZIP files. But it should not transparently
/// @b Write ZIP files I guess.
class File {


  /// The file name, which is the actual name of the file we're
  /// "interested in", but may be different from the file being
  /// written to (actualName) and the originalFileName, which is the
  /// one that was specified as an argument (before tilde expansion
  /// and other expansions).
  QString fileName;

  /// Verbatim the argument of the constructor
  QString originalFileName;


  /// @name Move
  ///
  /// These are only for move-at-close mode
  /// @{

  /// The current directory at the time of the opening
  QDir dirAtOpening;

  /// The name of the actual file, in case this is different
  QString actualName;


  /// @}

  /// The number of rotations, when applicable
  int rotations;

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
    Binary = 0x00,
    /// Overwrite mode
    NeverOverwrite = 0x00,
    PromptOverwrite = 0x10,
    AlwaysOverwrite = 0x20,
    RequirePresent = 0x30,
    OverwriteMask = 0x30,
    /// Expands tilde in file name
    /// @todo Implement ExpandTilde
    ExpandTilde = 0x40,
    /// Automatically create parents when necessary.
    MkPath = 0x80,
    /// Implement locking
    Locked = 0x100,
    /// Default read-write shortcuts
    BinaryRead = ReadOnlyMode|ExpandTilde,
    BinaryWrite = SimpleWriteMode|ExpandTilde|PromptOverwrite,
    BinaryOverwrite = SimpleWriteMode|ExpandTilde|AlwaysOverwrite,
    TextRead = BinaryRead|Text,
    TextWrite = BinaryWrite|Text,
    TextOverwrite = BinaryOverwrite|Text
  };

  Q_DECLARE_FLAGS(OpenModes,OpenMode);


  
private:

  /// The various opening modes for the file.
  OpenModes mode;

  /// @name Dependency tracking
  ///
  /// @{

  /// The list of full path of files read so far. The file paths are
  /// absolute.
  static QStringList * filesRead;

  /// The list of files written so far. The number corresponds to the
  /// size of filesRead at the time of their creation. The file paths
  /// are absolute.
  static QHash<QString, int> * filesWritten;

  /// Adds the tracking information of the given file. Absolute file
  /// path used.
  static void trackFile(const QString & path, OpenModes m);

public:

  /// Write a Makefile file for the dependencies
  ///
  /// @b Note: this function can be called with std::atexit because
  /// the global variables filesRead and filesWritten are NOT
  /// DESTROYED.
  ///
  /// It could also be a custom destroyable object.
  static void writeDependencies(const QString & outputFile);

  /// @}
  

  /// Constructs a file with the given mode, taking into account the
  /// options given in @a opts.
  ///
  /// @todo Maybe another constructor with a string for errors ?
  ///
  /// By default, the options are viable
  File(const QString & fn, OpenModes mode,
       const CommandOptions & opts = CommandOptions());

  ~File();

  /// Does the pre-open manipulations, i.e. rotation, and check for
  /// overwriting. Do not call both preOpen and open()
  void preOpen();

  /// Opens the file. This should most often not be done explicitly,
  /// but rather through the first call to the QIODevice automatic
  /// cast.
  void open();

  /// Closes the file. Like open(), this will be done automatically at
  /// deletion.
  void close();

  /// Returns the underlying device. Never NULL.
  QIODevice * ioDevice();

  /// Return a FileInfo to get information about the file.  It does
  /// not require that the file is open, and will not open a yet
  /// unopened file.
  FileInfo info() const;

  /// This allows the use of a File where a QIODevice would
  /// do. Automatically calls open().
  operator QIODevice*();

  /// These are options to handle the generation of options.
  enum Option {
    OverwriteOption = 0x1,
    RotationOption = 0x2,
    MkPathOption = 0x4
  };

  Q_DECLARE_FLAGS(Options,Option);

  /// Returns the arguments suitable for opening files
  static QList<Argument *> fileOptions(Options options);

  /// Convenience function to read a file in one go
  static QByteArray readFile(const QString & fileName, bool text = true);

  /// Finds the first file following the "printf-like" specification
  /// in @a base, which will be given an @b int. By default,
  /// findFreeFile tries each number until a non-existing one is
  /// found. Optionnally, the int given can be random.
  static QString findFreeFile(const QString & base, bool random = false);


  /// Performs all the checks/operations done in preOpen(), including:
  /// @li tilde expansion
  /// @li overwriting checks
  /// @li ??
  ///
  /// Returns the name of the file that should actually be
  /// read/written to
  static QString checkOpen(const QString & fileName,
                           const CommandOptions & opts,
                           OpenModes mode = OpenModes(SimpleWriteMode)|PromptOverwrite);

  /// Lists the contents of the given directory
  static QList<FileInfo> listDirectory(const QString & directory);

};

Q_DECLARE_OPERATORS_FOR_FLAGS(File::OpenModes)
Q_DECLARE_OPERATORS_FOR_FLAGS(File::Options)

#endif
