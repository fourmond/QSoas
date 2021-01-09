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


#include <file.hh>
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
    QString ver;
    QString json = readMetaDataFile(metaDataFileName, &ver);
    metaData.fromJSON(json);
  }
  catch(const RuntimeError & er) {
    if(! silent)
      throw RuntimeError("Error reading meta-data file '%1': %2").
        arg(metaDataFileName).arg(er.message());
  }
}

void MetaDataFile::write() const
{
  File r(metaDataFileName, File::TextOverwrite);
  QTextStream out(r);
  out << "// QSoas JSON meta-data -- version 1" << endl;
  out << metaData.toJSON() << endl;
}


QString MetaDataFile::readMetaDataFile(const QString & fileName,
                                       QString * version)
{
  File r(fileName, File::TextRead);
  QTextStream in(r);

  QString s = in.readAll();
  int idx = s.indexOf('{');
  if(idx < 0)
    throw RuntimeError("No opening brace in meta-data file");
  QString header = s.left(idx);
  QRegExp vre("//\\s+QSoas\\s+JSON\\s+.*version\\s*(\\S+)");
  if(vre.indexIn(header) < 0)
    throw RuntimeError("No QSoas signature in meta-data file");
  *version = vre.cap(1);

  return s.mid(idx);
}

bool MetaDataFile::isMetaDataFile(const QString & fileName)
{
  try {
    QString v;
    readMetaDataFile(fileName, &v);
    if(v.isEmpty())
      return false;
    return true;
  }
  catch(const RuntimeError & re) {
    return false;
  }
  return true;
}

QDateTime MetaDataFile::metaDataLastModified(const QString & fileName)
{
  /// @todo Switch to using a File-based class...
  QFileInfo info(metaDataForFile(fileName));
  if(info.exists()) {
    return info.lastModified();
  }
  return QDateTime();
}
