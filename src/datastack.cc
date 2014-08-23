/*
  datastack.cc: implementation of the DataStack class
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014 by CNRS/AMU

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

#include <exceptions.hh>

DataStack::DataStack() : 
  cachedByteSize(0)
{
}

DataStack::~DataStack()
{
  clear();
}

int DataStack::totalSize() const
{
  return stackSize() + redoStackSize();
}

quint64 DataStack::byteSize() const
{
  return cachedByteSize;
}

void DataStack::pushDataSet(DataSet * dataset, bool silent)
{
  dataSets << dataset;
  dataSetByName[dataset->name] = dataset;
  cachedByteSize += dataset->byteSize();
  if(! silent)
    emit(currentDataSetChanged());
}

void DataStack::showStackContents(int nb, bool /*unused*/) const
{
  QString head("\t F  C\tRows\tSegs");
  if(redoStack.size())
    Terminal::out << "Redo stack:\n" << head << endl;
  for(int i = -redoStack.size(); i < dataSets.size(); i++) {
    if(nb > 0 && abs(i) >= nb)
      continue;
    if(! i)
      Terminal::out << "Normal stack:\n" << head << endl;
    DataSet * ds = numberedDataSet(i);
    Terminal::out << "#" << i << "\t"
                  << ds->stringDescription() << endl;
  }
  Terminal::out << "Total size: " << (cachedByteSize >> 10) << " kB" << endl;
}

QList<const DataSet *> DataStack::allDataSets() const
{
  QList<const DataSet *> ret;
  for(int i = -redoStack.size(); i < dataSets.size(); i++)
    ret << numberedDataSet(i);
  return ret;
}

QList<DataSet *> DataStack::flaggedDataSets(bool flagged, const QString & flag)
{
  QList<DataSet *> ret;
  for(int i = -redoStack.size(); i < dataSets.size(); i++) {
    DataSet * ds = numberedDataSet(i);

    QStringList lst = QStringList::fromSet(ds->allFlags());

    bool flg = flag.isEmpty() ? ds->flagged() : ds->flagged(flag);
    if((!flg) == (!flagged))
      ret << ds;
  }
  return ret;
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
    throw RuntimeError("Need a buffer !");
  return ds;
}

void DataStack::undo(int nbTimes)
{
  while(nbTimes--) {
    if(dataSets.size() < 2)
      throw RuntimeError("Undo: nothing to undo");
    redoStack.append(dataSets.takeLast());
    emit(currentDataSetChanged());
  }
}

void DataStack::redo(int nbTimes)
{
  while(nbTimes--) {
    if(! redoStack.size())
      throw RuntimeError("Redo: nothing to redo");
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
  if(idx >= 0 && idx < lst->size()) {
    DataSet * ds = lst->takeAt(idx);
    cachedByteSize -= ds->byteSize();
    delete ds;
  }
  if(! nb) 
    emit(currentDataSetChanged());
}

void DataStack::dropDataSet(const DataSet * ds)
{
  int idx;
  if(indexOf(ds, &idx))
    dropDataSet(idx);
}

qint32 DataStack::serializationVersion = 0;

//////////////////////////////////////////////////////////////////////


/// @todo It may be possible to implement data serialization
/// versioning, ie being a little more tolerant about data persistence:
/// @li it would need a global variable -- as QDataStream doesn't have
/// means to carry use attribues
/// @li it would probably be good too to write out the QDataStream
/// version



#define MAGIC 0xFF342210

QDataStream & operator<<(QDataStream & out, const DataStack & stack)
{
  qint32 v = -5;                // (negative) Current version
  out << v;
  v = MAGIC;
  out << v;
  qint32 nbDs = stack.dataSets.size();
  out << nbDs;
  for(qint32 i = 0; i < nbDs; i++)
    out << *stack.dataSets[i];

  nbDs = stack.redoStack.size();
  out << nbDs;
  for(qint32 i = 0; i < nbDs; i++)
    out << *stack.redoStack[i];
  return out;
}


QDataStream & operator>>(QDataStream & in, DataStack & stack)
{
  qint32 nbDs;
  in >> nbDs;

  // Versioning !
  if(nbDs >= 0)
    DataStack::serializationVersion = 0;
  else {
    DataStack::serializationVersion = -nbDs;
    in >> nbDs;
    if(nbDs != MAGIC)
      throw RuntimeError("Bad signature for stack file, aborting");
    in >> nbDs;
  }

  stack.cachedByteSize = 0;
  stack.dataSets.clear();
  for(qint32 i = 0; i < nbDs; i++) {
    DataSet * ds = new DataSet;
    in >> *ds;
    stack.dataSets.append(ds);
    stack.cachedByteSize += ds->byteSize();
  }

  in >> nbDs;
  stack.redoStack.clear();
  for(qint32 i = 0; i < nbDs; i++) {
    DataSet * ds = new DataSet;
    in >> *ds;
    stack.redoStack.append(ds);
    stack.cachedByteSize += ds->byteSize();
  }
  /// Explicity signal call !
  stack.currentDataSetChanged();
  return in;
}
