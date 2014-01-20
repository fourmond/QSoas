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

#include <ruby.hh>
#include <exceptions.hh>

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

QHash<QString, QString> ValueHash::extractStrings(const QString & joinStringLists) const
{
  QHash<QString, QString> ret = extract<QString>(QVariant::String);
  if(! joinStringLists.isNull()) {
    // We look for string lists that were not converted already:
    for(const_iterator i = begin(); i != end(); i++) {
      QVariant v = i.value();
      if(! ret.contains(i.key()) && (v.type() == QVariant::StringList))
        ret[i.key()] = i.value().toStringList().join(joinStringLists);
    }
  }
  return ret;
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

 QString ValueHash::prettyPrint(int nbCols, 
                                const QString & prefix, 
                                const QString & joinStringLists) const
{
  QString output;
  QHash<QString, QString> strings = extractStrings(joinStringLists);
  QHash<QString, QString>::iterator it;
  int done = 0;

  for(int i = 0; i < keyOrder.size(); i++) {
    const QString & k = keyOrder[i];
    it = strings.find(k);
    if(! (done % nbCols))
      output += prefix;
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
    if(! (done % nbCols))
      output += prefix;
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

void ValueHash::merge(const ValueHash & other, bool override)
{
  for(const_iterator it = other.begin(); it != other.end(); it++) {
    if(override || (! contains(it.key())))
      (*this)[it.key()] = it.value();
  }
  keyOrder += other.keyOrder;
  /// @todo Only run if necessary ?
  keyOrder.removeDuplicates();
}

void ValueHash::appendToList(const QString & key, const QString & val)
{
  QStringList newval = value(key, QVariant()).toStringList();
  newval << val;
  (*this)[key] = newval;
}


VALUE ValueHash::toRuby() const
{
  VALUE ret = rb_hash_new();
  for(const_iterator it = begin(); it != end(); it++) {
    // Hmmm, QVariant says type() is QVariant::Type, but the
    // documentation says is really is QMetaType::Type.
    VALUE key = Ruby::fromQString(it.key());
    VALUE val = Qnil;
    switch(it.value().type()) {
    case QMetaType::Double:
      val = rb_float_new(it.value().toDouble());
      break;
    case QMetaType::QString:
      val = Ruby::fromQString(it.value().toString());
      break;
    default:
      ;
    }
    rb_hash_aset(ret, key, val);
  }
  return ret;
}

static int setFromRubyInternalHelper(VALUE key, VALUE val, void * arg)
{
  ValueHash * target = static_cast<ValueHash *>(arg);
  QString k = Ruby::toQString(key);
  QTextStream o(stdout);
  o << "Key: " << k << endl;
  // For now, very naive conversion...
  target->operator[](k) = Ruby::toQString(val);
  return ST_CONTINUE;
}

void ValueHash::setFromRuby(VALUE hsh)
{
  if(! RTEST(rb_obj_is_instance_of(hsh, rb_cHash)))
    throw RuntimeError("Trying to set a hash from a ruby value "
                       "that isn't a hash");
  rb_hash_foreach(hsh, (int (*)(...))
                  ::setFromRubyInternalHelper, (VALUE)this);
}

ValueHash ValueHash::fromRuby(VALUE hsh)
{
  ValueHash h;
  h.setFromRuby(hsh);
  return h;
}
