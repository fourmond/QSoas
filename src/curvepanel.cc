/*
  curveview.cc: displaying 2D curves...
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

#include <vector.hh>
#include <curvepanel.hh>
#include <curveitem.hh>
#include <soas.hh>

#include <math.h>


CurvePanel::CurvePanel() : 
  bgLinesPen(QColor("#DDD"), 1.5, Qt::DashLine),
  drawingXTicks(true), drawingYTicks(true), drawingLegend(true)
{
  bgLinesPen.setCosmetic(true);
}

CurvePanel::~CurvePanel()
{
  /// @todo Ensure all the objects are deleted ?
}

QRectF CurvePanel::currentZoom() const
{
  /// @todo zoom stack
  return boundingBox;
}

void CurvePanel::invalidateTicks()
{
  xTicks.clear();
}

void CurvePanel::setGeometry(const QRect & rect)
{
  internalRectangle = rect;
}

void CurvePanel::computeTransform(const QRect & wR2,
                                  const QRectF & sR2)
{
  QRectF sR = sR2.normalized();
  QRect wR = wR2.normalized();

  double m11 = wR.width()/sR.width();
  double dx = wR.x() - sR.x() * m11;

  double m22 = -wR.height()/sR.height();
  double dy = -sR.bottom() * m22 + wR.top();

  transform = QTransform(m11, 0, 0, m22, dx, dy);
  reverseTransform = transform.inverted(); // That's inversible,
                                           // thanks.
  invalidateTicks();
}

void CurvePanel::computeTransform()
{
  QRect r = internalRectangle;
  QRectF z = currentZoom();
  computeTransform(r, z);
}

void CurvePanel::updateBB()
{
  boundingBox = QRectF();
  for(int i = 0; i < displayedItems.size(); i++) {
    CurveItem * item = displayedItems[i];
    if(item && item->countBB)
      boundingBox = boundingBox.united(item->boundingRect());
  }
}

static double naturalDistances[] = { 1.0, 2.0, /*2.5,*/ 5.0, 10.0 };
const int nbNaturalDistances = sizeof(naturalDistances)/sizeof(double);

/// This function tries to picks ticks reasonably.
///
/// This code comes from Tioga's axes.c file.
static Vector pickMajorTicksLocation(double min, double max, 
                                     double * tick, double tickMin)
{
  /* The factor by which you need to divide to get
     the tick_min within [1,10[ */
  double factor = pow(10, floor(log10(tickMin)));
  double norm_tick_min = tickMin/factor;
  bool done = false;

  /* In principle, the loop below show run at most twice, but a
     safeguard is not too expensive ;-)... */
  int nb_tries = 0;
   
  do {
    nb_tries ++;
    int i;
    for(i = 0; i < nbNaturalDistances; i++)
      if(naturalDistances[i] >= norm_tick_min)
        break;
    /* Now, there is a corner case when there is not enough */

    *tick = naturalDistances[i] * factor;
      
    /* If the there is room for at most one tick here, there is a
       problem, so take the size down. */
    if( (max - min) < 2.0 * *tick) {
      factor = pow(10, floor(log10(tickMin/2)));
      norm_tick_min = tickMin/(2*factor);
    }
    else
      done = true;
  } while(! done && nb_tries < 3);

   double first_tick = ceil(min /(*tick)) * (*tick);
   double last_tick = floor(max /(*tick)) * (*tick);

   Vector ret;
   int nb = (int)round((last_tick - first_tick)/(*tick));
   for (int i = 0; i <= nb ; i++)
     ret << first_tick + (*tick) * i;
  
  return ret;
}

void CurvePanel::pickTicks()
{
  QTransform t = transform;
  QRectF rect = currentZoom().normalized();
  {
    double minTick = std::max(fabs(50/t.m11()), rect.width()/8);
    double tick;
    xTicks = pickMajorTicksLocation(rect.left(), rect.right(), 
                                    &tick, minTick);
  }  
  {
    double minTick = std::max(fabs(40/t.m22()), rect.height()/8);
    double tick;
    yTicks = pickMajorTicksLocation(rect.top(), rect.bottom(), 
                                    &tick, minTick);
  }
}


