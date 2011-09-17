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
#include <curveitem.hh>
#include <curvedataset.hh>

#include <math.h>

#include <mainwin.hh>
#include <curveeventloop.hh>
#include <dataset.hh>

#include <soas.hh>
#include <datastack.hh>

#include <exceptions.hh>

CurveView::CurveView() : 
  eventLoop(NULL), paintMarkers(false)
                                            
{
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::NoFrame);
  setCursor(Qt::CrossCursor);

  setOpenGL(soas().openGL());
}

CurveView::~CurveView()
{
}

void CurveView::setOpenGL(bool b)
{
  QWidget * w;
  if(b)
    w = new QGLWidget;
  else
    w = new QWidget;

  // We want mouse tracking !
  w->setMouseTracking(true);
  setViewport(w);
}

void CurveView::resizeEvent(QResizeEvent * /*event*/)
{
  viewport()->setGeometry(rect());
}

void CurveView::paintEvent(QPaintEvent * /*event*/)
{
  QPainter p(viewport());

  if(soas().antiAlias()) {
    p.setRenderHints(QPainter::Antialiasing, true);
    p.setRenderHints(QPainter::HighQualityAntialiasing, true);
  }

  layOutPanels();
  

  QRect r = rect();
  const QPalette & pal= palette();
  p.fillRect(r, pal.brush(QPalette::Window));

  panel.paint(&p);
  for(int i = 0; i < additionalPanels.size(); i++) {
    additionalPanels[i]->paint(&p);
  }
}

static const char * colors[] = 
  { "red", "blue", "#080", "orange", "purple" };
static int nbColors = sizeof(colors)/sizeof(colors[0]);

QPen CurveView::penForNextCurve()
{
  /// @todo and what about
  QPen p(QColor(colors[nbStyled++ % nbColors]), 1.2);
  return p;
}

void CurveView::addItem(CurveItem * item, bool doRepaint)
{
  panel.addItem(item);
  if(doRepaint)
    repaint();
}

void CurveView::addDataSet(const DataSet * ds, bool doRepaint)
{
  if(! ds)
    return;
  CurveDataSet * item = new CurveDataSet(ds);
  item->paintMarkers = paintMarkers;
  item->pen = penForNextCurve();
  addItem(item, doRepaint);
}

void CurveView::clear()
{
  nbStyled = 0;
  panel.clear();
  viewport()->repaint();
}

void CurveView::showCurrentDataSet()
{
  showDataSet(soas().stack().currentDataSet());
}

void CurveView::showDataSet(const DataSet * ds)
{
  clear();
  addDataSet(ds);
}

void CurveView::addPanel(CurvePanel * panel)
{
  additionalPanels << panel;
}

QList<CurvePanel*> CurveView::allPanels()
{
  QList<CurvePanel *> panels;
  panels << &panel;
  
  for(int i = 0; i < additionalPanels.size(); i++) {
    CurvePanel * p = additionalPanels[i];
    if(p)
      panels << p;
    else
      additionalPanels.removeAt(i--);
  }
  return panels;
}


void CurveView::layOutPanels()
{
  /// @todo take out some margins in panel.panelMargins()
  /// and remove them directly here (the frame margins)
  QRect r = rect();
  int height = rect().height();
  int leftm = 0; 
  int rightm = 0;
  QList<CurvePanel *> panels = allPanels();
  QList<QMargins> margins;

  int totalStretch = 0;
  // We lay the panels out vertically

  for(int i = 0; i < panels.size(); i++) {
    QMargins m = panels[i]->panelMargins();
    height -= m.top() + m.bottom();
    margins << m;
    if(leftm < m.left())
      leftm = m.left();
    if(rightm < m.right())
      rightm = m.right();

    totalStretch += panels[i]->stretch;
  }

  height = std::max(height, 3*panels.size());
  int top = 0;

  for(int i = 0; i < panels.size(); i++) {
    CurvePanel * p = panels[i];
    const QMargins & m = margins[i];
    int h = (height * p->stretch)/totalStretch;
    QRect pr = r.adjusted(leftm, top + m.top(), 
                          -rightm, 0);
    pr.setBottom(pr.top() + h);
    p->setGeometry(pr);
    top += h + m.bottom();;
  }
}

