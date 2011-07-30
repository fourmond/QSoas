/**
   \file curveitem.hh
   The CurveItem class, ie an object representing a 2D curve on a graphics scene
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

#ifndef __CURVEITEM_HH
#define __CURVEITEM_HH

class DataSet;
/// A 2D curve to be attached to a CurveView class.
///
/// 
class CurveItem {
public:
  /// The dataset attached to this item.
  const DataSet * dataSet;
private:
  QPainterPath * cachedPath;

  /// @todo With this scheme, things won't happen too well
  void createPath();
public:
  CurveItem(const DataSet * ds): dataSet(ds), 
                                 cachedPath(NULL) {;};

  virtual ~CurveItem();

  virtual QRectF boundingRect() const;

  /// Paint the curve. The painter is setup so that the coordinate are
  /// the curves coordinates.
  virtual void paint(QPainter * painter);

  /// The pen for painting the path
  QPen pen;

  /// Draw a legend in the given rectangle. Returns the actual width
  /// used for the draw.
  int drawLegend(QPainter * p, const QRect & rect) const;

};


#endif
