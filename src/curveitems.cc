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

#include <graphicssettings.hh>
#include <soas.hh>

#include <pointiterator.hh>

QRectF CurveLine::boundingRect() const
{
  return QRectF(p1, p2).normalized();
}

void CurveLine::paint(QPainter * painter, const QRectF &,
                      const QTransform & ctw)
{
  painter->save();
  painter->setPen(pen);
  painter->drawLine(ctw.map(QLineF(p1, p2)));
  painter->restore();
}

//////////////////////////////////////////////////////////////////////

void CurveVerticalLine::paint(QPainter * painter, const QRectF & bbox,
                              const QTransform & ctw)
{
  painter->save();
  painter->setPen(pen);
  painter->drawLine(ctw.map(QLineF(x, bbox.top(), x, bbox.bottom())));
  painter->restore();
}

void CurveVerticalLines::paint(QPainter * painter, const QRectF & bbox,
                               const QTransform & ctw)
{
  if(! xValues)
    return;
  painter->save();
  painter->setPen(pen);
  for(int i = 0; i < xValues->size(); i++) {
    double x = xValues->value(i);
    painter->drawLine(ctw.map(QLineF(x, bbox.top(), x, bbox.bottom())));
  }
  painter->restore();
}

//////////////////////////////////////////////////////////////////////

CurveHorizontalRegion::CurveHorizontalRegion() :
  autoSwap(false)
{
  leftPen = soas().graphicsSettings().getPen(GraphicsSettings::LeftSidePen);
  rightPen = soas().graphicsSettings().getPen(GraphicsSettings::RightSidePen);
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
  if(autoSwap && xleft > xright)
    std::swap(xleft, xright);
}

double CurveHorizontalRegion::xmin() const
{
  if(xleft > xright)
    return xright;
  return xleft;
}

double CurveHorizontalRegion::xmax() const
{
  if(xleft < xright)
    return xright;
  return xleft;
}

void CurveHorizontalRegion::getClosestIndices(const Vector & xvalues, int * left, int * right, bool ensureGrowing) const
{
  *left = xvalues.closestPoint(xleft);
  *right = xvalues.closestPoint(xright);
  if(ensureGrowing && (*left > *right))
    std::swap(*left, *right);
}


void CurveHorizontalRegion::paint(QPainter * painter, const QRectF & bbox,
                                  const QTransform & ctw)
{
  painter->save();
  painter->setPen(leftPen);
  painter->drawLine(ctw.map(QLineF(xleft, bbox.top(), 
                                   xleft, bbox.bottom())));
  painter->setPen(rightPen);
  painter->drawLine(ctw.map(QLineF(xright, bbox.top(), 
                                   xright, bbox.bottom())));
  painter->restore();
}

//////////////////////////////////////////////////////////////////////

void CurveRectangle::paint(QPainter * painter, const QRectF &,
                           const QTransform & ctw)
{
  QRectF r = QRectF(ctw.map(p1),ctw.map(p2));
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

//////////////////////////////////////////////////////////////////////

QRectF CurveData::boundingRect() const
{
  if(xvalues.size() == 0)
    return QRectF();
  double xmin = xvalues.min();
  double xmax = xvalues.max();
  double ymin = yvalues.min();
  double ymax = yvalues.max();

  return QRectF(QPointF(xmin, ymin), QPointF(xmax, ymax));
}

void CurveData::paint(QPainter * painter, const QRectF &,
                      const QTransform & ctw)
{
  int nb = std::min(xvalues.size(), yvalues.size());
  if(nb < 2)
    return;                     // Not much to do, admittedly
  painter->save();
  painter->setPen(pen);

  QPainterPath pp;
  PointIterator it(xvalues, yvalues, 
                   histogram ? PointIterator::Steps : PointIterator::Normal);
  it.addToPath(pp, ctw);
  painter->drawPath(pp);
  painter->restore();
}
