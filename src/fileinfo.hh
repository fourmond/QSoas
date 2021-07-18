/**
   \file fileinfo.hh
   A wrapper around QFileInfo also handling special files (within ZIP for instance)
   Copyright 2021 by CNRS/AMU

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
#ifndef __FILEINFO_HH
#define __FILEINFO_HH

#ifdef HAS_LIBZIP
#include <zipfile.hh>
#endif

/// A wrapper around QFileInfo that provides also handling of ZIP files.
///
/// QFileInfo should not be used anywhere else than in the File and
/// FileInfo handling code.
class FileInfo {
  /// Path given as argument
  QString originalPath;

  /// The two components of the zip/within zip thing
  mutable QString zipArchive, subPath;

  /// The underlying QFileInfo object.
  QFileInfo info;

#ifdef HAS_LIBZIP
#include <zipfile.hh>

  /// The storage place for the zip_stat info
  mutable struct zip_stat stat;
#endif

  /// Does the splitting of the names and also the stat.
  void doSplit() const;

  FileInfo(const QString & archive, const QString & sub);

  friend class File;
public:

  FileInfo(const QString & file);

  /// Returns the last modification time
  QDateTime lastModified() const;

  /// Returns the size of the file
  qint64 size() const;

  /// Return true if the target exists
  bool exists() const;

  /// Return true if the target is a directory.
  ///
  /// @todo Think about what one can do with ZIP files
  bool isDir() const;

  /// Returns the canonical file path, stripped of redundant path
  /// elements and symlinks.
  QString canonicalFilePath() const;


  /// @name File name manipulation
  ///
  /// These functions only perform file path manipulation, regardless
  /// of whether the file exists or not.
  ///
  /// @{

  /// The absolute path of the file.
  /// @b Note: only manipulates file names, does not care about ZIP and so on
  QString absoluteFilePath() const;

  /// The absolute path of the directory containing the file
  /// @b Note: same as absoluteFilePath()
  QString absolutePath() const;

  /// The name of the file
  /// @b Note: same as absoluteFilePath()
  QString fileName() const;

  /// The base name of the file, excluding extensions.
  /// @b Note: same as absoluteFilePath()
  QString baseName() const;

  /// The suffix of the file name
  /// @b Note: same as absoluteFilePath()
  QString suffix() const;

  /// The path of the file
  /// @b Note: same as absoluteFilePath()
  QString filePath() const;

  /// The path of the directory containing the file
  /// @b Note: same as absoluteFilePath()
  QString path() const;

  /// @}
};

#endif
