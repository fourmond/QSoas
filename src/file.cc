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

#include <QFile>

File::File(const QString & fn, OpenModes m,
           const CommandOptions & opts) : fileName(fn),
                                          rotations(5),
                                          device(NULL), mode(m)
{
  if(opts.contains("overwrite")) {
    bool ov;
    updateFromOptions(opts, "overwrite", ov);
    mode.setFlag(PromptOverwrite, !ov);
  }
  if(opts.contains("rotate")) {
    updateFromOptions(opts, "rotate", rotations);
    if(rotations > 0) {
      mode = mode & ~IOMask;
      mode = mode | RotateMode;
    }
    else {
      if(mode & IOMask == RotateMode) {
        // Switch to append ?
        mode = mode & ~IOMask;
        mode = mode | AppendMode;
      }
    }
  }
}

void File::preOpen()
{
  if(mode & IOMask == RotateMode) {
  }
  if(mode & PromptOverwrite) {
    Utils::confirmOverwrite(fileName);
  }
}

void File::open()
{
  preOpen();
  if(device)
    throw InternalError("Trying to open an already opened File");
  QIODevice::OpenMode m;
  if(mode & IOMask == ReadOnlyMode)
    m = QIODevice::ReadOnly;
  else {
    m = QIODevice::WriteOnly;
    if(mode & IOMask == AppendMode)
      m |= QIODevice::Append;
  }
  if(mode & Text == Text)
    m |= QIODevice::Text;
  std::unique_ptr<QFile> f(new QFile(fileName));
  if(!f->open(m)) {
    QString error = f->errorString();
    throw RuntimeError("Could not open file %1: %2").
      arg(fileName).arg(error);
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

File::operator QIODevice*()
{
  if(! device) {
    open();
    if(! device)
      throw InternalError("File opened but NULL nevertheless");
  }
  return device;
}
