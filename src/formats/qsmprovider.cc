/*
  qsmprovider.cc: meta-data provider for QSoas-edited meta-data
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
#include <metadataprovider.hh>
#include <metadatafile.hh>

#include <terminal.hh>

/// The class that reads a meta-data
class QSMProvider : public MetaDataProvider {
protected:


public:

  /// Whether or not the provider has meta-data for the give file.
  virtual bool handlesFile(const QString & fileName) const {
    return true;
  };

  /// Returns the meta-data for the given file
  virtual ValueHash metaDataForFile(const QString & fileName) const {
    MetaDataFile f(fileName);
    f.read();
    return f.metaData;
  };
  
  QSMProvider() : MetaDataProvider("QSM")
  {
  };

  
};


static QSMProvider provider;
