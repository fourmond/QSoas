/**
   \file zipfile.hh
   Reading (and writing ?) of ZIP archives
   Copyright 2020, 2021 by CNRS/AMU

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
#ifndef __ZIPFILE_HH
#define __ZIPFILE_HH

#ifndef HAS_LIBZIP
#error "Should not be used without libzip"
#endif
#include <zip.h>

/// This class represents a ZIP file, opened only for reading for now.
///
/// @todo Use shared pointer semantics for avoiding premature deletion
/// of the Zip File ? Yep.
///
/// @todo This class should not be instanceable by others. Maintain a
/// cache with the currently opened zip files.
class ZipFile : public QSharedData {
  /// The underlying ZIP archive
  struct zip * zipFile;

  /// The path of the ZIP file
  QString zipPath;

  /// The list of "silent" directories, i.e. directories that have no
  /// "stat".
  QSet<QString> silentDirectories;

  /// Opens the archive when it is not open. Or fail.
  void openArchive();

  /// Throws an error corresponding to the currently stored error.
  void throwError();

  ZipFile(const QString & path);

  ZipFile(const ZipFile &) = delete;

  /// A cache "absolute zip file path -> ZipFile"
  static QCache<QString, QExplicitlySharedDataPointer<ZipFile> > * cachedArchives;

  /// Returns the list of all the file names.
  QStringList fileNames();

  /// Lists the contents of the given directory in the archive, not
  /// including the files in the sub directories
  QStringList listDirectory(const QString & directory);

  /// Opens the given file, and returns an appropriate QIODevice,
  /// which still must be opened.
  QIODevice * openFile(const QString & file);


  friend class FileInfo;
  friend class File;

  // For command-line stuff.
  friend void listZip(const QStringList & args);
  friend void readZipFile(const QStringList & args);

private:

  /// Returns the ZipFile corresponding to the given path.
  /// (it is internally converted into an absolute file path);
  /// If @a silent is true, then no exception is raised upon failure.
  static QExplicitlySharedDataPointer<ZipFile> openArchive(const QString & path);



  /// Separates the given path into
  /// @li the path to an archive
  /// @li the path within the archive
  /// The first element is empty if no archive is found.
  ///
  /// The idea is that one can do:
  /// directory/archive.zip/archive_dir/file.dat
  static QPair<QString, QString> separatePath(const QString & path);

  
  /// Returns the information about the file in the archive.
  static bool zipStat(const QString & arch, const QString & file,
                      struct zip_stat * stat);


public:

  ~ZipFile();

  /// Returns the archive path for the given path, or an empty string
  /// if this is not within an archive.
  static QString archiveForFile(const QString & path);

  /// Opens the given file, <b>which must be within an archive</b>.
  /// The returned device can only be opened for @b reading. 
  static QIODevice * openFileInArchive(const QString & path);

  /// Returns true if the given file exists and is a ZIP file.
  static bool isZIP(const QString & path);

  /// Lists the files/directories contained in the archive/directory
  static QStringList listDirectory(const QString & archive,
                                   const QString & directory);

  /// Returns the run-time version of libzip
  static QString libzipVersion();

};


#endif
