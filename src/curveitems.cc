/*
  curveitems.cc: implementation of the useful CurveItem children
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
#include <curveitems.hh>

QRectF CurveLine::boundingRect() const
{
  return QRectF(p1, p2).normalized();
}

void CurveLine::paint(QPainter * painter, const QRectF &)
{
  painter->save();
  painter->setPen(pen);
  painter->drawLine(p1, p2);
  painter->restore();
}

void CurveVerticalLine::paint(QPainter * painter, const QRectF & bbox)
{
  painter->save();
  painter->setPen(pen);
  painter->drawLine(QLineF(x, bbox.top(), x, bbox.bottom()));
  painter->restore();
}


void CurveHorizontalRegion::setX(double value, Qt::MouseButton button)
{
  switch(button) {
  case Qt::LeftButton:
    xleft = value;
    break;
  case Qt::RightButton:
    xright = value;
    break;
  default:
    ;
  }
  if(xleft > xright)
    std::swap(xleft, xright);
}


void CurveHorizontalRegion::paint(QPainter * painter, const QRectF & bbox)
{
  painter->save();
  painter->setPen(pen);
  painter->drawLine(QLineF(xleft, bbox.top(), xleft, bbox.bottom()));
  painter->drawLine(QLineF(xright, bbox.top(), xright, bbox.bottom()));
  painter->restore();
}

void CurveRectangle::paint(QPainter * painter, const QRectF &)
{
  QRectF r = QRectF(p1, p2);
  if(r.isNull())
    return;
  painter->save();
  painter->setPen(pen);
  painter->setBrush(brush);
  painter->drawRect(r);
  painter->restore();
}

void CurveRectangle::setRect(const QRectF & r)
{
  p1 = r.topLeft();
  p2 = r.bottomRight();
}

QRectF CurveData::boundingRect() const
{
  double xmin = xvalues.min();
  double xmax = xvalues.max();
  double ymin = yvalues.min();
  double ymax = yvalues.max();

  return QRectF(QPointF(xmin, ymin), QPointF(xmax, ymax));
}

void CurveData::paint(QPainter * painter, const QRectF &)
{
  int nb = std::min(xvalues.size(), yvalues.size());
  if(nb < 2)
    return;                     // Not much to do, admittedly
  painter->save();
  painter->setPen(pen);
  QPointF first = QPointF(xvalues[0], yvalues[0]);
  for(int i = 1; i < nb; i++) {
    QPointF second = QPointF(xvalues[i], yvalues[i]);
    painter->drawLine(first, second);
    first = second;
  }
  painter->restore();
}
