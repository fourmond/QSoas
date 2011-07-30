/*
  datastack.cc: implementation of the DataStack class
  Copyright 2011 by Vincent Fourmond

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
#include <datastack.hh>
#include <terminal.hh>


DataStack::DataStack()
{
}

DataStack::~DataStack()
{
  /// @todo free buffers. Create a dedicated function.
}

void DataStack::pushDataSet(DataSet * dataset)
{
  dataSets << dataset;
  dataSetByName[dataset->name] = dataset;
}

void DataStack::showStackContents(bool mostRecentFirst) const
{
  int size = dataSets.size();
  int totalSize = 0;
  for(int i = 0; i < size; i++) {
    int j = (mostRecentFirst ? size-i-1 : i);
    int nb = (size - j - 1);
    Terminal::out << "#" << nb << ": "
                  << dataSets[j]->stringDescription() << endl;
    totalSize += dataSets[j]->size();
  }
  Terminal::out << "Total size: " << totalSize/1024 << " kB" << endl;
}
