/*
  boundingbox.cc: implementation of BoundingBox
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
#include <boundingbox.hh>
#include <dataset.hh>

#include <limits>

BoundingBox::BoundingBox() :
  lx(std::numeric_limits<double>::quiet_NaN()),
  by(std::numeric_limits<double>::quiet_NaN()),
  rx(std::numeric_limits<double>::quiet_NaN()),
  ty(std::numeric_limits<double>::quiet_NaN())
{
}

BoundingBox::BoundingBox(double x, double y, double width, double height) :
  lx(x), by(y), rx(x+width), ty(y+height)
{
}

BoundingBox::BoundingBox(const QPointF & bottomLeft, const QSizeF & size) :
  BoundingBox(bottomLeft.x(), bottomLeft.y(), size.width(), size.height())
{
}

BoundingBox::BoundingBox(const QRectF & r) : BoundingBox(r.topLeft(), r.size())
{
}

bool BoundingBox::isNull() const
{
  return ! (std::isfinite(lx) && std::isfinite(rx) &&
            std::isfinite(ty) && std::isfinite(by));
}

bool BoundingBox::isPoint() const
{
  return (rx - lx == 0) && (ty - by == 0);
}

double BoundingBox::xMin() const
{
  return std::min(lx, rx);
}

double BoundingBox::xMax() const
{
  return std::max(lx, rx);
}

double BoundingBox::yMin() const
{
  return std::min(by, ty);
}

double BoundingBox::yMax() const
{
  return std::max(by, ty);
}

void BoundingBox::uniteWith(const BoundingBox & bbox)
{
  if(bbox.isNull())
    return;                     // Nothing to do
  lx = std::min(xMin(), bbox.xMin());
  rx = std::max(xMax(), bbox.xMax());

  by = std::min(yMin(), bbox.yMin());
  ty = std::max(yMax(), bbox.yMax());
}
