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
  // QTextStream o(stdout);
  // o << "Creating zip: " << zipPath << " -> " << this << endl;
}

ZipFile::~ZipFile()
{
  if(zipFile)
    zip_discard(zipFile);
  zipFile = NULL;
  // QTextStream o(stdout);
  // o << "Destroying zip: " << zipPath << " -> " << this << endl;
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
  QExplicitlySharedDataPointer<ZipFile> zip;
public:
  ZipIO(ZipFile * z,
        zip_file_t * fl) : file(fl), zip(z) {
    
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

QCache<QString, QExplicitlySharedDataPointer<ZipFile> > * ZipFile::cachedArchives = NULL;

QExplicitlySharedDataPointer<ZipFile> ZipFile::openArchive(const QString & path)
{
  QFileInfo info(path);
  if(! info.exists())
    throw RuntimeError("File '%1' does not exist").arg(path);

  QString ap = info.canonicalFilePath();
  if(! cachedArchives) {
    cachedArchives = new QCache<QString, QExplicitlySharedDataPointer<ZipFile> >(20);
  }
  if(! cachedArchives->contains(ap)) {
    std::unique_ptr<ZipFile> fl(new ZipFile(ap));
    fl->openArchive();
    cachedArchives->insert(ap, new QExplicitlySharedDataPointer<ZipFile>(fl.release()));
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

  QExplicitlySharedDataPointer<ZipFile> archive = openArchive(spec.first);
  return archive->openFile(spec.second);
}


bool ZipFile::zipStat(const QString & arch, const QString & file,
                      struct zip_stat * stat)
{
  QExplicitlySharedDataPointer<ZipFile> zip = openArchive(arch);
  // zip_stat_init(stat);
  // QTextStream o(stdout);
  // o << "Trying to stat: '" << file << "' in '" << arch << "'" << endl;
  if(zip_stat(zip->zipFile, file.toUtf8(), ZIP_FL_ENC_GUESS, stat)) {
    // Trying a directory
    if(! file.endsWith("/")) {
      QString fl2 = file + "/";
      if(! zip_stat(zip->zipFile, fl2.toUtf8(), ZIP_FL_ENC_GUESS, stat))
        return true;
    }
    stat->valid = 0;
    return false;
  }
  return true;
}

QStringList ZipFile::listDirectory(const QString & directory)
{
  openArchive();
  if(! zipFile)
    throw InternalError("Somehow not using an initialized ZIP file");
  zip_int64_t nb = zip_get_num_entries(zipFile, 0);
  if(nb < 0)
    throw InternalError("Negative number of files in archive '%1'").
      arg(zipPath);
  QStringList rv;
  silentDirectories.clear();
  for(zip_int64_t i = 0; i < nb; i++) {
    const char * name = zip_get_name(zipFile, i, 0);
    if(! name)
      throw InternalError("Entry %1 has no file name in archive %2").
        arg(i).arg(zipPath);
    QString nm(name);
    if(nm.endsWith("/"))
      nm = nm.mid(0, nm.size()-1);
    QFileInfo info(nm);
    if(info.path() == directory)
      rv << info.fileName();

    // Add parents
    QString dir = info.path();
    while(true) {
      if(silentDirectories.contains(dir))
        break;
      silentDirectories.insert(dir);
      info.setFile(dir);
      dir = info.path();
      if(dir == directory)
        rv << info.filePath();
      if(dir.isEmpty())
        break;
      if(dir == ".")
        break;
    }
  }
  return rv;
}

QStringList ZipFile::listDirectory(const QString & archive,
                                   const QString & directory)
{
  QExplicitlySharedDataPointer<ZipFile> zip = openArchive(archive);
  return zip->listDirectory(directory);
}

QString ZipFile::libzipVersion()
{
  return QString(zip_libzip_version());
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


void listZip(const QStringList & args)
{
  QString file = args[0];
  ZipFile zip(file);
  QTextStream o(stdout);
  o << "Reading file: " << file << endl;
  for(const QString & s : zip.fileNames())
    o << " * " << s << "\n";
  o << endl;
  ::exit(0);
}

static CommandLineOption zl("--zip-lst", &::listZip, 1, "test: read zip");

void readZipFile(const QStringList & args)
{
  QString file = args[0];
  ZipFile zip(file);
  QString fn = args[1];
  {
    std::unique_ptr<QIODevice> dev(zip.openFile(fn));
    dev->open(QIODevice::ReadOnly);
    QFile out(fn);
    out.open(QIODevice::WriteOnly);
    out.write(dev->readAll());
  }
  ::exit(0);
}

static CommandLineOption zr("--zip-extract", &::readZipFile, 2, "test: read zip");
