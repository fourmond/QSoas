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
#include <curvemarker.hh>
#include <dataset.hh>

#include <soas.hh>
#include <datastack.hh>

#include <graphicssettings.hh>

#include <pointiterator.hh>

CurveDataSet::~CurveDataSet()
{
}

QRectF CurveDataSet::boundingRect() const
{
  if(dataSet)
    return dataSet->boundingBox();
  else
    return QRectF();
}

void CurveDataSet::paint(QPainter * painter, const QRectF &bbox,
                         const QTransform & ctw)
{
  if(! dataSet)
    return;
  if(dataSet->nbColumns() < 2)
    return;

  painter->save();
  const GraphicsSettings & gs = soas().graphicsSettings();
  if(dataSet->options.hasYErrors(dataSet)) {
    // paint it first so that lines/markers show up on top.
    QPen p(pen);
    QColor c = p.color();
    /// @todo make that customizable
    c.setAlphaF(0.2);
    p.setColor(c);
    painter->setPen(p);
    QPainterPath pp;
    PointIterator it(dataSet, PointIterator::Errors);
    it.addToPath(pp, ctw);
    painter->fillPath(pp, QBrush(c));
  }
  {
    painter->setPen(pen); 
    QPainterPath pp;
    PointIterator it(dataSet, dataSet->options.histogram ? 
                     PointIterator::Steps : PointIterator::Normal);
    it.addToPath(pp, ctw);
    painter->drawPath(pp);
  }
  if(paintMarkers) {
    PointIterator it(dataSet);
    while(it.hasNext())
      CurveMarker::paintMarker(painter, it.next(ctw),
                               CurveMarker::Circle, 3);
  }

  painter->setPen(gs.getPen(GraphicsSettings::SegmentsPen));
  // Then, we paint the segments if applicable
  Vector segments = dataSet->segmentPositions();
  for(int i = 0; i < segments.size(); i++)
    // @todo find a way to display the index of the segment !
    // (left/right), ie: 1 | 2
    painter->drawLine(ctw.map(QLineF(segments[i], bbox.top(), 
                                     segments[i], bbox.bottom())));
  painter->restore();

}

QRect CurveDataSet::paintLegend(QPainter * p, const QRect & rect)
{
  if(! dataSet)
    return QRect();
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
  QString txt = dataSet->name;
  {
    int i;
    if(soas().stack().indexOf(dataSet, &i))
      txt += QString(" (#%1)").arg(i);
  }
  p->drawText(r, 
              Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip,
              txt,
              &t);
  return t.united(tmp);
}

double CurveDataSet::distanceTo(const QPointF & point, 
                                double xscale,
                                double yscale)
{
  if(! dataSet)
    return -1;
  QPair<double, int> pair = dataSet->distanceTo(point, xscale, yscale);
  lastPoint = point;
  lastPointIdx = pair.second;
  return pair.first;
}

QString CurveDataSet::toolTipText(const QPointF & pt)
{
  if(! dataSet)
    return QString();
  if(lastPointIdx < 0 || pt != lastPoint)
    return QString();
  QString str;
  int idx;
  QString dsIdx;
  if(soas().stack().indexOf(dataSet, &idx))
    dsIdx = QString(" #%1").arg(idx);
  str += tr("Dataset%2: %1<br>").
    arg(dataSet->name).arg(dsIdx);
  QPointF p = dataSet->pointAt(lastPointIdx);
  str += tr("Point #%1: <br>X = %2, Y = %3").
    arg(lastPointIdx).arg(p.x()).arg(p.y());
  return str;
}


const DataSet * CurveDataSet::displayedDataSet() const
{
  return dataSet;
}