void CurveView::setPaintMarkers(bool enabled)
{
  QList<CurvePanel *> panels = allPanels();
  paintMarkers = enabled;
  for(int i = 0; i < panels.size(); i++) {
    QList<CurveItem *> items = panels[i]->items();
    for(int j = 0; j < items.size(); j++) {
      CurveDataSet * d = dynamic_cast<CurveDataSet *>(items[j]);
      if(d)
        d->paintMarkers = paintMarkers;
    }
  }
  repaint();
}


//////////////////////////////////////////////////////////////////////
/// Event-related functions.

bool CurveView::event(QEvent * event)
{
  switch(event->type()) {
  case QEvent::ToolTip: 
    helpEvent(dynamic_cast<QHelpEvent *>(event));
    return true;
  default:
    return QAbstractScrollArea::event(event);
  }
}

void CurveView::helpEvent(QHelpEvent * event)
{
  if(panel.contains(event->pos())) {
    QPointF point = panel.fromWidget(event->pos());
    double dist;
    CurveItem * item = panel.closestItem(point, &dist);
    QString str;
    if(item && dist < 20)
      str = item->toolTipText(point);
    /// @todo more fancy: higlight the actual data point.
    QToolTip::showText(event->globalPos(), str, this);
  }
  
}

void CurveView::mouseMoveEvent(QMouseEvent * event)
{
  if(panel.contains(event->pos())) {
    QPointF f = panel.fromWidget(event->pos());
    soas().showMessage(tr("X: %1, Y: %2").
                       arg(f.x()).arg(f.y()));
  }
  QAbstractScrollArea::mouseMoveEvent(event);
}

void CurveView::mousePressEvent(QMouseEvent * event)
{
  QAbstractScrollArea::mousePressEvent(event);
}

void CurveView::mouseReleaseEvent(QMouseEvent * event)
{
  QAbstractScrollArea::mouseReleaseEvent(event);
}

void CurveView::wheelEvent(QWheelEvent * event)
{
  CurvePanel * panel = panelAt(event->pos());
  if(panel) {
    event->accept();
    QPointF pos = panel->fromWidget(event->pos());

    /// @todo This provides basic zooming facilities. I would like
    /// also to provide zooming using a context menu, or a drag event?
    double by = 0.5*event->delta()/120.0;
    if(! event->modifiers())
      panel->zoomIn(pos, event->orientation(), by);
    else if(event->modifiers() == Qt::ShiftModifier)
      panel->zoomIn(pos, Qt::Horizontal, by);
    else if(event->modifiers() == Qt::ControlModifier)
      panel->zoomIn(pos, by);
    else if(event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
      panel->resetZoom();
    
    repaint();
    return;
  }
  QAbstractScrollArea::wheelEvent(event);
}

void CurveView::keyPressEvent(QKeyEvent * event)
{
  QAbstractScrollArea::keyPressEvent(event);
}

void CurveView::enterLoop(CurveEventLoop * loop)
{
  if(eventLoop)
    throw InternalError("Attempting to attach a second loop to the same view");
  if(! loop)
    return;

  eventLoop = loop;
  qApp->installEventFilter(eventLoop);
}

void CurveView::leaveLoop()
{
  if(! eventLoop)
    return;

  qApp->removeEventFilter(eventLoop);
  eventLoop = NULL;
}

CurvePanel * CurveView::panelAt(const QPoint & pos)
{
  QList<CurvePanel *> panels = allPanels();
  for(int i = 0; i < panels.size(); i++)
    if(panels[i]->contains(pos))
      return panels[i];
  return NULL;
}
