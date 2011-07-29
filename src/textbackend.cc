/*
  textbackend.cc: implementation of the TextBackend class
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
#include <textbackend.hh>
#include <dataset.hh>

TextBackend::TextBackend(const QRegExp & sep,
                         const char * n, const char * pn, const char * d) : 
  DataBackend(n, pn, d), separator(sep) {
}

int TextBackend::couldBeMine(const QByteArray & peek, 
                             const QString & fileName) const
{
  int nbSpaces = 0, nbSpecials = 0, nbRet = 0;
  for(int i = 0; i < peek.size(); i++) {
    unsigned char c = peek[i];
    switch(c) {
    case '\n': 
      nbRet++;
      break;
    case ' ':
    case '\t':
      nbSpaces++;
      break;
    case '\r': // ignoring that guy
      break;
    default:
      if(c < 32)
        nbSpecials++;
    }
  }
  QTextStream o(stdout);
  o << "On " << peek.size() << ", found: " 
    << nbSpecials << " specials and " << nbRet << " returns " << endl;
  if(nbSpecials > peek.size()/100)
    return 0;                   // Most probably not
  QString str(peek);
  int nbSeparatorMatches = str.split(separator).size();
  o << " -> and there are " << nbSeparatorMatches << " matches" << endl;
  /// Heuristic: we must one or more separators per line.
  if(nbSeparatorMatches > nbRet) {
    if(separator.indexIn(" \t") >= 0)
      return 2*nbSeparatorMatches - (5 * nbRet)/3;
    else
      return 2*nbSeparatorMatches - nbRet;
  }
  return 0;
}

DataSet * TextBackend::readFromStream(QIODevice * stream,
                                      const QString & /*fileName*/,
                                      const QString & /*args*/) const
{
  /// @todo implement comments / header parsing... BTW, maybe
  /// Vector::readFromStream could be more verbose about the nature of
  /// the commends (the exact line, for instance ?)
  QList<Vector> columns = Vector::readFromStream(stream, 
                                                 separator.pattern());
  /// @todo separator.pattern() is simply ugly. Fix that somehow (on
  /// the Vector side)
  DataSet * ds = new DataSet(columns);
  /// @todo 
  return ds;
}



TextBackend csv(QRegExp("\\s*[;,]\\s*"),
                "csv",
                QT_TRANSLATE_NOOP("Backends", "CSV files"),
                QT_TRANSLATE_NOOP("Backends", "Backend to read CSV files"));


TextBackend text(QRegExp("\\s+"),
                 "text",
                QT_TRANSLATE_NOOP("Backends", "Text files"),
                QT_TRANSLATE_NOOP("Backends", "Backend to read plain text files"));

                
