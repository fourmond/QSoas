/*
  filelock.cc: implementation of FileLock
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
#include <filelock.hh>

#ifdef Q_OS_LINUX
#include <sys/file.h>
#endif

FileLock::FileLock(QFile * tg) :
  target(tg)
{
#ifdef Q_OS_LINUX
  int fd = target->handle();
  int rv = flock(fd,LOCK_EX);
  // QTextStream o(stdout);
  // o << "Locking: " << fd << " -> " << rv << endl;
#endif
}

FileLock::~FileLock()
{
#ifdef Q_OS_LINUX
  int fd = target->handle();
  int rv = flock(fd,LOCK_UN);
  // QTextStream o(stdout);
  // o << "Unlocking: " << fd << " -> " << rv << endl;
#endif
}
