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
#include <curveview.hh>

#include <vector.hh>
#include <terminal.hh>
#include <utils.hh>

#include <math.h>

CurveView::CurveView(QGraphicsScene * sc) : QGraphicsView(sc)
{
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::NoFrame);
  connect(sc, SIGNAL(sceneRectChanged(const QRectF &)),
          SLOT(updateSceneRect(const QRectF &)));

  /// @todo customize this
  // setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);
  // Only turn that on with openGL 

  setTransformationAnchor(QGraphicsView::NoAnchor);
  setResizeAnchor(QGraphicsView::NoAnchor);
  setAlignment(Qt::AlignLeft | Qt::AlignTop);

  /// The approach by setViewportMargins is interesting, excepted for
  /// the fact that it will greatly complicate printing, while
  // setViewportMargins(20,30,0,0);
}

CurveView::~CurveView()
{
}

QRect CurveView::internalRectangle() const
{
  return rect().normalized().adjusted(30, 10, -10, -30);
}

QRectF CurveView::currentZoom() const
{
  /// @todo zoom stack
  return scene()->sceneRect();
}

void CurveView::invalidateTicks()
{
  xTicks.clear();
}

void CurveView::setTransform(const QRect & wR,
                             const QRectF & sR)
{
  QTransform t;
  t.scale(wR.width()/sR.width(), -wR.height()/sR.height());
  QGraphicsView::setTransform(t);

  QRect r = rect();
  r.translate(-wR.x(), -wR.y());

  // Reset internal translations to QGraphicsView
  setSceneRect(sR);
  QRectF r2 = mapToScene(r).boundingRect();
  setSceneRect(r2);

  invalidateTicks();
}

void CurveView::setTransform()
{
  QRect r = internalRectangle();
  QRectF z = currentZoom();
  setTransform(r, z);
}

void CurveView::zoomTo(const QRectF &z)
{
  /// @todo zoom stack
  setTransform();
}

void CurveView::updateSceneRect(const QRectF &z)
{
  if(zoomStack.size() > 0)
    return;                     // Don't update view
  setTransform();
}

void CurveView::resizeEvent(QResizeEvent * event)
{
  QGraphicsView::resizeEvent(event);
  setTransform();
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

void CurveView::pickTicks()
{
  QTransform t = transform();
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

void CurveView::drawBackground(QPainter * painter, const QRectF & /*rect*/)
{
  QRectF r = currentZoom();
  if(xTicks.size() == 0)
    pickTicks();
  
  painter->setPen(QPen("red"));
  for(int i = 0; i < xTicks.size(); i++) {
    double x = xTicks[i];
    painter->drawLine(QLineF(x, r.bottom(), x, r.top()));
  }
  for(int i = 0; i < yTicks.size(); i++) {
    double y = yTicks[i];
    painter->drawLine(QLineF(r.left(), y, r.right(), y));
  }
}

/// Draws a filled frame inside \p rect but outside \p inner.
///
/// @todo move to Utils ?
static void drawFrame(QPainter * p,
                      const QRectF& outer, const QRectF & inner,
                      const QPen & pen, const QBrush & brush)
{
  QRectF o = outer.normalized();
  QRectF i = inner.normalized();
  i = i.intersected(o);

  // Left:
  QRectF r = QRectF(o.topLeft(), QSizeF(i.left() - o.left(), o.height()));
  p->fillRect(r, brush);
  // right
  r = QRectF(o.topRight(), QSizeF(i.right() - o.right(), o.height()));
  p->fillRect(r, brush);
  // bottom
  r = QRectF(o.topLeft(), QSizeF(o.width(), i.top() - o.top()));
  p->fillRect(r, brush);
  r = QRectF(o.bottomLeft(), QSizeF(o.width(), i.bottom() - o.bottom()));
  p->fillRect(r, brush);

  p->save();
  p->setPen(pen);
  p->drawRect(i);
  p->restore();
}

void CurveView::drawForeground(QPainter * painter, const QRectF & /*rect*/)
{
  QRect r = rect();
  QRectF outer = mapToScene(r).boundingRect();
  QRectF inner = currentZoom();
  const QPalette & p = palette();
  ::drawFrame(painter, outer, inner, QPen(), 
              QColor("red")
              //p.brush(QPalette::Window)
              );
}
