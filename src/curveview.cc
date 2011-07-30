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

#include <terminal.hh>
#include <utils.hh>

CurveView::CurveView(QGraphicsScene * sc) : QGraphicsView(sc)
{
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::NoFrame);
  connect(sc, SIGNAL(sceneRectChanged(const QRectF &)),
          SLOT(updateSceneRect(const QRectF &)));
}

CurveView::~CurveView()
{
}

QRect CurveView::internalRectangle() const
{
  return rect().adjusted(10, 10, -15, -15);
}

QRectF CurveView::currentZoom() const
{
  /// @todo zoom stack
  return sceneRect();
}

void CurveView::setTransform(const QRect & wR,
                             const QRectF & sR)
{
  QTransform t;
  t.translate(-sR.x(), -sR.y());
  t.scale(wR.width()/sR.width(), -wR.height()/sR.height());
  t.translate(wR.x(), wR.y());
  QGraphicsView::setTransform(t);
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
