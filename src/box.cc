/*
  box.cc: implementation of the Box class
  Copyright 2015 by CNRS/AMU

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
#include <box.hh>

Box::Box(double left, double right, double top, double bottom)
{
  values[0] = left;
  values[1] = right;
  values[2] = top;
  values[3] = bottom;
}

Box::Box(const QRectF & r)
{
  values[0] = r.left();
  values[1] = r.right();
  values[2] = r.bottom();
  values[3] = r.top();
}

void Box::normalize()
{
  if(values[0] > values[1])
    std::swap(values[0], values[1]);

  if(values[3] > values[2])
    std::swap(values[3], values[2]);
}

QRectF Box::toRectF() const
{
  return QRectF(values[0], values[3], values[1] - values[0], values[2] - values[3]);
}

/// purely debug function
QString Box::toStr() const
{
  return QString("%1,%2 to %3,%4").arg(left()).arg(top()).arg(right()).arg(bottom());
}
