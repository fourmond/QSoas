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

#include <exceptions.hh>
#include <debug.hh>

#include <argument-templates.hh>

#include <general-arguments.hh>
#include <outfile.hh>
#include <dataset.hh>
#include <terminal.hh>

#include <mruby.hh>

#include <soas.hh>
#include <datastack.hh>

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

QString ValueHash::toString(const QVariant & value, bool * canConvert)
{
  QVariant v = value;
  if(canConvert)
    *canConvert = false;
  if(v.canConvert<double>()) {
   QVariant nv = v;
    if(nv.convert(QVariant::Double)) {
      if(canConvert)
        *canConvert = true;
      double d = nv.toDouble();
      return QString::number(d, 'g', 12);
    }
  }

  if(v.canConvert<QString>()) {
    QVariant nv = v;
    if(nv.convert(QVariant::String)) {
      if(canConvert)
        *canConvert = true;
      return nv.value<QString>();
    }
  }
  if(v.canConvert<QList<QVariant> >()) {
    QVariant nv = v;
    nv.convert(QMetaType::QVariantList);
    QList<QVariant> lst = nv.toList();
    QStringList rv;
    for(const QVariant & v : lst)
      rv << toString(v);
    return rv.join(", ");
  }
  return QString();
}

QString ValueHash::toString(const QString & sep, 
                            const QString & missing,
                            bool skip) const
{
  ValueHash v = *this;
  QStringList output;
  for(int i = 0; i < keyOrder.size(); i++) {
    const QString & k = keyOrder[i];
    bool ok = false;
    if(v.contains(k)) {
      QString s = toString(v[k], &ok);
      if(ok)
        output << s;
      v.remove(k);
    }
    if(! ok)
      output << missing;
  }

  QHash<QString, QVariant>::iterator it;
  if(! skip)
    for(it = v.begin(); it != v.end(); ++it) {
      bool ok = false;
      QString s = toString(*it, &ok);
      if(ok)
        output << s;
    }
  return output.join(sep);
}

typedef enum {
  Unspec,
  String,
  Number,
  Date,
  // StringList,
  NumberList
} VariantTypes;


QList<Argument *> ValueHash::variantConversionOptions()
{
  QHash<QString, VariantTypes>
    variantTypes({{"text", String},
                  {"number", Number},
                  // {"date", Date},
                  //          {"text-list", StringList},
                  {"number-list", NumberList}
      });
  QList<Argument*> args;
  args << new TemplateChoiceArgument<VariantTypes>
    (variantTypes, "type", "Type",
     "type of the meta-data");
  return args;
}



QVariant ValueHash::variantFromText(const QString & text,
                                    const CommandOptions & opts)
{
  VariantTypes type = Unspec;
  updateFromOptions(opts, "type", type);
  switch(type) {
  case Unspec: {
    bool ok = false;
    double val = text.toDouble(&ok);
    if(ok)
      return QVariant(val);
  }
  case String:
    return QVariant(text);
  case Number:{
    bool ok = false;
    double val = text.toDouble(&ok);
    if(! ok)
      throw RuntimeError("'%1' is not a number").arg(text);
    return QVariant(val);
  }
  case NumberList: {
    QStringList lst = text.split(QRegExp("\\s*,\\s*"));
    QList<QVariant> rv;
    for(const QString & t : lst) {
      bool ok = false;
      double val = t.toDouble(&ok);
      if(! ok)
        throw RuntimeError("'%1' is not a number").arg(text);
      rv << val;
    }
    return rv;
  }
  default:
    break;
  }
  return QVariant();
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
                               const QString & joinStringLists,
                               bool dumpOther,
                               bool sort,
                               bool overrideorder) const
{
  QString output;
  QHash<QString, QVariant> vals = *this;
  QHash<QString, QVariant>::iterator it;
  int done = 0;

  if(! overrideorder) {
    for(int i = 0; i < keyOrder.size(); i++) {
      const QString & k = keyOrder[i];
      it = vals.find(k);
      if(! (done % nbCols))
        output += prefix;

      bool found = false;
      QString s;
      if(it != vals.end()) {
        found = true;
        s = toString(*it);
        vals.erase(it);
      }
      else {
        if(dumpOther && contains(k)) {
          MRuby * mr = MRuby::ruby();
          // kinda hackish, but, well
          mrb_value v = variantToRuby((*this)[k]);
          found = true;
          s = mr->inspect(v);
        }
      }
      if(found) {
        output += QString("%1 =\t %2").arg(k).arg(s);
        done++;
        if(done % nbCols)
          output += "\t";
        else
          output += "\n";
      }
    }
  }
  
  QStringList keys = vals.keys();
  if(sort)
    qSort(keys);
  
  for(int i = 0; i < keys.size(); i++) {
    if(! (done % nbCols))
      output += prefix;
    output += QString("%1 =\t %2").arg(keys[i]).
      arg(toString(vals[keys[i]]));
    done++;
    if(done % nbCols)
      output += "\t";
    else
      output += "\n";
  }
  return output;
}


