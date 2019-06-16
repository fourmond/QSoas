/*
  utils-osspec.cc: OS-specific utility functions
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
#include <utils.hh>
#include <exceptions.hh>


// Memory use


#ifdef Q_OS_LINUX
#include <sys/time.h>
#include <sys/resource.h>
#endif

// See https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process

int Utils::memoryUsed()
{
#ifdef Q_OS_LINUX
  struct rusage r;
  if(! getrusage(RUSAGE_SELF, &r)) {
    return r.ru_maxrss;
  }
  else {
    // Error of some kind
    return 0;
  }
#else
  return 0;
#endif
}
