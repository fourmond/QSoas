/*
  backtrace.cc: platform-dependent backtrace reading
  Copyright 2012 by CNRS/AMU

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

#ifdef HAS_EXECINFO
#include <execinfo.h>

QStringList Utils::backtrace(int maxframes)
{
  QVarLengthArray<void *, 100> frames(maxframes);
  ::backtrace(frames.data(), maxframes);
  char ** symbols = ::backtrace_symbols(frames.data(), maxframes);
  QStringList ret;
  for(int i = 0; i < maxframes; i++)
    ret << QString("0x%1: %2").arg((ulong)frames[i], 0, 16).arg(symbols[i]);
  ::free(symbols);
  return ret;
}

#else
QStringList Utils::backtrace(int maxframes)
{
  return QStringList();         // Not implemented here

  // On windows:
  // CaptureStackBackTrace (http://msdn.microsoft.com/en-us/library/windows/desktop/bb204633%28v=vs.85%29.aspx)
  // SymFromAddr (http://msdn.microsoft.com/en-us/library/windows/desktop/ms681323%28v=vs.85%29.aspx)
}

#endif
