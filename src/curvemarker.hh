/**
   \file curvemarker.hh
   The CurveMarker class to draw markers.
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

#ifndef __CURVEMARKER_HH
#define __CURVEMARKER_HH

#include <curveitem.hh>


/// A marker, centered on a point.
class CurveMarker : public CurveItem {
  void paintMarker(QPainter * painter, const QPointF & realPos);
public:

  QPointF p;

  /// Points. If empty, p is used.
  QList<QPointF> points;

  QBrush brush;
  double size;

  typedef enum {
    Circle,
  } MarkerType;

  MarkerType type;

  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget);

  /// Paints the marker of the given \p type at the given position,
  /// using the current pen and brush.
  static void paintMarker(QPainter * painter, const QPointF & p,
                          MarkerType type);

  CurveMarker() : p(0.0/0.0, 0.0/0.0), size(1.0), type(Circle) {;};
};


#endif
