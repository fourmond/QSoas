/*
  curvepanel.cc: displaying 2D curves...
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

#include <vector.hh>
#include <curvepanel.hh>
#include <curveitem.hh>
#include <curvedataset.hh>

#include <soas.hh>

#include <math.h>
#include <utils.hh>
#include <debug.hh>

#include <boundingbox.hh>

CurvePanel::CurvePanel() : 
  bgLinesPen(QColor("#DDD"), 1.5, Qt::DashLine),
  orgPen(QColor("#BBB"), 2.4, Qt::DashLine),
  xTracking(0),
  drawingXTicks(true), drawingYTicks(true), drawingLegend(true),
  anyZoom(false),  xLabel("X"), yLabel("Y"), stretch(100)
{
  bgLinesPen.setCosmetic(true);
}

CurvePanel::~CurvePanel()
{
  /// @todo Ensure all the objects are deleted ?
  clear();
}

QRectF CurvePanel::currentZoom() const
{
  // DO NOT USE isEmpty, as it will return false for NaN bounding boxes !
  // QTextStream o(stdout);
  QRectF bbox = (zoom.isValid() ? zoom : boundingBox);
  // o << "Zoom: ";
  // Utils::dumpRectangle(o, zoom);
  // o << "\nBB: ";
  // Utils::dumpRectangle(o, boundingBox);
  // o << "\nFinal: ";
  // Utils::dumpRectangle(o, bbox);
  // o << endl;
  if(xTracking) {
    QRectF trackedX = xTracking->currentZoom();
    bbox.setLeft(trackedX.left());
    bbox.setRight(trackedX.right());
  }
  // o << "Final BB: ";
  bbox = Utils::sanitizeRectangle(bbox);
  // Utils::dumpRectangle(o, bbox);
  // o << endl;                    
  return bbox;
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
  if(! std::isfinite(m11))      // Shouldn't happen ?
    m11 = 1;
  double dx = wR.x() - sR.x() * m11;

  double m22 = -wR.height()/sR.height();
  if(! std::isfinite(m22))      // Shouldn't happen too...
    m22 = 1;

  double dy = -sR.bottom() * m22 + wR.top();

  transform = QTransform(m11, 0, 0, m22, dx, dy);


  // We do the inversion manually, as Qt gets confused by very small
  // determinants, which may occur when the coordinates are
  // exceptionally large.
  double det = transform.determinant();
  reverseTransform = transform.adjoint() / det;

  invalidateTicks();
}

void CurvePanel::computeTransform()
{
  QRect r = internalRectangle;
  QRectF z = currentZoom();
  computeTransform(r, z);
}

QRectF CurvePanel::overallBB() const
{
  return boundingBox;
}

void CurvePanel::updateBB()
{
  BoundingBox bb;
  for(int i = 0; i < displayedItems.size(); i++) {
    CurveItem * item = displayedItems[i];
    if(item && item->countBB && ! item->hidden) {
      BoundingBox itemBB = BoundingBox(item->boundingRect());
      if(! itemBB.isNull())     // This is what should count ?
        bb.uniteWith(itemBB);
    }
  }
  boundingBox = bb;
}

static double naturalDistances[] = { 1.0, 2.0, /*2.5,*/ 5.0, 10.0 };
const int nbNaturalDistances = sizeof(naturalDistances)/sizeof(double);

/// This function tries to picks ticks reasonably.
///
/// This code comes from Tioga's axes.c file.
static Vector pickMajorTicksLocation(double min, double max, 
                                     double * tick, double tickMin,
                                     double * fact)
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

   *fact = factor;
  
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
                                    &tick, minTick, &xTicksFactor);
  }  
  {
    double minTick = std::max(fabs(40/t.m22()), rect.height()/8);
    if(minTick > rect.height()/3)
      minTick = rect.height()/4; // Try to have several ticks.
    double tick;
    yTicks = pickMajorTicksLocation(rect.top(), rect.bottom(), 
                                    &tick, minTick, &yTicksFactor);
  }
}


