/**
   \file zipfile.hh
   Reading (and writing) of ZIP archives
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
#ifndef __ZIPFILE_HH
#define __ZIPFILE_HH

#ifndef HAS_LIBZIP
#error "Should not be used without libzip"
#endif
#include <zip.h>

/// This class represents a ZIP file, opened only for reading for now.
class ZipFile {
  /// The underlying ZIP archive
  struct zip * zipFile;

  /// The path of the ZIP file
  QString zipPath;

  /// Opens the archive when it is not open. Or fail.
  void openArchive();
public:

  ZipFile(const QString & path);
  ~ZipFile();

  /// Returns the list of all the file names.
  QStringList fileNames();

  /// @todo Return QIODevice subclass for reading from the
  /// Archive. This would integrate nicely in the File system.
  /// The game will be to properly cache the archive.
  
};


#endif