ValueHash & ValueHash::operator<<(const QList<QString> & lst)
{
  QList<QVariant> v;
  for(QVariant s : lst)
    v << s;

  return (*this) << v;
}

ValueHash & ValueHash::operator<<(mrb_value v)
{
  return (*this) << rubyToVariant(v);
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

void ValueHash::merge(const ValueHash & other, bool override,
                      bool allowMultiple)
{
  for(const_iterator it = other.begin(); it != other.end(); ++it) {
    if(override || (! contains(it.key())))
      (*this)[it.key()] = it.value();
  }
  keyOrder += other.keyOrder;
  /// @todo Only run if necessary ?
  if(! allowMultiple)
    keyOrder.removeDuplicates();
}

void ValueHash::appendToList(const QString & key, const QString & val)
{
  QStringList newval = value(key, QVariant()).toStringList();
  newval << val;
  (*this)[key] = newval;
}

QVariant ValueHash::rubyToVariant(mrb_value value)
{
  MRuby * mr = MRuby::ruby();
  if(mrb_float_p(value))
    return mr->floatValue_up(value);
  if(mrb_fixnum_p(value))
    // This may overflow...
    return QVariant((int)mrb_fixnum(value));

  QString ret = mr->toQString(value);
  /// @todo Handling of date/time
  return ret;
}

mrb_value ValueHash::variantToRuby(const QVariant & variant)
{
  MRuby * mr = MRuby::ruby();

  mrb_value val = mrb_nil_value();
  // Hmmm, QVariant says type() is QVariant::Type, but the
  // documentation says is really is QMetaType::Type.
  switch(variant.type()) {      // the documentation of QVariant::type()
                                // is rather confusing...
  case QMetaType::Double:
    val = mr->newFloat(variant.toDouble());
    break;
  case QMetaType::Int:          // (bad) support of large numbers
  case QMetaType::UInt:
  case QMetaType::Long:
  case QMetaType::ULong:
  case QMetaType::LongLong:
    // This will break in the case of very large numbers but will
    // succeed most of the times.
    val = mr->newInt(variant.toInt());
    break;
  // case QMetaType::LongLong:
  //   qlonglong val = variant.toLongLong();
  //   if(abs(val) < )
  case QMetaType::QString:
    val = mr->fromQString(variant.toString());
    break;
  case QMetaType::QDateTime:
  case QMetaType::QDate: {
    QDateTime dt = variant.toDateTime();
    QDate d = dt.date();
    QTime t = dt.time();
    val = mr->newTime(d.year(), d.month(), d.day(),
                       t.hour(), t.minute(), t.second(), t.msec() * 1000);
    break;
  }
  case QMetaType::QVariantList: {
    val = mr->newArray();
    QList<QVariant> lst = variant.toList();
    for(QVariant v : lst)
      mr->arrayPush(val, variantToRuby(v));
    break;
  }
  case QMetaType::QStringList: {
    val = mr->newArray();
    QStringList lst = variant.toStringList();
    for(QString v : lst)
      mr->arrayPush(val, mr->fromQString(v));
    break;
  }
  default:
    break;
  }
  return val;
}


mrb_value ValueHash::toRuby() const
{
  MRuby * mr = MRuby::ruby();
  mrb_value ret = mr->newFancyHash();
  MRubyArenaContext c(mr);

  // mr->gcRegister(ret);
  for(const_iterator it = begin(); it != end(); ++it) {
    try {
      // QTextStream o(stdout);
      // o << "Key: " << it.key() << " -> type: " << it.value().type() << endl;

      mrb_value key = mr->fromQString(it.key());
      // DUMP_MRUBY(key);
      mrb_value val = variantToRuby(it.value());
      // DUMP_MRUBY(val);
      mr->fancyHashSet(ret, key, val);
    }
    catch(RuntimeError & er) {
      Debug::debug() << "Error converting key '" << it.key() << "' (=" 
                     << it.value().toString() << "):"
                     << er.message() << "\n\t"
                     << er.exceptionBacktrace().join("\n\t") << endl;
    }
  }
  // mr->gcUnregister(ret);
  return ret;
}

void ValueHash::setFromRuby(mrb_value hsh)
{
  MRuby * mr = MRuby::ruby();
  if(! mr->isHash(hsh))
    throw RuntimeError("Trying to set a hash from a ruby value "
                       "that isn't a hash: '%1'").
      arg(mr->inspect(hsh));
  mr->hashIterate(hsh, [this, mr] (mrb_value key, mrb_value v) {
      QString k = mr->toQString(key);
      (*this)[k] = rubyToVariant(v);
    });
}

ValueHash ValueHash::fromRuby(mrb_value hsh)
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
       << (new SeveralStringsArgument(QRegExp("\\s*,\\s*"), "meta", 
                                     "Meta-data",
                                      "when writing to output file, also prints the listed meta-data"))->describe("comma-separated list of names of meta-data", "meta-data")
       << (new SeveralStringsArgument(QRegExp("\\s*,\\s*"), "set-meta", 
                                     "Set meta-data",
                                      "saves the results of the command as meta-data rather than/in addition to saving to the output file"))->describe("comma separated list of names of values (or meta-data), or `a->b` specifications, see [here](#output-set-meta)", "value-names")
       << (new SeveralStringsArgument(QRegExp("\\s*,\\s*"), "accumulate", 
                                     "Accumulate",
                                      "accumulate the given data into a dataset"))->describe("comma separated list of names of values or meta-data to accumulate, see [here](#output-accumulate)", "value-names")
       << (new SeveralStringsArgument(QRegExp("\\s*,\\s*"), "set-global", 
                                     "Set the $values global variable",
                                      "saves the results of the command into the $values global variable"))->describe("comma separated list of names of values (or meta-data), or `a->b` specifications, see [here](#output-set-global)", "value-names")
    ;
}

