/*
  curvedataset.cc: implementation of the CurveDataSet class
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
#include <curvedataset.hh>
#include <dataset.hh>

CurveDataSet::~CurveDataSet()
{
  delete cachedPath;
}

QRectF CurveDataSet::boundingRect() const
{
  return dataSet->boundingBox();
}

void CurveDataSet::createPath()
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


void CurveDataSet::paint(QPainter * painter)
{
  createPath();
  painter->setPen(pen);
  painter->drawPath(*cachedPath);
}

QRect CurveDataSet::paintLegend(QPainter * p, const QRect & rect)
{
  /// @todo many things to customize here...
  
  QPoint p1 = QPoint(rect.left(), (rect.bottom() + rect.top())/2);
  QPoint p2 = p1 + QPoint(10, 0); // Mwouaf...

  p->save();
  QPen pen2 = pen;
  pen2.setWidthF(1.5);
  p->setPen(pen);
  p->drawLine(p1, p2);
  p->restore();

  QRect tmp(p1, p2);
  tmp.adjust(0,-1,0,1);
  
  QRect r = rect.adjusted(13, 0, 0, 0);
  QRect t;
  p->drawText(r, 
              Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip,
              dataSet->name,
              &t);
  return t.united(tmp);
}
