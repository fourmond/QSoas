/*
  curvemarker.cc: implementation of the useful CurveItem children
  Copyright 2011 by Vincent Fourmond
            2013 by CNRS/AMU

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

#include <utils.hh>

void CurveMarker::paintMarker(QPainter * painter, const QPointF & realPos,
                              MarkerType type, double size)
{
  if(realPos.x() != realPos.x() || // NaN != NaN
     realPos.y() != realPos.y())
    return;                     // Do not attempt to draw anything.
  switch(type) {
  case Circle:
    painter->drawEllipse(realPos, size, size);
    break;
  default:
    ;
  }
}


void CurveMarker::paintMarkerLabel(QPainter * painter, const QPointF & realPos,
                                   MarkerType /*type*/, double size, 
                                   const QString & str, const QPen & pen)
{
  if(realPos.x() != realPos.x() || // NaN != NaN
     realPos.y() != realPos.y())
    return;                     // Do not attempt to draw anything.
  QPointF a(realPos);

  // Draw label at the bottom right
  a += QPointF(size, size);
  QRectF r(a, QSizeF(50,50));
  painter->save();
  painter->setPen(pen);
  painter->drawText(r, Qt::AlignLeft|Qt::AlignTop|Qt::TextDontClip, str);
  painter->restore();
}


void CurveMarker::paint(QPainter * painter, const QRectF &,
                        const QTransform & ctw)
{
  painter->save();
  painter->setPen(pen);
  painter->setBrush(brush);
  if(points.size() == 0 && xvalues.size() == 0) {
    QPointF pos = ctw.map(p);
    paintMarker(painter, pos, type, size);
    if(! l.isEmpty())
      paintMarkerLabel(painter, pos, type, size, l);
  }
  else {
    if(points.size() == 0) {
      for(int i = 0; i < xvalues.size(); i++) {
        QPointF np(xvalues[i], yvalues[i]);
        QPointF pos = ctw.map(np);
        paintMarker(painter, pos, type, size);
        if(labels.size() > i && (! labels[i].isEmpty()))
          paintMarkerLabel(painter, pos, type, size, 
                           labels[i], textPen);
      }
    }
    else {
      for(int i = 0; i < points.size(); i++) {
        QPointF pos = ctw.map(points[i]);
        paintMarker(painter, pos, type, size);
        if(labels.size() > i && (! labels[i].isEmpty()))
          paintMarkerLabel(painter, pos, type, size, 
                           labels[i], textPen);
      }
    }
  }
  painter->restore();
}

QRectF CurveMarker::boundingRect() const
{
  QRectF rv;
  if(points.size() == 0 && xvalues.size() == 0) {
    rv = QRectF(p, QSizeF(0,0));
  }
  else {
    if(points.size() == 0) {
      rv = QRectF(QPointF(xvalues.min(), yvalues.min()),
                  QPointF(xvalues.max(), yvalues.max()));
    }
    else {
      rv = QRectF(points[0], points[0]);
      for(int i = 1; i < points.size(); i++)
        rv = rv.united(QRectF(points[i], points[i]));
    }
  }
  return rv;
}
