/*
  valuehash-json.cc: implementation of JSON meta-data files for ValueHash
  Copyright 2019 by CNRS/AMU

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


static QJsonValue variantToJSON(const QVariant & variant)
{
  QJsonValue ret;
  switch(variant.type()) {      // the documentation of QVariant::type()
                                // is rather confusing...
  case QMetaType::Double:
    return QJsonValue(variant.toDouble());
  case QMetaType::Int:          // No support of large numbers
  case QMetaType::UInt:
    return QJsonValue(variant.toInt());
  case QMetaType::QString:
    return QJsonValue(variant.toString());
  default:
    break;
  }
  
  return ret;
}

static QVariant jsonToVariant(const QJsonValue & json)
{
  switch(json.type()) {
  case QJsonValue::Null:
    return QVariant();
  case QJsonValue::Bool:
    return json.toBool();
  case QJsonValue::Double: {
    int iv = json.toInt();
    double dv = json.toDouble();
    if(iv == dv)
      return iv;
    return dv;
  }
  case QJsonValue::String:
    return json.toString();
  default:
    break;
  }
  return QVariant();
}

void ValueHash::readMetaDataFile(QTextStream & in)
{
  QString s = in.readAll();
  int idx = s.indexOf('{');
  if(idx < 0)
    throw RuntimeError("No opening brace in meta-data file");
  QString header = s.left(idx);
  QRegExp vre("//\\s+QSoas\\s+JSON\\s+.*version\\s*(\\d+)");
  if(vre.indexIn(header) < 0)
    throw RuntimeError("No QSoas signature in meta-data file");
  
  s = s.mid(idx);
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(s.toUtf8(), &err);
  if(doc.isNull()) {
    throw RuntimeError("Failed to parse the JSON in the meta-data file");
  }
  if(! doc.isObject())
    throw RuntimeError("The JSON data should be a single object");
  QJsonObject obj = doc.object();

  QJsonObject::const_iterator it;

  for(it = obj.constBegin(); it != obj.constEnd(); ++it) {
    QVariant v = jsonToVariant(it.value());
    if(! v.isNull())
      (*this)[it.key()] = v;
  }
}


void ValueHash::writeMetaDataFile(QTextStream & out) const
{
  QJsonObject obj;
  QHash<QString, QVariant>::const_iterator it;
  for(it = begin(); it != end(); ++it) {
    obj[it.key()] = variantToJSON(it.value());
  }

  out << "// QSoas JSON meta-data -- version 1" << endl;
  QJsonDocument doc(obj);
  out << doc.toJson();
  out << endl;
}


