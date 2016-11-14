/*
  curvevector.cc: implementation of the CurveVector class
  Copyright 2011 by Vincent Fourmond
            2014 by CNRS/AMU

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
#include <curvevector.hh>
#include <dataset.hh>
#include <vector.hh>

#include <pointiterator.hh>

CurveVector::~CurveVector()
{
}

QRectF CurveVector::boundingRect() const
{
  QRectF bbox = dataSet->boundingBox();
  double ymin = yValue(0);
  double ymax = yValue(0);

  // This will be really slow on large data sets.
  for(int i = 1; i < dataSet->nbRows(); i++) {
    ymin = std::min(ymin, yValue(i));
    ymax = std::max(ymax, yValue(i));
  }
  bbox.setTop(ymin);
  bbox.setBottom(ymax);
  return bbox;
}

double CurveVector::yValue(int i) const
{
  if(residuals)
    return dataSet->y()[i]- gsl_vector_get(&view.vector, i);
  else
    return gsl_vector_get(&view.vector, i);
}

// Vector CurveVector::yValues() const
// {
//   Vector ret;
//   int nb = dataSet->nbRows();
//   ret.reserve(nb);
//   for(int i = 0; i < nb; i++)
//     ret << yValue(i);
//   return ret;
// }

void CurveVector::paint(QPainter * painter, const QRectF &,
                        const QTransform & ctw)
{
  painter->setPen(pen);
  const Vector & x = dataSet->x();

  int nb = dataSet->nbRows();
  if(nb < 2)
    return;                     // Not much to do, admittedly
  painter->save();
  painter->setPen(pen);

  {
    QPainterPath pp;
    PointIterator it(&view.vector, dataSet, residuals);
    it.addToPath(pp, ctw);
    painter->drawPath(pp);
  }
  painter->restore();
}

QRect CurveVector::paintLegend(QPainter * p, const QRect & rect)
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
              tr("Fit"),
              &t);
  return t.united(tmp);
}
