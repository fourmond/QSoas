/*
  zipfile.cc: handling of ZIP files
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
#include <zipfile.hh>

#include <exceptions.hh>

ZipFile::ZipFile(const QString & file) :
  zipFile(NULL), zipPath(file)
{
}

ZipFile::~ZipFile()
{
  if(zipFile)
    zip_discard(zipFile);
  zipFile = NULL;
}

void ZipFile::openArchive()
{
  if(zipFile)
    return;
  int err = 0;
  zipFile = zip_open(zipPath.toLocal8Bit().constData(),
                     ZIP_RDONLY, &err);
  if(! zipFile)
    throw RuntimeError("Error opening zip file '%1': %2").
      arg(zipPath).arg(err);
}

QStringList ZipFile::fileNames()
{
  openArchive();
  if(! zipFile)
    throw InternalError("Somehow not using an initialized ZIP file");
  zip_int64_t nb = zip_get_num_entries(zipFile, 0);
  if(nb < 0)
    throw InternalError("Negative number of files ?");
  QStringList rv;
  for(zip_int64_t i = 0; i < nb; i++) {
    const char * name = zip_get_name(zipFile, i, 0);
    if(! name)
      throw RuntimeError("We'll figure that out later");
    rv << name;
  }
  return rv;
}

void ZipFile::throwError()
{
  if(! zipFile)
    throw InternalError("Somehow not using an initialized ZIP file");
  throw RuntimeError("Error in zip file: %1 -- %2").
    arg(zipPath).arg(zip_strerror(zipFile));
}

/// A QIODevice subclass for reading the contents of an archive
class ZipIO : public QIODevice {
  zip_file_t * file;
  QSharedPointer<ZipFile> zip;
public:
  ZipIO(ZipFile * z, zip_file_t * fl) : file(fl), zip(z) {
    
  };

  ~ZipIO() {
    zip_fclose(file);
  };
    

  virtual qint64 readData(char *data, qint64 maxSize) override {
    return zip_fread(file, data, maxSize);
  };
  
  virtual qint64 writeData(const char *data, qint64 maxSize) override {
    throw InternalError("Really, this shouldn't be called");
    return -1;
  };
};



QIODevice * ZipFile::openFile(const QString & file)
{
  openArchive();
  if(! zipFile)
    throw InternalError("Somehow not using an initialized ZIP file");
  zip_file_t * fl = zip_fopen(zipFile, file.toUtf8(), 0);
  if(! fl)
    throwError();
  return new ZipIO(this, fl);
}

QCache<QString, QSharedPointer<ZipFile> > * ZipFile::cachedArchives = NULL;

QSharedPointer<ZipFile> ZipFile::openArchive(const QString & path)
{
  QFileInfo info(path);
  if(! info.exists())
    throw RuntimeError("File '%1' does not exist").arg(path);

  QString ap = info.canonicalFilePath();
  if(! cachedArchives) {
    cachedArchives = new QCache<QString, QSharedPointer<ZipFile> >(20);
  }
  if(! cachedArchives->contains(ap)) {
    std::unique_ptr<ZipFile> fl(new ZipFile(ap));
    fl->openArchive();
    cachedArchives->insert(ap, new QSharedPointer<ZipFile>(fl.release()));
  }
  return *(*cachedArchives)[ap];
}


bool ZipFile::isZIP(const QString & path)
{
  QFileInfo info(path);
  if(! info.exists())
    return false;
  if(info.isDir())
    return false;

  if(cachedArchives && cachedArchives->contains(info.canonicalFilePath()))
    return true;                // already cached

  int err = 0;
  zip_t * zip = zip_open(path.toLocal8Bit().constData(),
                         ZIP_RDONLY, &err);
  if(! zip)
    return false;
  zip_discard(zip);
  return true;
}

QPair<QString, QString> ZipFile::separatePath(const QString & path)
{
  QFileInfo info(path);
  if(info.exists())
    return QPair<QString, QString>("", path);

  QString rhs;
  while(true) {
    QString parent = info.path();
    if(rhs.size() > 0)
      rhs = "/" + rhs;
    rhs = info.fileName() + rhs;
    info = QFileInfo(parent);
    if(info.exists()) {
      if(isZIP(parent))
        return QPair<QString, QString>(parent, rhs);
      else
        return QPair<QString, QString>("", path);
    }
    if(info.path() == parent)
      return QPair<QString, QString>("", path);
  }
  return QPair<QString, QString>("", path);
}


QString ZipFile::archiveForFile(const QString & path)
{
  return ZipFile::separatePath(path).first;
}

QIODevice * ZipFile::openFileInArchive(const QString & path)
{
  QPair<QString, QString> spec = separatePath(path);
  if(spec.first.isEmpty())
    throw InternalError("For some reason believed that '%1' was in a ZIP archive").arg(path);

  QSharedPointer<ZipFile> archive = openArchive(spec.first);
  return archive->openFile(spec.second);
}


//////////////////////////////////////////////////////////////////////

#include <commandlineparser.hh>


static void isZip(const QStringList & args)
{
  const QString & fl = args.first();
  bool zip = ZipFile::isZIP(fl);
  QTextStream o(stdout);
  o << "File '" << fl << "' "
    << (zip ? "is" : "isn't") << " a ZIP file" << endl;
  ::exit(0);
}

static CommandLineOption zc("--zip-check", &::isZip, 1, "test: is zip file");

// static void separateZip(const QStringList & args)
// {
//   const QString & fl = args.first();
//   QPair<QString, QString> sep = ZipFile::separatePath(fl);
//   QTextStream o(stdout);
//   o << "Path '" << fl << "' -> "
//     << sep.first << "##" << sep.second << endl;
//   ::exit(0);
// }

// static CommandLineOption zs("--zip-sep", &::separateZip, 1, "test: is zip file");


// static void readZip(const QStringList & args)
// {
//   QString file = args[0];
//   ZipFile zip(file);
//   QTextStream o(stdout);
//   o << "Reading file: " << file << endl;
//   for(const QString & s : zip.fileNames())
//     o << " * " << s << "\n";
//   o << endl;
//   ::exit(0);
// }

// static CommandLineOption zl("--zip-lst", &::readZip, 1, "test: read zip");

// static void readZipFile(const QStringList & args)
// {
//   QString file = args[0];
//   ZipFile zip(file);
//   QString fn = args[1];
//   {
//     std::unique_ptr<QIODevice> dev(zip.openFile(fn));
//     dev->open(QIODevice::ReadOnly);
//     QFile out(fn);
//     out.open(QIODevice::WriteOnly);
//     out.write(dev->readAll());
//   }
//   ::exit(0);
// }

// static CommandLineOption zr("--zip-extract", &::readZipFile, 2, "test: read zip");
