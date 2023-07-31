/**
   \file curvepoints.hh
   The CurvePoints class to draw series of points, possibly with error bars
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
#ifndef __CURVEPOINTS_HH
#define __CURVEPOINTS_HH

#include <curveitem.hh>
#include <curvemarker.hh>


class XYIterable;
class DataSet;


/// A series of data points drawn as markers, optionally with error bars.
///
/// Data is specified using a XYIterable object.
class CurvePoints : public CurveItem {

  /// The data source. Owned by the CurvePoints object.
  XYIterable * source;

  /// Draws an error bar at the given position
  void drawErrorBar(QPainter * painter, double x, double y, double err,
                    const QTransform & ctw) const;

public:


  QBrush brush;
  double size;

  /// Size in points of one half of the error bar.
  double errorBarSize;

  /// Whether the error is relative or not
  bool relativeErrorBar = false;

  CurveMarker::MarkerType type;

  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget) override;

  virtual QRectF boundingRect() const override;

  /// Creates a CurvePoints object showing the given source.
  ///
  /// The CurvePoints take ownership of the @a source
  explicit CurvePoints(XYIterable * source);

  ~CurvePoints();

  /// Makes a dataset from the source
  DataSet * makeDataSet();
};


#endif
