/*
  file.cc: implementation of the File class
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
#include <file.hh>
#include <utils.hh>
#include <general-arguments.hh>

#include <QFile>

File::File(const QString & fn, OpenModes m,
           const CommandOptions & opts) : fileName(fn),
                                          rotations(5),
                                          device(NULL), mode(m)
{
  // QTextStream o(stdout);
  // o << "Init, file name: " << fileName << ", mode = " << mode << endl;
  // o << " * " << opts.keys().join(", ") << endl;
  if(opts.contains("overwrite")) {
    bool ov = false;
    updateFromOptions(opts, "overwrite", ov);
    mode = (mode & ~OverwriteMask) | (ov ? AlwayOverwrite : NeverOverwrite);
  }
  if(opts.contains("rotate")) {
    updateFromOptions(opts, "rotate", rotations);
    if(rotations > 0) {
      mode &= ~IOMask;
      mode |= RotateMode;
    }
    else {
      if((mode & IOMask) == RotateMode) {
        // Switch to append ?
        mode &= ~IOMask;
        mode |= AppendMode;
      }
    }
  }
  // o << " -> mode = " << mode << endl;
}

void File::preOpen()
{
  // QTextStream o(stdout);
  // o << "Preopen: " << mode << endl;
  if((mode & IOMask) == RotateMode) {
  }
  if((mode & IOMask) == 0)
    throw InternalError("No IO mode specified: %1").arg(mode);

  // Check overwriting
  if((mode & IOMask) != ReadOnlyMode ) {
    switch(mode & OverwriteMask) {
    case PromptOverwrite:
    case NeverOverwrite:
      if(QFile::exists(fileName)) {
        if((mode & OverwriteMask) == PromptOverwrite) {
          QString s = QObject::tr("Overwrite file '%1' ?").
            arg(fileName);
          if(Utils::askConfirmation(s))
            break;
        }
        throw RuntimeError("Not overwriting file '%1'").arg(fileName);
      }
      break;
    case RequirePresent:
      if(! QFile::exists(fileName))
        throw RuntimeError("File '%1' should be present").arg(fileName);
    default:
      break;
    }
  }
}

void File::open()
{
  if(device)
    throw InternalError("Trying to open an already opened File");
  preOpen();

  QIODevice::OpenMode m;
  if((mode & IOMask) == ReadOnlyMode)
    m = QIODevice::ReadOnly;
  else {
    m = QIODevice::WriteOnly;
    if((mode & IOMask) == AppendMode)
      m |= QIODevice::Append;

    if((mode & OverwriteMask) == NeverOverwrite)
      m |= QIODevice::NewOnly;  // To avoid race conditions ?
  }
  if(mode & Text)
    m |= QIODevice::Text;
  // QTextStream o(stdout);
  // o << "Opening file: " << fileName << " with mode: "
  //   << m <<  " (internal mode: " << mode << ")" << endl;
  std::unique_ptr<QFile> f(new QFile(fileName));
  if(!f->open(m)) {
    QString error = f->errorString();
    QString mdStr = (m & QIODevice::ReadWrite) == QIODevice::ReadOnly ?
      "for reading" : "for writing";
    throw RuntimeError("Could not open file %1 %2: %3").
      arg(fileName).arg(mdStr).arg(error);
  }
  device = f.release();
  // Successful opening !
}

void File::close()
{
  if(device) {
    delete device;
    /// @todo MoveAtClose !

    device = NULL;
  }
}

File::~File()
{
  close();
}

QIODevice * File::ioDevice()
{
  if(! device) {
    open();
    if(! device)
      throw InternalError("File opened but NULL nevertheless");
  }
  return device;
}

File::operator QIODevice*()
{
  return ioDevice();
}

QList<Argument *> File::fileOptions(Options options)
{
  QList<Argument *> rv;
  if(options & OverwriteOption)
    rv  << new BoolArgument("overwrite", 
                            "Overwrite",
                            "If true, overwrite without prompting");
  if(options & MkPathOption)
    rv << new BoolArgument("mkpath",
                           "Make path",
                           "If true, creates all necessary directories");
  return rv;
}
