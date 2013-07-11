/*
  regex.cc: keeping track of who to thank really
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
#include <regex.hh>

Regex::Regex(const QString & p) :
  originalPattern(p)
{
  pattern = parsePattern(originalPattern, &cs);
}

QString Regex::parsePattern(const QString & pattern, 
                            Qt::CaseSensitivity * cs)
{
  *cs = Qt::CaseSensitive;
  QString p;
  if(pattern.startsWith("/")) {
    if(pattern.endsWith("/i")) {
      p = pattern.mid(1, pattern.size() - 3);
      *cs = Qt::CaseInsensitive;
      return p;
    }
    else if(pattern.endsWith("/")) {
      p = pattern.mid(1, pattern.size() - 2);
      return p;
    }
    else {
      // Issue a warning ?
    }
  }

  /// @todo support combination
  if(pattern == "{blank-line}")      // blank line
    p = "^\\s*$";
  else if(pattern == "{blank}")          // blank line
    p = "\\s+";
  else if(pattern == "{text-line}")  // text line
    p = "^\\s*[^0-9.+-\\s]";
  else
    p = QRegExp::escape(pattern);
  return p;
}

QRegExp Regex::toQRegExp() const
{
  return QRegExp(pattern, cs, QRegExp::RegExp2);
}
