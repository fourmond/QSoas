/*
  utils.cc: various utility functions
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
#include <utils.hh>

QStringList Utils::glob(const QString & pattern, bool trim)
{
  QFileInfo info(pattern);
  QDir dir = info.dir();
  QStringList lst = dir.entryList(QStringList() << info.fileName(), 
                                  QDir::NoDotAndDotDot |
                                  QDir::Files | QDir::Dirs | 
                                  QDir::CaseSensitive);
  if(lst.isEmpty() && ! trim)
    lst << info.fileName();
  for(int i = 0; i < lst.size(); i++)
    lst[i] = dir.filePath(lst[i]);
  return lst;
}

QStringList Utils::stringsStartingWith(const QStringList & strings, 
                                       const QString & start)
{
  QStringList ret;
  for(int i = 0; i < strings.size(); i++) {
    if(strings[i].startsWith(start))
      ret << strings[i];
  }
  return ret;
}

QString Utils::commonBeginning(const QStringList & strings)
{
  if(! strings.size())
    return "";

  // First, determine the minimum size of the strings
  int min = strings.first().size();
  for(int i = 1; i < strings.size(); i++)
    if(strings[i].size() < min)
      min = strings[i].size();

  QString ret;
  int i = 0;
  while(i < min) {
    QChar cur = strings.first()[i];
    bool diff = false;
    for(int j = 1; j < strings.size(); j++)
      if( strings[j][i] != cur) {
        diff = true;
        break;
      }
    if(diff)
      break;
    ret.append(cur);
    i++;
  }
  return ret;
}

bool Utils::askConfirmation(const QString & what, 
                            const QString & title)
{
  QString t = (title.isEmpty() ? QObject::tr("Please confirm") : title);
  return QMessageBox::question(NULL, t, what,
                               QMessageBox::Ok | QMessageBox::Cancel) == 
    QMessageBox::Ok;
}

QRectF Utils::scaledAround(const QRectF & rect, const QPointF & point,
                           double xscale, double yscale)
{
  QRectF nr(point, QSizeF(0,0));
  
  nr.adjust((rect.left() - point.x()) * xscale,
            (rect.top() - point.y()) * yscale,
            (rect.right() - point.x()) * xscale,
            (rect.bottom() - point.y()) * yscale);
  return nr;
}
