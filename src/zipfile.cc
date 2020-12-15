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
                     0, &err);
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





#include <commandlineparser.hh>


static void readZip(const QStringList & args)
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

static CommandLineOption hlp("--zip-lst", &::readZip, 1, "test: read zip");
