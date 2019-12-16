/*
  printpreviewhelper.cc: print preview
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
#include <printpreviewhelper.hh>

#include <soas.hh>
#include <curveview.hh>

PrintPreviewHelper::PrintPreviewHelper(int h, const QRect & r,
                                       const QString & t) :
  height(h), rect(r), title(t)
{
}

void PrintPreviewHelper::paint(QPaintDevice * p)
{
  QPainter painter;
  painter.begin(p);
  soas().view().render(&painter, height, rect, title);
}

void PrintPreviewHelper::paintOnPrinter(QPrinter * tg)
{
  paint(tg);
}
