/*
  curveitem.cc: implementation of the CurveItem class
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
#include <curveitem.hh>
#include <dataset.hh>

CurveItem::~CurveItem()
{
  delete cachedPath;
}

QRectF CurveItem::boundingRect() const
{
  return dataSet->boundingBox();
}

void CurveItem::createPath()
{
  if(cachedPath)
    return;
  cachedPath = new QPainterPath;
  int size = dataSet->nbRows();
  if(! size)
    return;
  const Vector & x = dataSet->x();
  const Vector & y = dataSet->y();
  cachedPath->moveTo(x[0], y[0]);
  for(int i = 1; i < size; i++)
    cachedPath->lineTo(x[i], y[i]);
}


void CurveItem::paint(QPainter * painter)
{
  createPath();
  QPen def("black");
  painter->setPen(def);
  painter->drawPath(*cachedPath);
}
