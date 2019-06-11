/**
   \file metadatafile.hh
   A class representing the meta-data for a file
   Copyright 2019 by CNRS/AMU

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
#ifndef __METADATAFILE_HH
#define __METADATAFILE_HH

#include <valuehash.hh>

/// The class represents the mete-data attached to a specific file,
/// whose name is given to the constructor. The meta-data are stored
/// in a separated file, with the suffix .qsm. This
class MetaDataFile {
  QString fileName;

  QString metaDataFileName;

public:
  MetaDataFile(const QString & file);

  ValueHash metaData;

  /// Reads the information into the metaData hash. By default,
  /// parsing errors are silently ignored, unless \a silent is false.
  void read(bool silent = true);

  /// Writes the information from the metaData hash
  void write() const;

  /// Returns the file name for the given data file
  static QString metaDataForFile(const QString & fileName);

  /// Reads a meta-data file and returns the JSON for it.
  /// Stores the version number of the format in version
  static QString readMetaDataFile(const QString & fileName, QString * version);

  /// Returns true if the target file exists and is a meta-data file
  static bool isMetaDataFile(const QString & fileName);

};
#endif
