/*
  columnbasedformat.cc: implementation of ColumnBasedFormat
  Copyright 2022 by CNRS/AMU

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
#include <columnbasedformat.hh>
#include <exceptions.hh>

#include <vector.hh>
#include <utils.hh>


ColumnBasedFormat::Value::Value(const QString & n, bool o,
                                const ReadFunction & r,
                                const WriteFunction & w) :
  name(n), optional(o), read(r), write(w)
{
}


ColumnBasedFormat::ColumnBasedFormat()
{
}

ColumnBasedFormat & ColumnBasedFormat::addColumn(const QString & name,
                                                 const ReadFunction & read,
                                                 const WriteFunction & write,
                                                 bool optional)
{
  if(indices.contains(name))
    throw InternalError("Duplicate column name: '%1'").arg(name);

  columns << Value(name, optional, read, write);
  indices[name] = columns.size() - 1;

  return *this;
}

QStringList ColumnBasedFormat::headers() const
{
  QStringList rv;
  for(const Value & v : columns)
    rv << v.name;
  return rv;
}

QStringList ColumnBasedFormat::writeValues() const
{
  QStringList rv;
  for(const Value & v : columns)
    rv << v.write();
  return rv;
}


void ColumnBasedFormat::readIntoVector(Vector & target, int index,
                                       const QString & value,
                                       double missing)
{
  while(target.size() <= index)
    target << missing;
  /// @todo No validation ?
  target[index] = value.toDouble();
}

void ColumnBasedFormat::readValues(const QHash<QString, QString> & source,
                                   QStringList * missingOptional) const
{
  bool missing[columns.size()];
  for(int i = 0; i < columns.size(); i++)
    missing[i] = true;
  QHash<QString, QString>::const_iterator i = source.begin();
  while (i != source.end()) {
    const QString & k = i.key();
    if(! indices.contains(k))
      throw RuntimeError("Unkonwn field: '%1'").arg(k);

    int idx = indices[k];
    columns[idx].read(i.value());
    missing[idx] = false;
  }

  for(int i = 0; i < columns.size(); i++) {
    if(missing[i]) {
      if(columns[i].optional) {
        if(missingOptional)
          *missingOptional << columns[i].name;
      }
      else {
        throw RuntimeError("Missing required field: '%1'").
          arg(columns[i].name);
      }
    }
  }
}
