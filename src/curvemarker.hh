/**
   \file curvemarker.hh
   The CurveMarker class to draw markers.
   Copyright 2011 by Vincent Fourmond
             2012, 2013 by CNRS/AMU

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
#ifndef __CURVEMARKER_HH
#define __CURVEMARKER_HH

#include <curveitem.hh>


/// A marker, centered on a point.
class CurveMarker : public CurveItem {
public:

  /// The point of the marker
  QPointF p;

  /// Points. If empty, p is used.
  QList<QPointF> points;

  /// A label for the point \a p if applicable
  QString l;

  /// Labels for \a points
  QStringList labels;

  QBrush brush;
  double size;

  QPen textPen;

  typedef enum {
    Circle,
  } MarkerType;

  MarkerType type;

  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget);

  /// Paints the marker of the given \a type and the given size at the
  /// given position, using the current pen and brush.
  static void paintMarker(QPainter * painter, const QPointF & p,
                          MarkerType type, double size);

  /// Paints a label for the marker of the given type at point p.
  ///
  /// @todo add alignment flags ?
  static void paintMarkerLabel(QPainter * painter, const QPointF & p,
                               MarkerType type, double size, 
                               const QString & label, 
                               const QPen & pen = QPen());

  CurveMarker() : p(0.0/0.0, 0.0/0.0), size(1.0), type(Circle) {;};
};


#endif
