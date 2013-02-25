/*
  valuehash.cc: keeping track of who to thank really
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
#include <valuehash.hh>

template<class T> QHash<QString, T> ValueHash::extract(QVariant::Type t) const
{
  QHash<QString, T> ret;
  for(const_iterator i = begin(); i != end(); i++) {
    QVariant v = i.value();
    if(v.canConvert<T>() && v.convert(t))
      ret[i.key()] = v.value<T>();
  }
  return ret;
}

QHash<QString, double> ValueHash::extractDoubles() const
{
  return extract<double>(QVariant::Double);
}

QHash<QString, QString> ValueHash::extractStrings() const
{
  return extract<QString>(QVariant::String);
}

QHash<QString, QDateTime> ValueHash::extractDates() const
{
  return extract<QDateTime>(QVariant::DateTime);
}

QString ValueHash::toString(const QString & sep, 
                            const QString & missing,
                            bool skip) const
{
  QHash<QString, QString> strings = extractStrings();
  QHash<QString, QString>::iterator it;
  QStringList output;
  for(int i = 0; i < keyOrder.size(); i++) {
    const QString & k = keyOrder[i];
    it = strings.find(k);
    if(it != strings.end()) {
      output << *it;
      strings.erase(it);
    }
    else
      output << missing;
  }

  if(! skip)
    for(it = strings.begin(); it != strings.end(); it++)
      output << *it;
  return output.join(sep);
}

QString ValueHash::prettyPrint(int nbCols) const
{
  QString output;
  QHash<QString, QString> strings = extractStrings();
  QHash<QString, QString>::iterator it;
  int done = 0;

  for(int i = 0; i < keyOrder.size(); i++) {
    const QString & k = keyOrder[i];
    it = strings.find(k);
    if(it != strings.end()) {
      output += QString("%1 = %2").arg(k).arg(*it);
      done++;
      if(done % nbCols)
        output += "\t";
      else
        output += "\n";
      strings.erase(it);
    }
  }
  
  for(it = strings.begin(); it != strings.end(); it++) {
    output += QString("%1 = %2").arg(it.key()).arg(*it);
    done++;
    if(done % nbCols)
      output += "\t";
    else
      output += "\n";
  }
  return output;
}


ValueHash & ValueHash::operator<<(const QVariant & v)
{
  if(lastKey.isEmpty())
    /// @todo maybe should raise a warning if initial type isn't
    /// string ?
    lastKey = v.toString();
  else {
    (*this)[lastKey] = v;
    keyOrder << lastKey;
    lastKey.clear();
  }
  return *this;
}

void ValueHash::merge(const ValueHash & other)
{
  for(const_iterator it = other.begin(); it != other.end(); it++)
    (*this)[it.key()] = it.value();
  keyOrder += other.keyOrder;
  /// @todo Only run if necessary ?
  keyOrder.removeDuplicates();
}
