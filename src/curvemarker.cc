/*
  curvemarker.cc: implementation of the useful CurveItem children
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
#include <curvemarker.hh>


void CurveMarker::paintMarker(QPainter * painter, const QPointF & realPos)
{
  switch(type) {
  case Circle:
    painter->drawEllipse(realPos, size, size);
    break;
  default:
  }
}


void CurveMarker::paint(QPainter * painter, const QRectF &)
{
  // Here, we cheat
  QTransform t = painter->transform();
  QPointF realP = t.map(p);

  painter->save();
  painter->setPen(pen);
  painter->setBrush(brush);

  // Now, we cancel the transformation, then we draw !
  painter->setTransform(QTransform());
  paintMarker(painter, realP);
  painter->restore();
}
