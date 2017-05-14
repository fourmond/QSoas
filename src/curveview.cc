/*
  curveview.cc: displaying 2D curves...
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
#include <graphicssettings.hh>
#include <datastack.hh>

#include <exceptions.hh>
#include <stylegenerator.hh>





CurveView::CurveView(QWidget * parent) : 
  QAbstractScrollArea(parent), nbStyled(0),
  eventLoop(NULL), paintMarkers(false),
  repaintDisabled(false), sideGround(QPalette::Window)
                                            
{
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::NoFrame);
  setCursor(Qt::CrossCursor);

  setOpenGL(soas().graphicsSettings().openGL());
}

CurveView::~CurveView()
{
}

void CurveView::doRepaint()
{
  if(repaintDisabled) {
    repaintRequested = true;
  }
  else {
    viewport()->update();
  }
}

void CurveView::disableUpdates()
{
  repaintDisabled = true;
  repaintRequested = false;
}

void CurveView::enableUpdates()
{
  if(! repaintDisabled)
    return;                     // Definitely nothing to do
  repaintDisabled = false;
  if(repaintRequested)
    doRepaint();
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

  if(soas().graphicsSettings().antiAlias()) {
    p.setRenderHints(QPainter::Antialiasing, true);
    p.setRenderHints(QPainter::HighQualityAntialiasing, true);
  }

  layOutPanels();
  

  if(sideGround != QPalette::NoRole) {
    QRect r = rect();
    const QPalette & pal= palette();
    p.fillRect(r, pal.brush(sideGround));
  }
  
  panel.paint(&p);
  for(int i = 0; i < additionalPanels.size(); i++) {
    additionalPanels[i]->paint(&p);
  }
}

QPen CurveView::penForNextCurve()
{
  return soas().graphicsSettings().dataSetPen(nbStyled++);
}

void CurveView::addItem(CurveItem * item)
{
  panel.addItem(item);
  doRepaint();
}

void CurveView::addDataSet(const DataSet * ds, StyleGenerator * gen)
{
  if(! ds)
    return;
  CurveDataSet * item = new CurveDataSet(ds);
  item->paintMarkers = paintMarkers;

  /// @todo Maybe the overall setup should be more complex than just
  /// choosing the pen ?
  if(gen)
    item->pen = gen->nextStyle();
  else
    item->pen = penForNextCurve();
  addItem(item);
  if(! currentDataSet)
    currentDataSet = item;
}

void CurveView::removeDataSet(const DataSet * ds)
{
  if(! ds)
    return;

  QList<CurveItem *> its;
  QList<CurvePanel*> pnls = allPanels();
  for(int i = 0; i < pnls.size(); i++)
    its += pnls[i]->items();

  for(int i = 0; i < its.size(); i++) {
    CurveDataSet * st = dynamic_cast<CurveDataSet *>(its[i]);
    if(st && st->displayedDataSet() == ds) {
      delete its[i];
      update();
    }
  }
}

void CurveView::clear()
{
  nbStyled = 0;
  panel.clear();
  doRepaint();
  currentDataSet = NULL;
}

void CurveView::showCurrentDataSet()
{
  showDataSet(soas().stack().currentDataSet());
}

void CurveView::showDataSet(const DataSet * ds, StyleGenerator * gen)
{
  clear();
  addDataSet(ds, gen);
}

void CurveView::addPanel(CurvePanel * panel)
{
  additionalPanels << panel;
}

void CurveView::setPanel(int i, CurvePanel * panel)
{
  additionalPanels[i] = panel;
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
  layOutPanels(rect());
}

void CurveView::layOutPanels(const QRect & r)
{
  /// @todo take out some margins in panel.panelMargins()
  /// and remove them directly here (the frame margins)
  int height = r.height();
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
  doRepaint();
}

void CurveView::nupPrint(QPrinter * printer, 
                         const QList<CurveView *> &views,
                         int cols, int rows, 
                         int individualHeight)
{
  /// @todo be more clever here
  if(individualHeight < 0)
    individualHeight = 400;

  // We divide the page rectangle
  QRect pageRect = printer->pageRect();
  int dx = pageRect.width()/cols;
  int dy = pageRect.height()/rows;
  
  QPainter painter;
  int nb = cols * rows;
  painter.begin(printer);
  for(int i = 0; i < views.size(); i++) {
    if(i && i % nb == 0)
      printer->newPage();
    
    int col = (i % nb) / rows;
    int row = i % rows;
    int xl = pageRect.left() + dx * col;
    int yt = pageRect.top() + dy * row;
    views[i]->render(&painter, individualHeight,
                     QRect(xl, yt, dx, dy));
  }
}

void CurveView::render(QPainter * painter,
                       int innerHeight, 
                       const QRect & targetRectangle,
                       const QString & title)
{
  painter->save();

  QPointF tl = targetRectangle.topLeft();
  QPointF br = targetRectangle.bottomRight();
  double ht = targetRectangle.height();

  // First, write the title
  if(! title.isEmpty()) {
    painter->save();
    QFont ft = painter->font();
    ft.setPointSize(20);
    painter->setFont(ft);
    QRectF tr;
    painter->drawText(targetRectangle.adjusted(0,2,0,0), 
                      Qt::AlignHCenter|Qt::AlignTop|
                      Qt::TextWordWrap, title, &tr);
    // We need to set a neat font ?
    tl.setY(tr.bottomLeft().y());
    ht = fabs(tl.y() - br.y());
    painter->restore();
  }

  double scale = innerHeight/ht;

  br *= scale;
  tl *= scale;


  QRect actual = QRect(QPoint(tl.x(), tl.y()), 
                       QPoint(br.x(), br.y()));

  layOutPanels(actual);


  // Do the drawing.
  painter->scale(1/scale, 1/scale);

  QList<CurvePanel *> panels = allPanels();
  for(int i = 0; i < panels.size(); i++)
    panels[i]->paint(painter);
  
  layOutPanels();
  painter->restore();
}


QPixmap CurveView::renderDatasetAsPixmap(const DataSet * dataset,
                                         const QSize & size)
{
  CurveView v;
  v.setBackgroundRole(QPalette::NoRole);
  v.sideGround = QPalette::NoRole;
  v.addDataSet(dataset);
  v.resize(size);
  
  return QPixmap::grabWidget(&v);
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
    
    doRepaint();
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
  savedProxy = focusProxy();
  setFocusProxy(NULL);
  setFocus();
  qApp->installEventFilter(eventLoop);
}

void CurveView::leaveLoop()
{
  if(! eventLoop)
    return;

  qApp->removeEventFilter(eventLoop);
  setFocusProxy(savedProxy);
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

QList<DataSet *> CurveView::displayedDataSets()
{
  QList<DataSet *> ret;
  QList<CurvePanel *> panels = allPanels();
  for(int i = 0; i < panels.size(); i++) {
    QList<CurveItem *> items = panels[i]->items();
    for(int j = 0; j < items.size(); j++) {
      CurveDataSet * cds = dynamic_cast<CurveDataSet *>(items[j]);
      if(cds) {
        const DataSet * ds = cds->displayedDataSet();
        if(ds)
          ret << const_cast<DataSet*>(ds);
      }
    }
  }
  return ret;
}
