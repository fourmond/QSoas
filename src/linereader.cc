/*
  linereader.cc: implementation of the LineReader class
  Copyright 2014 by CNRS/AMU

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
#include <linereader.hh>

LineReader::LineReader(QIODevice * dev) : hasPending(false)
{
  source = new QTextStream(dev);
  ownSource = true;
}

LineReader::LineReader(QTextStream * s) : source(s),
                                          ownSource(false),
                                          hasPending(false)
{
}

LineReader::~LineReader()
{
  if(ownSource)
    delete source;
}

class LREOF {
public:
  LREOF() {;};
};

QChar LineReader::getc()
{
  if(hasPending) {
    hasPending = false;
    return pending;
  }
  QChar ch;
  if(source->atEnd())
    throw LREOF();
  *source >> ch;
  return ch;
}

QChar LineReader::peekc()
{
  QChar r = getc();
  hasPending = true;
  pending = r;
  return r;
}

bool LineReader::atEnd() const
{
  if(hasPending)
    return false;
  return source->atEnd();
}

QString LineReader::readLine(bool keepCR)
{
  QString ret;
  try {
    while(! atEnd()) {
      QChar c = getc();
      if(c == '\r') {
        c = peekc();
        if(c == '\n')
          getc();
        break;
      }
      if(c == '\n') {
        c = peekc();
        if(c == '\r')
          getc();
        break;
      }
      ret.append(c);
    }
  }
  catch(const LREOF & s) {
  }
  if(keepCR)
    ret.append('\n');
  return ret;
}
