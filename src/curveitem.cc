/*
  curveitem.cc: implementation of the CurveItem class
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
#include <curveitem.hh>
#include <dataset.hh>

CurveItem::CurveItem(bool c) : countBB(c), hidden(false)
{
}

CurveItem::~CurveItem()
{
}

QRectF CurveItem::boundingRect() const
{
  return QRectF();
}

QRect CurveItem::paintLegend(QPainter *, const QRect &)
{
  return QRect();
}

double CurveItem::distanceTo(const QPointF &, double, double)
{
  return -1;
}

QString CurveItem::toolTipText(const QPointF &)
{
  return QString();
}

void CurveItem::timeOut(int milliseconds)
{
  QTimer::singleShot(milliseconds, this, 
                     SLOT(deleteLater()));
}
