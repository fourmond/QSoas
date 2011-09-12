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
  clear();
}

void DataStack::pushDataSet(DataSet * dataset, bool silent)
{
  dataSets << dataset;
  dataSetByName[dataset->name] = dataset;
  if(! silent)
    emit(currentDataSetChanged());
}

void DataStack::showStackContents(bool /*unused*/) const
{
  int totalSize = 0;
  if(redoStack.size())
    Terminal::out << "Redo stack:" << endl;
  for(int i = -redoStack.size(); i < dataSets.size(); i++) {
    if(! i)
      Terminal::out << "Normal stack:" << endl;
    DataSet * ds = numberedDataSet(i);
    Terminal::out << "#" << i << ": "
                  << ds->stringDescription() << endl;
    totalSize += ds->byteSize();
  }
  Terminal::out << "Total size: " << totalSize/1024 << " kB" << endl;
}

int DataStack::dsNumber2Index(int nb, const QList<DataSet *> ** target) const
{
  if(nb >= 0) {
    *target = &dataSets;
    return dataSets.size() - (nb+1);
  }
  *target = &redoStack;
  return redoStack.size() + nb;
}

int DataStack::dsNumber2Index(int nb, QList<DataSet *> * * target)
{
  if(nb >= 0) {
    *target = &dataSets;
    return dataSets.size() - (nb+1);
  }
  *target = &redoStack;
  return redoStack.size() + nb;
}


DataSet * DataStack::numberedDataSet(int nb) const
{
  const QList<DataSet *> * lst;
  int idx = dsNumber2Index(nb, &lst);
  return lst->value(idx, NULL);
}

DataSet * DataStack::currentDataSet(bool silent) const
{
  DataSet * ds = numberedDataSet(0);
  if(!silent && !ds)
    throw std::runtime_error("Need a buffer !");
  return ds;
}

void DataStack::undo(int nbTimes)
{
  while(nbTimes--) {
    if(dataSets.size() < 2)
      throw std::runtime_error("Undo: nothing to undo");
    redoStack.append(dataSets.takeLast());
    emit(currentDataSetChanged());
  }
}

void DataStack::redo(int nbTimes)
{
  while(nbTimes--) {
    if(! redoStack.size())
      throw std::runtime_error("Redo: nothing to redo");
    dataSets.append(redoStack.takeLast());
    emit(currentDataSetChanged());
  }
}

void DataStack::clear()
{
  for(int i = 0; i < dataSets.size(); i++)
    delete dataSets[i];
  dataSets.clear();
  for(int i = 0; i < redoStack.size(); i++)
    delete redoStack[i];
  redoStack.clear();
  emit(currentDataSetChanged());
}

DataSet * DataStack::fromText(const QString & str) const
{
  bool ok = false;
  int nb = str.toInt(&ok);
  if(! ok)
    return NULL;
  return numberedDataSet(nb);
}

bool DataStack::indexOf(const DataSet * ds, int * idx) const
{
  // I guess I can't get around the const_cast here...
  int i = dataSets.lastIndexOf(const_cast<DataSet*>(ds)); 
  if(i >= 0) {
    *idx = dataSets.size() - (1 + i);
    return true;
  }
  i = redoStack.indexOf(const_cast<DataSet*>(ds));
  if(i >= 0) {
    *idx = - (redoStack.size() - i);
    return true;
  }
  return false;
}

void DataStack::dropDataSet(int nb)
{
  QList<DataSet *> * lst;
  int idx = dsNumber2Index(nb, &lst);
  if(idx < lst->size())
    delete lst->takeAt(idx);
  if(! nb) 
    emit(currentDataSetChanged());
}

void DataStack::dropDataSet(const DataSet * ds)
{
  int idx;
  if(indexOf(ds, &idx))
    dropDataSet(idx);
}
