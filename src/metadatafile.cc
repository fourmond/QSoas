/*
  metadatafile.cc: implementation of the MetaDataFile class
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
#include <metadatafile.hh>


#include <utils.hh>


MetaDataFile::MetaDataFile(const QString & file) :
  fileName(file)
{
  metaDataFileName = metaDataForFile(file);
}


QString MetaDataFile::metaDataForFile(const QString & fileName)
{
  return fileName + ".qsm";
}



void MetaDataFile::read(bool silent)
{
  if(! QFile::exists(metaDataFileName))
    return;                     // Exits silently upon missing file
  try {
    QFile r(metaDataFileName);
    Utils::open(&r, QIODevice::ReadOnly);
    QTextStream in(&r);
    metaData.readMetaDataFile(in);
  }
  catch(const RuntimeError & er) {
    if(! silent)
      throw RuntimeError("Error reading meta-data file '%1': %2").
        arg(metaDataFileName).arg(er.message());
  }
}

void MetaDataFile::write() const
{
  QFile r(metaDataFileName);
  Utils::open(&r, QIODevice::WriteOnly);
  QTextStream out(&r);
  metaData.writeMetaDataFile(out);
}