void CurvePanel::paintCurves(QPainter * p)
{
  // First, setup the transformation
  QRectF r = currentZoom();
  if(xTicks.size() == 0)
    pickTicks();
    
  p->setTransform(transform);
  p->setWorldMatrixEnabled(true);

  /// 
  p->setPen(bgLinesPen); 
  for(int i = 0; i < xTicks.size(); i++) {
    double x = xTicks[i];
    p->drawLine(QLineF(x, r.bottom(), x, r.top()));
  }
  for(int i = 0; i < yTicks.size(); i++) {
    double y = yTicks[i];
    p->drawLine(QLineF(r.left(), y, r.right(), y));
  }

  for(int i = 0; i < displayedItems.size(); i++) {
    CurveItem * it = displayedItems[i];
    if(it)
      it->paint(p);
    else
      displayedItems.removeAt(i--); // autocleanup elements gone
                                    // astray.
  }
}

void CurvePanel::paint(QPainter * painter)
{
  // First, update the BB
  updateBB();
  

  // Draws the area
  computeTransform();
  QRect r2 = internalRectangle.adjusted(-2,-2,2,2);

  painter->eraseRect(r2);
  painter->save();
  painter->setClipRect(r2);
  paintCurves(painter);
  painter->restore();
  painter->drawRect(r2);

  // Now drawing tick labels
  if(drawingXTicks) {
    for(int i = 0; i < xTicks.size(); i++) {
      double x = xTicks[i];
      QPointF pt(x, 0);
      pt = transform.map(pt);
      pt.setY(r2.bottom() + 2); 
      QRectF textPos(pt, QSizeF(0,0));
      painter->drawText(textPos.adjusted(-5,2,5,5),
                        Qt::AlignHCenter | Qt::AlignTop | Qt::TextDontClip,
                        QString::number(x));
    }
  }

  if(drawingYTicks) {
    for(int i = 0; i < yTicks.size(); i++) {
      double y = yTicks[i];
      QPointF pt(0, y);
      pt = transform.map(pt);
      pt.setX(r2.left() -2); 
      QRectF textPos(pt, QSizeF(0,0));
      painter->drawText(textPos.adjusted(-15,-5,-2, 5),
                        Qt::AlignVCenter | Qt::AlignRight | Qt::TextDontClip,
                        QString::number(y));
    }
  }

  if(drawingLegend) {
    // Here the legend:
    int start = 3;
    for(int i = 0; i < displayedItems.size(); i++) {
      QRect r(start, 3, 40, 12);
      r = displayedItems[i]->paintLegend(painter, r);
      if(! r.isNull())
        start += r.width() + 3;
    }
  }
}



void CurvePanel::clear()
{
  for(int i = 0; i < displayedItems.size(); i++)
    delete displayedItems[i];
  displayedItems.clear();
  boundingBox = QRectF();
  invalidateTicks();
}

CurveItem * CurvePanel::closestItem(const QPointF &point, 
                                    double * distance) const
{
  double d = 0;
  CurveItem * item = NULL;
  for(int i = 0; i < displayedItems.size(); i++) {
    CurveItem * it = displayedItems[i];
    if(! it)
      continue;
    double d2 = it->distanceTo(point, transform.m11(),
                               transform.m22());
    if(d2 < 0)
      continue;

    if(! item || d2 < d) {
      item = it;
      d = d2;
    }
  }
  if(distance)
    *distance = d;
  return item;
}

void CurvePanel::addItem(CurveItem * item)
{
  displayedItems.append(QPointer<CurveItem>(item));
  /// @todo: update bounding box
}

QMargins CurvePanel::panelMargins() const
{
  return QMargins(drawingYTicks ? 50 : 10,
                  drawingLegend ? 20 : 10,
                  10, 
                  drawingXTicks ? 30 : 10);
}
