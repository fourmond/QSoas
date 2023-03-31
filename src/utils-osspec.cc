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
    return pmc.PagefileUsage >> 10;
  else
    return pmc.PrivateUsage >> 10;
#endif
  return 0;
}

void Utils::processorUsed(long * user, long * system,
                          long * vCS, long * iCS)
{
  *user = 0;
  *system = 0;
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
  struct rusage r;
  if(! getrusage(RUSAGE_SELF, &r)) {
    *user = r.ru_utime.tv_sec * 1000 +
      r.ru_utime.tv_usec / 1000;
    *system = r.ru_stime.tv_sec * 1000 +
      r.ru_stime.tv_usec / 1000;
    if(vCS)
      *vCS = r.ru_nvcsw;
    if(iCS)
      *iCS = r.ru_nivcsw;
  }
#endif
#ifdef Q_OS_WIN32
  FILETIME ct, et, kt, ut;
  ULARGE_INTEGER tmp;
  if(GetProcessTimes(GetCurrentProcess(), &ct, &et, &kt, &ut)) {
    tmp.u.LowPart = kt.dwLowDateTime;
    tmp.u.HighPart = kt.dwHighDateTime;
    *system = tmp.QuadPart/10000;
    tmp.u.LowPart = ut.dwLowDateTime;
    tmp.u.HighPart = ut.dwHighDateTime;
    *user = tmp.QuadPart/10000;

    // Unsupported for now
    if(vCS)
      *vCS = 0;
    if(iCS)
      *iCS = 0;
  }
#endif
}
