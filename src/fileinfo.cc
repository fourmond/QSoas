/*
  fileinfo.cc: implementation of the FileInfo class
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
#include <fileinfo.hh>

#include <exceptions.hh>


#ifdef HAS_LIBZIP
#include <zipfile.hh>
#endif

#include <QFileInfo>


FileInfo::FileInfo(const QString & file) :
  originalPath(file), info(file)
{
#ifdef HAS_LIBZIP
  stat.valid = 0;
#endif
}

void FileInfo::doSplit() const
{
  if(! subPath.isEmpty())
    return;
#ifdef HAS_LIBZIP
  QPair<QString, QString> pair = ZipFile::separatePath(originalPath);
  zipArchive = pair.first;
  subPath = pair.second;
#else
  subPath = originalPath;
#endif
}
  
QDateTime FileInfo::lastModified() const
{
#ifdef HAS_LIBZIP
  doSplit();
  if(! zipArchive.isEmpty()) {
    throw NOT_IMPLEMENTED;
  }
#endif
  return info.lastModified();
}

bool FileInfo::exists() const
{
#ifdef HAS_LIBZIP
  doSplit();
  if(! zipArchive.isEmpty()) {
    throw NOT_IMPLEMENTED;
  }
#endif
  return info.exists();
}

QString FileInfo::canonicalFilePath() const
{
#ifdef HAS_LIBZIP
  doSplit();
  if(! zipArchive.isEmpty()) {
    throw NOT_IMPLEMENTED;
  }
#endif
  return info.canonicalFilePath();
}


QString FileInfo::absoluteFilePath() const
{
  return info.absoluteFilePath();
}

QString FileInfo::absolutePath() const
{
  return info.absolutePath();
}

QString FileInfo::fileName() const
{
  return info.fileName();
}

QString FileInfo::filePath() const
{
  return info.filePath();
}

QString FileInfo::baseName() const
{
  return info.baseName();
}

QString FileInfo::path() const
{
  return info.path();
}

QString FileInfo::suffix() const
{
  return info.suffix();
}

