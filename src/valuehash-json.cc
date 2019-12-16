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
  return QJsonValue::fromVariant(variant);
}

static QVariant jsonToVariant(const QJsonValue & json)
{
  return json.toVariant();
}

void ValueHash::fromJSON(const QString & str)
{
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8(), &err);
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


QString ValueHash::toJSON() const
{
  QJsonObject obj;
  QHash<QString, QVariant>::const_iterator it;
  for(it = begin(); it != end(); ++it) {
    obj[it.key()] = variantToJSON(it.value());
  }

  QJsonDocument doc(obj);
  return doc.toJson();
}


