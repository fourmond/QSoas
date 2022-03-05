/*
  curvedataset.cc: implementation of the CurveDataSet class
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014 by CNRS/AMU

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

CurveDataSet::CurveDataSet(const DataSet * ds) :
  CurveItem(true), dataSet(ds), lastPointIdx(-1),
  paintMarkers(false), tryPaintLines(true)
{
}

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
  QList<int> nans;
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

  painter->setPen(pen); 
  if(tryPaintLines && dataSet->options.shouldDrawLines(dataSet)) {
    QPainterPath pp;
    PointIterator it(dataSet, dataSet->options.histogram ? 
                     PointIterator::Steps : PointIterator::Normal);
    it.addToPath(pp, ctw);
    painter->drawPath(pp);
    nans = it.nonFinitePoints;
  }

  if(paintMarkers || dataSet->options.shouldDrawMarkers(dataSet) || dataSet->nbRows() <= 1) {
    PointIterator it(dataSet);
    while(it.hasNext())
      CurveMarker::paintMarker(painter, it.next(ctw),
                               CurveMarker::Circle, 2.5);
    nans = it.nonFinitePoints;
  }

  // Now, draw the NaNs
  if(nans.size() > 0) {
    painter->save();
    QList<QPair<int, int> > pos;
    QPair<int, int> cur(-1, -1);
    for(int i : nans) {
      if(cur.first < 0) {
        cur.first = i;
        cur.second = i;
      }
      else {
        if(i > cur.second+1) {
          pos << cur;
          cur.first = i;
          cur.second = i;
        }
        else {
          cur.second = i;
        }
      }
    }
    pos << cur;

    // QBrush b(pen.color().lighter());
    painter->setBrush(QBrush(pen.color().lighter()));

    for(const QPair<int, int> & nan : pos) {
      QPointF left, right;
      if(nan.first > 0)
        left = QPointF(dataSet->x()[nan.first-1],
                       dataSet->y()[nan.first-1]);
      if(nan.second < dataSet->nbRows() - 1)
        right = QPointF(dataSet->x()[nan.second+1],
                        dataSet->y()[nan.second+1]);
      if(nan.first == 0)
        left = right;
      if(nan.second == dataSet->nbRows() - 1)
        right = left;
      left += right;
      left *= 0.5;
      CurveMarker::paintMarker(painter, ctw.map(left),
                               CurveMarker::XShape, 12);
    }
    painter->restore();
  }

  painter->setPen(gs.getPen(GraphicsSettings::SegmentsPen));
  // Then, we paint the segments if applicable
  Vector segments = dataSet->segmentPositions();
  for(int i = 0; i < segments.size(); i++) {
    QPen p = painter->pen();
    p.setColor(i % 2 ? QColor("darkBlue") : QColor(229,140,20));
    painter->setPen(p);
    QLineF line(ctw.map(QLineF(segments[i], bbox.top(), 
                               segments[i], bbox.bottom())));
    painter->drawLine(line);

    QRectF r(line.p2() + QPointF(-6,15), QSizeF(0,0));
    painter->drawText(r, Qt::AlignRight | Qt::AlignVCenter | Qt::TextDontClip,
                      QString("%1").arg(i));
    r = QRectF(line.p2() + QPointF(6,15), QSizeF(0,0));
    painter->drawText(r, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip,
                      QString("%1").arg(i+1));
  }
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
  if(! dataSet || dataSet->nbColumns() < 2)
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
  str += QString("Dataset%2: %1<br>").
    arg(dataSet->name).arg(dsIdx);
  // display of perpendicular coordinates
  if(dataSet->perpendicularCoordinates().size() > 0)
    str += QString("%1 (perpendicular coordinate): %2<br>").
      arg(dataSet->perpendicularCoordinatesName()).
      arg(dataSet->perpendicularCoordinates()[0]);

  QPointF p = dataSet->pointAt(lastPointIdx);
  str += tr("Point #%1: <br>X = %2, Y = %3").
    arg(lastPointIdx).arg(p.x()).arg(p.y());
  QString n = dataSet->mainRowNames().value(lastPointIdx, "");
  if(! n.isEmpty())
    str += QString("<br>Row name: %1").arg(n);
  return str;
}


const DataSet * CurveDataSet::displayedDataSet() const
{
  return dataSet;
}

bool CurveDataSet::shouldBeCached() const
{
  return true;
}
