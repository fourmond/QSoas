/*
  icons.cc: implementation of icon loading
  Copyright 2017 by CNRS/AMU

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
#include <icons.hh>
#include <fileinfo.hh>

#include <debug.hh>

QIcon Icons::namedIcon(const QString & name)
{
  QString path = ":/icons/" + name + ".svg";
  FileInfo info(path);
  if(! info.exists()) {
    Debug::debug() << "Asked for icon '" << name
                   << "', but " << path << " does not exist" << endl;
  }
  QIcon rv = QIcon::fromTheme(name);
  if(rv.isNull())
    rv = QIcon(path);

  return rv;
}
