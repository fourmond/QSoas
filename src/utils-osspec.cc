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


#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
#include <sys/time.h>
#include <sys/resource.h>
#endif


#ifdef Q_OS_WIN32
#include <windows.h>
#include <psapi.h>
#endif




// See https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process

int Utils::memoryUsed()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
  struct rusage r;
  if(! getrusage(RUSAGE_SELF, &r)) {
#  ifdef Q_OS_LINUX
    return r.ru_maxrss;         // Linux getrusage says kB
#  else
    return (r.ru_maxrss >> 10); // Mac getrusage says bytes
#  endif
  }
  else {
    // Error of some kind
    return 0;
  }
#endif
#ifdef Q_OS_WIN32
  PROCESS_MEMORY_COUNTERS_EX pmc;
  GetProcessMemoryInfo(GetCurrentProcess(),
                       (PROCESS_MEMORY_COUNTERS*) &pmc, sizeof(pmc));
  if(pmc.PagefileUsage != 0)
    return pmc.PagefileUsage;
  else
    return pmc.PrivateUsage;
#endif
  return 0;
}
