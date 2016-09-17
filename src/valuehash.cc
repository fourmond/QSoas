/*
  valuehash.cc: keeping track of who to thank really
  Copyright 2013, 2014 by CNRS/AMU

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
#include <ruby-templates.hh>
#include <exceptions.hh>
#include <debug.hh>

#include <general-arguments.hh>
#include <outfile.hh>
#include <dataset.hh>
#include <terminal.hh>

template<class T> QHash<QString, T> ValueHash::extract(QVariant::Type t) const
{
  QHash<QString, T> ret;
  for(const_iterator i = begin(); i != end(); ++i) {
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
    for(const_iterator i = begin(); i != end(); ++i) {
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

void ValueHash::clear()
{
  QHash<QString, QVariant>::clear();
  keyOrder.clear();
}

void ValueHash::append(const QString & key, const QVariant & value)
{
  keyOrder << key;
  operator[](key) = value;
}

void ValueHash::multiAdd(const QStringList & keys,
                         const QList<QVariant> & values)
{
  if(keys.size() != values.size())
    throw InternalError("Size mismatch in multiAdd: %1 vs %2").
      arg(keys.size()).arg(values.size());
  for(int i = 0; i < keys.size(); i++)
    append(keys[i], values[i]);
}

void ValueHash::prepend(const QString & key, const QVariant & value)
{
  keyOrder.prepend(key);
  operator[](key) = value;
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
    for(it = strings.begin(); it != strings.end(); ++it)
      output << *it;
  return output.join(sep);
}


ValueHash ValueHash::select(const QStringList & ks) const
{
  ValueHash rv;
  for(int i = 0; i < ks.size(); i++) {
    if(contains(ks[i]))
      rv << ks[i] << value(ks[i]);
  }
  return rv;
}

QString ValueHash::prettyPrint(int nbCols, 
                               const QString & prefix, 
                               const QString & joinStringLists, bool sort,
                               bool overrideorder) const
{
  QString output;
  QHash<QString, QString> strings = extractStrings(joinStringLists);
  QHash<QString, QString>::iterator it;
  int done = 0;

  if(! overrideorder) {
    for(int i = 0; i < keyOrder.size(); i++) {
      const QString & k = keyOrder[i];
      it = strings.find(k);
      if(! (done % nbCols))
        output += prefix;
      if(it != strings.end()) {
        output += QString("%1 =\t %2").arg(k).arg(*it);
        done++;
        if(done % nbCols)
          output += "\t";
        else
          output += "\n";
        strings.erase(it);
      }
    }
  }
  
  QStringList keys = strings.keys();
  if(sort)
    qSort(keys);
  
  for(int i = 0; i < keys.size(); i++) {
    if(! (done % nbCols))
      output += prefix;
    output += QString("%1 =\t %2").arg(keys[i]).arg(strings[keys[i]]);
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
  for(const_iterator it = other.begin(); it != other.end(); ++it) {
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

RUBY_VALUE ValueHash::variantToRuby(const QVariant & variant)
{
  RUBY_VALUE val = rbw_nil;
  switch(variant.type()) {      // the documentation of QVariant::type()
                                // is rather confusing...
  case QMetaType::Double:
    val = rbw_float_new(variant.toDouble());
    break;
  case QMetaType::Int:          // No support of large numbers
  case QMetaType::UInt:
    val = rbw_long2num(variant.toInt());
    break;
  case QMetaType::QString:
    val = Ruby::fromQString(variant.toString());
    break;
  case QMetaType::QTime: {
    QTime t = variant.toTime();
    // We use an epoch-based time: the time will be on the 1st of
    // January 1970
    QTime ref(0,0);
    val = Ruby::run<time_t, long>(rbw_time_new, ref.secsTo(t), 
                                  t.msec() * 1000);
    break;
  }
  case QMetaType::QDateTime:
  case QMetaType::QDate: {
    QDateTime t = variant.toDateTime();
    // in some cases, this call fails...
    val = Ruby::run<time_t, long>(rbw_time_new, t.toTime_t(), 
                                  t.time().msec() * 1000);
    break;
  }
  case QMetaType::QVariantList: {
    val = rbw_ary_new();
    QList<QVariant> lst = variant.toList();
    for(int i = 0; i < lst.size(); i++)
      rbw_ary_push(val, variantToRuby(lst[i]));
  }
  default:
    break;
  }
  return val;
}


RUBY_VALUE ValueHash::toRuby() const
{
  QMutexLocker l(&Ruby::rubyGlobalLock);
  RUBY_VALUE ret = rbw_hash_new();
  for(const_iterator it = begin(); it != end(); ++it) {
    // Hmmm, QVariant says type() is QVariant::Type, but the
    // documentation says is really is QMetaType::Type.
    try {
      RUBY_VALUE key = Ruby::fromQString(it.key());
      RUBY_VALUE val = variantToRuby(it.value());
      rbw_hash_aset(ret, key, val);
    }
    catch(RuntimeError & er) {
      Debug::debug() << "Error converting key '" << it.key() << "' (=" 
                     << it.value().toString() << "):"
                     << er.message() << "\n\t"
                     << er.exceptionBacktrace().join("\n\t") << endl;
    }
  }
  return ret;
}

static int setFromRubyInternalHelper(RUBY_VALUE key, RUBY_VALUE val, void * arg)
{
  ValueHash * target = static_cast<ValueHash *>(arg);
  QString k = Ruby::toQString(key);

  // For now, very naive conversion...
  target->operator[](k) = Ruby::toQVariant(val);

  return RBW_ST_CONTINUE;
}

void ValueHash::setFromRuby(RUBY_VALUE hsh)
{
  if(! rbw_is_hash(hsh))
    throw RuntimeError("Trying to set a hash from a ruby value "
                       "that isn't a hash");
  rbw_hash_foreach(hsh, (int (*)())
                  ::setFromRubyInternalHelper, (RUBY_VALUE)this);
}

ValueHash ValueHash::fromRuby(RUBY_VALUE hsh)
{
  ValueHash h;
  h.setFromRuby(hsh);
  return h;
}


double ValueHash::doubleValue(const QString & key) const
{
  bool ok;
  return doubleValue(key, &ok);
}

double ValueHash::doubleValue(const QString & key, bool * ok) const
{
  *ok = false;
  double n = 0.0/0.0;
  if(! contains(key)) {
    *ok = false;
    return n;
  }

  QVariant val = value(key);
  QVariant val2 = val;
  if(val.convert(QVariant::Double)) {
    *ok = true;
    return val.toDouble();
  }
  else if(val2.convert(QVariant::DateTime)) {
    *ok = true;
    QDateTime dt = val2.toDateTime();
    return dt.toMSecsSinceEpoch()*1e-3;
  }
  return n;
} 

void ValueHash::setValue(const QString & key, const QVariant& value)
{
  (*this)[key] = value;
}

QList<Argument *> ValueHash::outputOptions(bool deflt)
{
  return 
    QList<Argument *>() 
       << new BoolArgument("output", 
                           "To output file",
                           (deflt ?
                            "whether to write data to output file (defaults to true)" : "whether to write data to output file (defaults to false)")
                           )
       << new SeveralStringsArgument(QRegExp("\\s*,\\s*"), "meta", 
                                     "Meta-data",
                                     "when writing to output file, also prints the listed meta-data")
    ;
}

void ValueHash::handleOutput(const DataSet * ds, const CommandOptions & opts,
                             bool deflt) const
{
  bool output = deflt;
  updateFromOptions(opts, "output", output);
  QStringList metaNames;
  updateFromOptions(opts, "meta", metaNames);


  ValueHash meta;
  if(metaNames.size() > 0) {
    const ValueHash & origMeta = ds->getMetaData();
    for(int i = 0; i < metaNames.size(); i++) {
      const QString & n = metaNames[i];
      if(origMeta.contains(n))
        meta << n << origMeta[n];
      else 
        Terminal::out << "Requested meta '" << n 
                      << "' but it is missing from buffer" << endl;
    }
  }
  
  ValueHash ov = *this;
  if(output) {
    ov.merge(meta);
    Terminal::out << "Writing to output file" << endl;
    OutFile::out.writeValueHash(ov, ds);
  }
}