void CurvePanel::paintCurves(QPainter * p)
{
  // First, setup the transformation
  QRectF r = currentZoom();
  if(xTicks.size() == 0)
    pickTicks();

  
  /// p->setTransform(transform, true);

  for(int i = 0; i < xTicks.size(); i++) {
    double x = xTicks[i];
    if(x == 0.0)
      p->setPen(orgPen); 
    else
      p->setPen(bgLinesPen); 
    p->drawLine(transform.map(QLineF(x, r.bottom(), x, r.top())));
  }
  for(int i = 0; i < yTicks.size(); i++) {
    double y = yTicks[i];
    if(y == 0.0)
      p->setPen(orgPen); 
    else
      p->setPen(bgLinesPen); 
    p->drawLine(transform.map(QLineF(r.left(), y, r.right(), y)));
  }

  for(int i = 0; i < displayedItems.size(); i++) {
    CurveItem * it = displayedItems[i];
    if(it) {
      if(! it->hidden)
        it->paint(p, r, transform);
    }
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

  painter->fillRect(r2, QColor("white"));
  painter->save();
  painter->setClipRect(r2);
  paintCurves(painter);
  painter->restore();
  painter->drawRect(r2);

  // Now drawing tick labels

  if(drawingXTicks) {
    int bot = r2.bottom();
    for(int i = 0; i < xTicks.size(); i++) {
      double x = xTicks[i];
      QPointF pt(x, 0);
      pt = transform.map(pt);
      pt.setY(r2.bottom() + 2); 
      QRectF textPos(pt, QSizeF(0,0));
      QRectF br;
      painter->drawText(textPos.adjusted(-5,2,5,5),
                        Qt::AlignHCenter | Qt::AlignTop | Qt::TextDontClip,
                        QString::number(x), &br);
      if(br.bottom() > bot)
        bot = br.bottom();
    }
    if(! xLabel.isEmpty()) {
      QRect xl = r2;
      xl.setTop(bot+3);
      xl.setBottom(bot + 13);
      painter->drawText(xl,
                        Qt::AlignHCenter | Qt::AlignTop | Qt::TextDontClip,
                        xLabel);
      
    }
      
  }

  /// @todo I handle drawing tick labels with a common factor, but it
  /// would be interesting as well in some cases to extract an offset.
  ///
  /// @todo In real, I need a whole lot of functions to handle ticks
  /// position and tick label choice. Those should get factored for
  /// both X and Y ticks

  if(drawingYTicks) {
    QString realYLabel = yLabel;
    double fact = 1;
    double magnitude = std::max(fabs(yTicks.min()), fabs(yTicks.max()));
    double yf = pow(10, 3*floor(log10(magnitude)/3));
    if(yf < 0.1 || yf >= 1e3) {
      fact = 1/yf;
      realYLabel += QString(" (%1 %2)").
        arg(QChar(0xD7)
            // "scale:"
            ).arg(QString::number(1/fact, 'e', 0));
    }

    for(int i = 0; i < yTicks.size(); i++) {
      double y = yTicks[i];
      QPointF pt(0, y);
      pt = transform.map(pt);
      pt.setX(r2.left() -2); 
      QRectF textPos(pt, QSizeF(0,0));
      painter->drawText(textPos.adjusted(-15,-5,-2, 5),
                        Qt::AlignVCenter | Qt::AlignRight | Qt::TextDontClip,
                        QString::number(y*fact));
    }
    // Now more fun: Y label
    if(! realYLabel.isEmpty()) {
      painter->save();
      painter->translate(r2.left() - 40, 0.5*(r2.bottom() + r2.top()));
      painter->rotate(-90);
      QRect textPos(QPoint(0,0), QSize(0,0));
      painter->drawText(textPos.adjusted(-5,-5,5, 5),
                        Qt::AlignHCenter | Qt::AlignBottom | Qt::TextDontClip,
                        realYLabel);

      painter->restore();
    }
  }

  if(drawingLegend) {
    // Here the legend:
    int start = r2.left();
    for(int i = 0; i < displayedItems.size(); i++) {
      CurveItem * it = displayedItems[i];
      if(it && ! it->hidden) {
        QRect r(start, r2.top() - 15, 40, 12);
        r = displayedItems[i]->paintLegend(painter, r);
        if(! r.isNull())
          start += r.width() + 3;
      }
    }
  }
}



void CurvePanel::clear()
{
  for(int i = 0; i < displayedItems.size(); i++)
    delete displayedItems[i];
  displayedItems.clear();
  boundingBox = QRectF();
  zoom = QRectF();
  invalidateTicks();

  xLabel = "X";
  yLabel = "Y";
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
}

QMargins CurvePanel::panelMargins() const
{
  return QMargins(drawingYTicks ? 70 : 10,
                  drawingLegend ? 20 : 10,
                  15, 
                  drawingXTicks ? 40 : 10);
}

QPointF CurvePanel::scaleFactors() const
{
  return QPointF(transform.m11(), transform.m22());
}

void CurvePanel::setZoom(const QRectF & recti, bool force)
{
  // Force an update of the bounding box
  updateBB();
  QRectF rect = recti;
  if(boundingBox.isValid() && !(anyZoom || force)) {
    // 10% around
    double dx = fabs(rect.width() * 0.05);
    double dy = fabs(rect.height() * 0.05);

    // not very satisfying...
    // We need to implement a drag stuff...
    QRectF bb = boundingBox.normalized().adjusted(-dx, -dy, dx, dy);

    if(rect.contains(bb))
      rect = QRectF();            // Disable zoom then.
    else {
      // We try to be smart and take up to 10% of the zoom around the
      // full bounding box.
      rect = bb.intersected(rect);
    }
  }
  zoom = rect;

  // Here, forward to Xtracking
  if(xTracking) {
    QRectF trackedZoom = xTracking->currentZoom();
    QRectF eff;
    if(zoom.isValid())
      eff = zoom;
    else
      eff = xTracking->boundingBox;
    trackedZoom.setLeft(eff.left());
    trackedZoom.setRight(eff.right());
    xTracking->setZoom(trackedZoom);
  }
}

void CurvePanel::zoomIn(const QPointF & point, double by)
{
  double factor = pow(1.3, - by);
  setZoom(Utils::scaledAround(currentZoom(), point, factor, factor));
}

void CurvePanel::zoomIn(const QPointF & point, 
                        Qt::Orientation orient,
                        double by)
{
  double factor = pow(1.3, - by);
  setZoom(Utils::scaledAround(currentZoom(), point, 
                              (orient == Qt::Horizontal ? factor : 1),
                              (orient == Qt::Vertical ? factor : 1)));
}

void CurvePanel::resetZoom()
{
  setZoom(QRectF());
}

void CurvePanel::zoomIn(const QRectF & rect, bool force)
{
  setZoom(rect, force);
}

void CurvePanel::setYRange(double ymin, double ymax, 
                           CurvePanel * panel)
{
  QRectF bbox = currentZoom(); /// X values are meaningless, unless
                               /// panel is NULL
  bbox.setTop(ymin);
  bbox.setBottom(ymax);
  zoomIn(bbox);
  xTracking = (panel == this ? NULL : panel); // Avoid infinite recursion
}

void CurvePanel::render(QPainter * painter,
                        int innerPanelHeight, 
                        const QRect & targetRectangle,
                        const QString & title)
{
  QMargins m = panelMargins();
  if(! title.isEmpty())
    m.setTop(m.top() * 2);
  int totalHeight = m.top() + m.bottom() + innerPanelHeight;
  
  // Now compute the outer rectangle
  QPointF tl = targetRectangle.topLeft();
  QPointF br = targetRectangle.bottomRight();
  double scale = totalHeight/(targetRectangle.height()*1.0);
  br *= scale;
  tl *= scale;
  QRect finalWidgetRect = QRect(QPoint(tl.x(), tl.y()), 
                                QPoint(br.x(), br.y()));
  QRect innerRect = Utils::applyMargins(finalWidgetRect, m);

  QRect savedRect = internalRectangle;

  painter->scale(1/scale, 1/scale);
  internalRectangle = innerRect;
  paint(painter);
  // dropping title for now.
  internalRectangle = savedRect;
}

QList<CurveItem *> CurvePanel::items()
{
  QList<CurveItem *> ret;
  for(int i = 0; i < displayedItems.size(); i++) {
    CurveItem * it = displayedItems[i];
    if(! it)
      continue;
    ret << it;
  }
  return ret;
}

QList<DataSet *> CurvePanel::displayedDataSets() const
{
  QList<DataSet *> ret;
  for(int i = 0; i < displayedItems.size(); i++) {
    CurveItem * it = displayedItems[i];
    CurveDataSet * cds = dynamic_cast<CurveDataSet *>(it);
    if(cds) {
      const DataSet * ds = cds->displayedDataSet();
      if(ds)
        ret << const_cast<DataSet*>(ds);
    }
  }
  return ret;
}

CurveDataSet * CurvePanel::findDataSet(const DataSet * dataset) const
{
  if(! dataset)
    return NULL;
  for(int i = displayedItems.size() - 1; i >= 0; i--) {
    CurveItem * it = displayedItems[i];
    CurveDataSet * cds = dynamic_cast<CurveDataSet *>(it);
    if(cds) {
      const DataSet * ds = cds->displayedDataSet();
      if(ds == dataset)
        return cds;
    }
  }
  return NULL;
}
