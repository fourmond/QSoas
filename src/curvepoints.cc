/*
  curvepoints.cc: implementation of CurvePoints
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
#include <curvepoints.hh>

#include <xyiterable.hh>

#include <utils.hh>


CurvePoints::CurvePoints(XYIterable * src) :
  source(src),
  size(3.0), errorBarSize(6), type(CurveMarker::Circle) {
}

CurvePoints::~CurvePoints()
{
  delete source;
}

void CurvePoints::drawErrorBar(QPainter * painter, double x,
                               double y, double err,
                               const QTransform & ctw) const
{
  if(err == 0)
    return;                     // not drawing absent error bars
  if(relativeErrorBar)
    err *= y;
  double yp = y + err;
  double ym = y - err;

  QPointF np(x, yp);
  np = ctw.map(np);

  QPointF nm(x, ym);
  nm = ctw.map(nm);

  painter->drawLine(np, nm);

  QPointF lft = np;
  lft.setX(lft.x() - errorBarSize);

  QPointF rght = np;
  rght.setX(rght.x() + errorBarSize);

  painter->drawLine(lft, rght);

  lft = nm;
  lft.setX(lft.x() - errorBarSize);

  rght = nm;
  rght.setX(rght.x() + errorBarSize);

  painter->drawLine(lft, rght);
}



void CurvePoints::paint(QPainter * painter, const QRectF &,
                        const QTransform & ctw)
{
  painter->save();
  painter->setPen(pen);
  painter->setBrush(brush);
  double x,y,err;
  source->reset();
  while(source->nextWithErrors(&x, &y, &err)) {
    drawErrorBar(painter, x, y, err, ctw);

    QPointF pt(x, y);
    CurveMarker::paintMarker(painter, ctw.map(pt), type, size);
  }
  painter->restore();
}

QRectF CurvePoints::boundingRect() const
{
  return source->boundingBox();
}

DataSet * CurvePoints::makeDataSet()
{
  return source->makeDataSet();
}
