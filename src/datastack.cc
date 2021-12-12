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

#include <utils.hh>
#include <soas.hh>
#include <curveview.hh>
#include <debug.hh>

DataStack::DataStack(bool no) : notOwner(no),
  cachedByteSize(0), accumulator(NULL)
{
}

DataStack::~DataStack()
{
  if(! notOwner)
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

QSet<QString> DataStack::definedFlags() const
{
  QSet<QString> rv;
  for(int i = 0; i < dataSets.size(); i++)
    rv += dataSets[i]->allFlags();
  for(int i = 0; i < redoStack.size(); i++)
    rv += redoStack[i]->allFlags();
  return rv;
}

QList<const DataSet *> DataStack::datasetsFromSpec(const QString & str) const
{
  QList<const DataSet *> dsets;
  QRegExp multi("^\\s*(-?[0-9]+)\\s*..\\s*(-?[0-9]+|end)\\s*(?::(\\d+)\\s*)?\\s*$");
  QRegExp single("^\\s*-?[0-9]+\\s*$");

  QRegExp flgs("^\\s*(un)?flagged(-)?(:(.*))?\\s*$");
  flgs.setMinimal(true);        // to catch the last - if applicable

  if(multi.indexIn(str) == 0 || str == "all") {
    int first;
    int last;
    int sign = 1;
    if(str == "all") {
      first = -redoStackSize();
      last = stackSize()-1;
    }
    else {
      first = multi.cap(1).toInt();
      last = (multi.cap(2) == "end" ? stackSize()-1 : 
              multi.cap(2).toInt());
      sign = (first < last ? 1 : -1);
    }
    int delta = 1;
    if(! multi.cap(3).isEmpty())
      delta = multi.cap(3).toInt();
    while((first - last) * sign <= 0) {
      DataSet * ds = numberedDataSet(first);
      if(! ds)
        Terminal::out << "No such buffer number : " << first << endl;
      else
        dsets << ds;
      first += delta * sign;
    }
  }
  else if(flgs.indexIn(str) == 0) {
    bool flg = (flgs.cap(1).size() == 0);
    bool dec = (flgs.cap(2).size() > 0); // the - sign at the end
      
    QString flagName = flgs.cap(4);
    QList<const DataSet *> mkd;
    if(flagName.isEmpty())
      mkd = flaggedDataSets(flg);
    else
      mkd = flaggedDataSets(flg, flagName);

    if(dec)
      Utils::reverseList(mkd);

    dsets += mkd;
  }
      
  else if(str == "displayed")  {
    QList<DataSet *> mkd = soas().view().displayedDataSets();
    for(int i = 0; i < mkd.size(); i++)
      dsets << mkd[i];
  }
  else if(str.startsWith("named:")) {
    int d = str.indexOf(':');
    QString n = str.mid(d+1);
    return namedDataSets(n);
  }
  else if(str == "latest" || str.startsWith("latest:"))  {
    int idx = 1;
    int d = str.indexOf(':');
    if(d > 0)
      idx = str.mid(d+1).toInt();
    const QList<GuardedPointer<DataSet> > & src = latest[idx];
    for(int i = 0; i < src.size(); i++) {
      if(src[i].isValid())
        dsets << src[i];
    }
  }
  else if(single.indexIn(str) == 0) {
    int idx = str.toInt();      // Valid
    DataSet * ds = numberedDataSet(idx);
    if(! ds)
      Terminal::out << "No such buffer number: " << idx << endl;
    dsets << ds;
  }
  else
    throw RuntimeError("Not a dataset: '%1'").arg(str);
  return dsets;
}

void DataStack::startNewCommand()
{
  latest.insert(0, QList<GuardedPointer<DataSet> >());
}

void DataStack::setAutoFlags(const QSet<QString> & flags)
{
  autoFlags = flags;
}

int DataStack::pushSpy()
{
  int rv = spies.size();
  spies << QList<GuardedPointer<DataSet> >();
  return rv;
}

QList<DataSet *> DataStack::popSpy(int targetLevel)
{
  QList<GuardedPointer<DataSet> > lst;
  if(spies.size() > targetLevel)
    lst = spies[targetLevel];
  spies = spies.mid(0, targetLevel);
  QList<DataSet *> dss;
  for(GuardedPointer<DataSet> & p : lst) {
    if(p.isValid())
      dss << p.target();
  }
  return dss;
}

void DataStack::pushDataSet(DataSet * dataset, bool silent)
{
  dataSets << dataset;
  latest.first() << dataset;
  dataset->setFlags(autoFlags);
  cachedByteSize += dataset->byteSize();
  for(QList<GuardedPointer<DataSet> > & l : spies)
    l << dataset;
  if(! silent) {
    Debug::debug().saveStack();
    emit(currentDataSetChanged());
  }
}

void DataStack::showStackContents(int nb,
                                  const QStringList & meta) const
{
  QString head("\t F  C\tRows\tSegs\tName\t");
  head += meta.join("\t");
  if(redoStack.size())
    Terminal::out << "Redo stack:\n" << head << endl;
  for(int i = -redoStack.size(); i < dataSets.size(); i++) {
    if(nb > 0 && abs(i) >= nb)
      continue;
    if(! i)
      Terminal::out << "Normal stack:\n" << head << endl;
    DataSet * ds = numberedDataSet(i);
    Terminal::out << Terminal::alternate << "#" << i << "\t"
                  << ds->stringDescription();
    for(const QString & m : meta)
      Terminal::out << "\t" << ds->getMetaData(m).toString();
    
    Terminal::out << endl;
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

QSet<QString> DataStack::datasetNames() const
{
  QSet<QString> ret;
  for(const DataSet * ds : dataSets)
    ret.insert(ds->name);
  for(const DataSet * ds : redoStack)
    ret.insert(ds->name);
  return ret;
}

QList<const DataSet *> DataStack::namedDataSets(const QString & name,
                                                NameMatchingRule matcher) const
{
  QList<const DataSet *> rv;
  QRegExp::PatternSyntax st = QRegExp::Wildcard;
  if(matcher == Strict)
    st = QRegExp::FixedString;
  if(matcher == Regex)
    st = QRegExp::RegExp;
  QRegExp mt(name, Qt::CaseSensitive, st);
  
  for(const DataSet * ds : dataSets) {
    if(mt.exactMatch(ds->name))
      rv << ds;
  }
  for(const DataSet * ds : redoStack) {
    if(mt.exactMatch(ds->name))
      rv << ds;
  }
  return rv;
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

QList<const DataSet *> DataStack::flaggedDataSets(bool flagged, const QString & flag) const
{
  QList<DataSet *> ds = const_cast<DataStack *>(this)->flaggedDataSets(flagged, flag);
  QList<const DataSet *> ret;
  for(int i = 0; i < ds.size(); i++)
    ret << ds[i];
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
  cachedByteSize = 0;
  emit(currentDataSetChanged());
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

QString DataStack::textSummary() const
{
  QString s;
  QTextStream o(&s);
  DataSet * cds = currentDataSet();
  o << dataSets.size() << " datasets, "
    << redoStack.size() << " redo stack, "
    << (cachedByteSize >> 10) << " kB, current buffer: "
    << (cds ? QString("'%1'").arg(cds->name) : "(none)");
  return s;
}


void DataStack::accumulateValues(const ValueHash & data)
{
  if(! accumulator)
    accumulator = new DataSet;
  // accumulator->setMetaData("keys", data.keyOrder);
  int nbr = accumulator->nbRows();
  for(int i = 0; i < data.keyOrder.size(); i++) {
    const QString & k = data.keyOrder[i];
    if(i >= accumulator->nbColumns()) {
      Vector v;
      if(nbr > 0)
        v = Vector(nbr, 0.0/0.0);
      accumulator->appendColumn(v);
    }
    accumulator->column(i) << data[k].toDouble();
  }
  if(accumulator->columnNames.size() == 0)
    accumulator->columnNames << data.keyOrder;
  else {
    if(accumulator->columnNames[0] != data.keyOrder) {
      Terminal::out << "Mismatch in the accumulator column names: "
                    << accumulator->columnNames[0].join(", ") << " vs "
                    << data.keyOrder.join(", ") << endl;
      accumulator->columnNames[0] = data.keyOrder;
    }
  }
  Terminal::out << "Accumulator has " << accumulator->nbRows()
                << " rows and " << accumulator->nbColumns()
                << " columns" << endl;
}

DataSet * DataStack::popAccumulator()
{
  DataSet * ds = accumulator;
  accumulator = NULL;
  return ds;
}

const DataSet * DataStack::peekAccumulator()
{
  return accumulator;
}


void DataStack::insertStack(const DataStack & s)
{
  dataSets.append(s.dataSets);
  redoStack.append(s.redoStack);
  emit(currentDataSetChanged());
}


void DataStack::reorderDatasets(const QList<const DataSet *> newOrder)
{
  QList<int> indices;
  for(const DataSet * ds : newOrder) {
    int idx;
    if(! indexOf(ds, &idx))
      throw RuntimeError("Trying to sort a dataset which isn't in the stack");
    indices << idx;
  }
  std::sort(indices.begin(), indices.end());
  for(int i = 0; i < newOrder.size(); i++) {
    int idx = indices[i];
    if(idx < 0)
      redoStack[redoStack.size() - idx] = const_cast<DataSet*>(newOrder[i]);
    else
      dataSets[dataSets.size() - 1 - idx] = const_cast<DataSet*>(newOrder[i]);
  }
}


qint32 DataStack::serializationVersion = 0;

//////////////////////////////////////////////////////////////////////


/// @todo write out the accumulator ?



#define MAGIC 0xFF342210
#define CURRENT_STACK_VERSION 6


void DataStack::writeSerializationHeader(QDataStream & out)
{
  qint32 v = -CURRENT_STACK_VERSION; // (negative) Current version
  // We use Qt format 4.8 by default
  out.setVersion(QDataStream::Qt_4_8);
  out << v;
  v = MAGIC;
  out << v;
}

void DataStack::writeStack(QDataStream & out) const
{
  qint32 nbDs = dataSets.size();
  out << nbDs;
  for(qint32 i = 0; i < nbDs; i++)
    out << *dataSets[i];

  nbDs = redoStack.size();
  out << nbDs;
  for(qint32 i = 0; i < nbDs; i++)
    out << *redoStack[i];
}

QDataStream & operator<<(QDataStream & out, const DataStack & stack)
{
  DataStack::writeSerializationHeader(out);
  // QByteArray b;
  // QDataStream o(&b, QIODevice::WriteOnly);
  stack.writeStack(out);
  // out << qCompress(b);
  return out;
}

void DataStack::readSerializationHeader(QDataStream & in)
{
  qint32 nbDs;
  in.setVersion(QDataStream::Qt_4_8);
  in >> nbDs;

  // Versioning !
  if(nbDs >= 0 || nbDs < -CURRENT_STACK_VERSION)
    throw RuntimeError("Bad signature for stack file '%1', or "
                       "very very very old stack file, aborting").
      arg(Utils::fileName(in));
  else {
    DataStack::serializationVersion = -nbDs;
    in >> nbDs;
    if(nbDs != MAGIC)
      throw RuntimeError("Bad signature for stack file '%1', aborting").
      arg(Utils::fileName(in));
  }
}

void DataStack::readStack(QDataStream & in)
{
  qint32 nbDs;
  in >> nbDs;
  cachedByteSize = 0;
  dataSets.clear();
  for(qint32 i = 0; i < nbDs; i++) {
    DataSet * ds = new DataSet;
    in >> *ds;
    dataSets.append(ds);
    cachedByteSize += ds->byteSize();
  }

  in >> nbDs;
  redoStack.clear();
  for(qint32 i = 0; i < nbDs; i++) {
    DataSet * ds = new DataSet;
    in >> *ds;
    redoStack.append(ds);
    cachedByteSize += ds->byteSize();
  }
  emit(currentDataSetChanged());
}




QDataStream & operator>>(QDataStream & in, DataStack & stack)
{
  DataStack::readSerializationHeader(in);
  // if(DataStack::serializationVersion >= 6) {
  //   QByteArray cmp;
  //   in >> cmp;
  //   cmp = qUncompress(cmp);
  //   QDataStream nin(cmp);
  //   stack.readStack(in);
  // }
  // else
  stack.readStack(in);
  return in;
}

