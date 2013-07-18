/*
  datasetoptions.cc: implementation of DatasetOptions
  Copyright 2013 by Vincent Fourmond

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
#include <datasetoptions.hh>
#include <dataset.hh>

DatasetOptions::DatasetOptions() :
  yErrors(-1)
{
}

bool DatasetOptions::hasYErrors() const
{
  return yErrors >= 0;
}


//////////////////////////////////////////////////////////////////////

QDataStream & operator<<(QDataStream & out, const DatasetOptions & ds)
{
  out << ds.yErrors;
  
  return out;
}


QDataStream & operator>>(QDataStream & in, DatasetOptions & ds)
{
  in >> ds.yErrors;
  // if(DataStack::serializationVersion >= 1)
  return in;
}
