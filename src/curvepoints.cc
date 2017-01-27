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

#include <utils.hh>


CurvePoints::CurvePoints(gsl_vector * xv, gsl_vector * yv,
                         gsl_vector * err) :
  xvalues(xv), yvalues(yv), errors(err),
  size(3.0), errorBarSize(6), type(CurveMarker::Circle) {
}

void CurvePoints::drawErrorBar(QPainter * painter, double x,
                               double y, double err,
                               const QTransform & ctw) const
{
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
  for(int i = 0; i < xvalues->size; i++) {
    double x = gsl_vector_get(xvalues, i);
    double y = gsl_vector_get(yvalues, i);

    
    // first draw the errors if applicable
    if(errors)
      drawErrorBar(painter, x, y, gsl_vector_get(errors, i), ctw);

    QPointF pt(x, y);
    CurveMarker::paintMarker(painter, ctw.map(pt), type, size);
  }
  painter->restore();
}

QRectF CurvePoints::boundingRect() const
{
  QRectF rv;
  if(xvalues->size < 1)
    return rv;

  double xmin = gsl_vector_min(xvalues);
  double xmax = gsl_vector_max(xvalues);

  double ymin = gsl_vector_min(yvalues);
  double ymax = gsl_vector_max(yvalues);

  rv = QRectF(QPointF(xmin, ymin), QPointF(xmax, ymax));
    
  QTextStream o(stdout);
  o << "Bounding rect: ";
  Utils::dumpRectangle(o, rv);
  o << "\nSize: " << xvalues->size << endl;
  

  return rv;
}
