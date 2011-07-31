/**
   \file curveitems.hh
   Various generally useful CurveItem children
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

#ifndef __CURVEITEMS_HH
#define __CURVEITEMS_HH

#include <curveitem.hh>


/// A simple line
class CurveLine : public CurveItem {
public:

  QPointF p1;
  QPointF p2;
  QPen pen;

  virtual QRectF boundingRect() const;
  virtual void paint(QPainter * painter, const QRectF & bbox);
};

/// A marker, centered on a point.
class CurveMarker : public CurveItem {
public:

  QPointF p;
  QPen pen;
  QBrush brush;
  double size;

  virtual void paint(QPainter * painter, const QRectF & bbox);

  CurveMarker() : size(1.0) {;};
};


#endif
