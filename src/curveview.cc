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

CurveView::CurveView() : 
  eventLoop(NULL)
                                            
{
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::NoFrame);

  setMouseTracking(true);
}

CurveView::~CurveView()
{
}


void CurveView::resizeEvent(QResizeEvent * /*event*/)
{
  viewport()->setGeometry(rect());
}

void CurveView::paintEvent(QPaintEvent * /*event*/)
{
  QPainter p(viewport());

  if(soas().antiAlias())
    p.setRenderHints(QPainter::Antialiasing, true);

  layOutPanels();
  

  QRect r = rect();
  const QPalette & pal= palette();
  p.fillRect(r, pal.brush(QPalette::Window));

  panel.paint(&p);
}

static const char * colors[] = 
  { "red", "blue", "#080", "orange", "black" };
static int nbColors = sizeof(colors)/sizeof(colors[0]);

QPen CurveView::penForNextCurve()
{
  /// @todo and what about
  QPen p(QColor(colors[nbStyled++ % nbColors]), 1.2);
  p.setCosmetic(true);
  return p;
}

void CurveView::addItem(CurveItem * item)
{
  panel.addItem(item);
  viewport()->repaint();
}

void CurveView::addDataSet(const DataSet * ds)
{
  CurveDataSet * item = new CurveDataSet(ds);
  item->pen = penForNextCurve();
  addItem(item);
}

void CurveView::clear()
{
  nbStyled = 0;
  panel.clear();
  viewport()->repaint();
}

void CurveView::showDataSet(const DataSet * ds)
{
  clear();
  addDataSet(ds);
}

void CurveView::layOutPanels()
{
  QRect r = rect();
  QMargins m = panel.panelMargins();
  r.adjust(m.left(), m.top(), -m.right(), -m.bottom());
  panel.setGeometry(r);
}


//////////////////////////////////////////////////////////////////////
/// Event-related functions.

// These are not necessary, but kept just for the record.

bool CurveView::event(QEvent * event)
{
  switch(event->type()) {
  // case QEvent::HoverEnter:
  // case QEvent::HoverLeave: 
  // case QEvent::HoverMove:
  //   hoverEvent(dynamic_cast<QHoverEvent*>(event));
  //   return true;
  //   break;
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
  if(eventLoop)
    eventLoop->receiveMouseEvent(event);
  else {
    if(panel.contains(event->pos())) {
      QPointF f = panel.fromWidget(event->pos());
      MainWin::showMessage(tr("X: %1, Y: %2").
                           arg(f.x()).arg(f.y()));
    }
    QAbstractScrollArea::mouseMoveEvent(event);
  }
}

void CurveView::mousePressEvent(QMouseEvent * event)
{
  if(eventLoop)
    eventLoop->receiveMouseEvent(event);
  else
    QAbstractScrollArea::mousePressEvent(event);
}

void CurveView::mouseReleaseEvent(QMouseEvent * event)
{
  if(eventLoop)
    eventLoop->receiveMouseEvent(event);
  else
    QAbstractScrollArea::mouseReleaseEvent(event);
}

void CurveView::keyPressEvent(QKeyEvent * event)
{
  if(eventLoop)
    eventLoop->receiveKeyEvent(event);
  else
    QAbstractScrollArea::keyPressEvent(event);
}

void CurveView::enterLoop(CurveEventLoop * loop)
{
  if(eventLoop)
    throw std::logic_error("Attempting to attach a second loop to the same view");
  if(! loop)
    return;

  eventLoop = loop;
  /// @todo It would be better to use QCoreApplication::setEventFilter
  /// to not prevent switching to other windows, as that is pretty
  /// painful. Even better, the filtering could be done at the level
  /// of CurveLoop, which would lift the need to forward events.
  viewport()->grabMouse();
  grabKeyboard();
}

void CurveView::leaveLoop()
{
  if(! eventLoop)
    return;
  viewport()->releaseMouse();
  releaseKeyboard();
  eventLoop = NULL;
}

