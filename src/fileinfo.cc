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

FileInfo::FileInfo(const QString & archive, const QString & sub) :
  zipArchive(archive), subPath(sub)
{
  originalPath = archive + "/" + subPath;
  info.setFile(originalPath);
  stat.valid = 0;
}

void FileInfo::doSplit() const
{
#ifndef HAS_LIBZIP
  subPath = originalPath;
#else
  if(subPath.isEmpty()) {
    QPair<QString, QString> pair = ZipFile::separatePath(originalPath);
    zipArchive = pair.first;
    subPath = pair.second;
    stat.valid = 0;
  }
  if((! zipArchive.isEmpty()) && (stat.valid == 0)) {
    subPath = QDir::cleanPath(subPath);
    ZipFile::zipStat(zipArchive, subPath, &stat);
  }
#endif
}
  
QDateTime FileInfo::lastModified() const
{
#ifdef HAS_LIBZIP
  doSplit();
  if(! zipArchive.isEmpty()) {
    QDateTime dt;
    if(stat.valid & ZIP_STAT_MTIME)
      dt.setSecsSinceEpoch(stat.mtime);
    return dt;
  }
#endif
  return info.lastModified();
}

qint64 FileInfo::size() const
{
#ifdef HAS_LIBZIP
  doSplit();
  if(! zipArchive.isEmpty()) {
    if(stat.valid & ZIP_STAT_SIZE)
      return stat.size;
    return -1;
  }
#endif
  return info.size();
}

bool FileInfo::exists() const
{
#ifdef HAS_LIBZIP
  doSplit();
  if(! zipArchive.isEmpty())
    return stat.valid;
#endif
  return info.exists();
}

bool FileInfo::isDir() const
{
#ifdef HAS_LIBZIP
  doSplit();
  if(! zipArchive.isEmpty()) {
    if(! (stat.valid & ZIP_STAT_NAME))
      throw InternalError("Somehow could not get the name ?");
    QString n(stat.name);
    return n.endsWith("/");
  }
#endif
  return info.isDir();
}

QString FileInfo::canonicalFilePath() const
{
#ifdef HAS_LIBZIP
  doSplit();
  if(! zipArchive.isEmpty()) {
    QFileInfo zi(zipArchive);
    return zi.canonicalFilePath() + "/" + subPath;
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