ValueHash ValueHash::copyFromSpec(const QStringList & spec, QStringList * missing) const
{
  ValueHash cnv;
  for(int i = 0; i < spec.size(); i++) {
    const QString &sp = spec[i];
    if(sp == "*") {
      cnv.merge(*this);
      cnv.keyOrder += keyOrder;
      continue;
    }
    int idx = sp.indexOf("->");
    if(idx < 0) {
      if(! contains(sp)){
        if(missing)
          *missing << sp;
      }
      else {
        cnv[sp] = (*this)[sp];
        cnv.keyOrder << sp;
      }
    }
    else {
      QString org = sp.left(idx);
      QString dest = sp.mid(idx+2);
      if(! contains(org)) {
        if(missing)
          *missing << org;
      }
      else {
        cnv[dest] = (*this)[org];
        cnv.keyOrder << dest;

      }
    }
  }
  return cnv;
}


bool ValueHash::hasOutputOptions(const CommandOptions & opts)
{
  return (opts.contains("output") ||
          opts.contains("meta") ||
          opts.contains("set-meta") ||
          opts.contains("set-global") ||
          opts.contains("accumulate"));
}

void ValueHash::handleOutput(const DataSet * ds, const CommandOptions & opts,
                             bool deflt) const
{

  /// @hack We'll have to const-cast, here. Is it a real problem ?
  bool output = deflt;
  updateFromOptions(opts, "output", output);
  QStringList metaNames;
  updateFromOptions(opts, "meta", metaNames);

  QStringList setMeta;
  updateFromOptions(opts, "set-meta", setMeta);

  QStringList setGlobal;
  updateFromOptions(opts, "set-global", setGlobal);

  QStringList accumulate;
  updateFromOptions(opts, "accumulate", accumulate);


  ValueHash meta;
  if(metaNames.size() > 0) {
    const ValueHash & origMeta = ds->getMetaData();
    for(const QString & n : metaNames) {
      if(n == "*")
        meta.merge(origMeta);
      else {
        if(origMeta.contains(n))
          meta << n << origMeta[n];
        else 
          Terminal::out << "Requested meta '" << n 
                        << "' but it is missing from buffer '"
                        << ds->name << "'" << endl;
      }
    }
  }
  
  ValueHash ov = *this;
  ov.merge(meta);
  if(output) {
    Terminal::out << "Writing to output file" << endl;
    OutFile::out.writeValueHash(ov, ds);
  }

  if(setMeta.size() > 0) {
    QStringList missing;
    ValueHash cnv = copyFromSpec(setMeta, &missing);
    if(missing.size() > 0)
      Terminal::out << "Missing the values for keys '" << missing.join("', '")
                    << "'" << endl;
    DataSet * d = const_cast<DataSet *>(ds);

    Terminal::out << "Setting meta-data :'"
                  << QStringList(cnv.keys()).join("', '")
                  << "'" << endl;
    d->addMetaData(cnv);
  }

  if(setGlobal.size() > 0) {
    QStringList missing;
    ValueHash cnv = copyFromSpec(setGlobal, &missing);
    if(missing.size() > 0)
      Terminal::out << "Missing the values for keys '" << missing.join("', '")
                    << "'" << endl;
    
    MRuby * mr = MRuby::ruby();
    mr->setGlobal("$values", cnv.toRuby());

  }

  if(accumulate.size() > 0) {
    QStringList missing;
    ValueHash cnv = ov.copyFromSpec(accumulate, &missing);
    cnv.merge(meta, true);

    if(missing.size() > 0)
      Terminal::out << "Missing the values for keys '" << missing.join("', '")
                    << "'" << endl;
    soas().stack().accumulateValues(cnv, ds->name);
  }
}
