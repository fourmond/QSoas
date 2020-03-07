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
  }
}

