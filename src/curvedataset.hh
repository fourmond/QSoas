/**
   \file curvedataset.hh
   The CurveDataSet class, ie an object representing a 2D curve on a graphics scene
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

#ifndef __CURVEDATASET_HH
#define __CURVEDATASET_HH

#include <curveitem.hh>

class DataSet;
/// A representation of a DataSet attached to a CurveView class.
class CurveDataSet {
public:
  /// The dataset attached to this dataset.
  const DataSet * dataSet;
private:
  QPainterPath * cachedPath;

  /// @todo With this scheme, things won't happen too well
  void createPath();
public:
  CurveDataSet(const DataSet * ds): dataSet(ds), 
                                    cachedPath(NULL) {;};

  virtual ~CurveDataSet();

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
