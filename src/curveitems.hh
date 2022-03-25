/**
   \file curveitems.hh
   Various generally useful CurveItem children
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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
#ifndef __CURVEITEMS_HH
#define __CURVEITEMS_HH

#include <curveitem.hh>
#include <vector.hh>

/// A simple line
class CurveLine : public CurveItem {
public:

  QPointF p1;
  QPointF p2;

  virtual QRectF boundingRect() const override ;
  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget) override ;
};

/// A vertical line
class CurveVerticalLine : public CurveItem {
public:

  double x;

  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget) override;
};

/// A set of vertical lines
class CurveVerticalLines : public CurveItem {
public:

  const Vector * xValues;

  CurveVerticalLines() : xValues(NULL) {;};

  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget) override;
};


/// A set of vertical lines
class CurveCross : public CurveItem {
public:

  QPointF p;

  CurveCross() {};

  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget) override;
};


/// A horizontal region, delimited by two vertical lines
class CurveHorizontalRegion : public CurveItem {
public:

  /// Whether this guarantees xleft < xright.
  ///
  /// Off by default
  bool autoSwap;

  CurveHorizontalRegion();

  QPen leftPen;
  QPen rightPen;
  double xleft, xright;

  /// Returns the lowest X value
  double xmin() const;

  /// Returns the highest X value
  double xmax() const;

  /// Sets either the left of right X value, depending on the button
  /// pressed. (or neither)
  void setX(double value, Qt::MouseButton which);

  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget) override;

  /// Finds the indices of the points closest to the current values.
  ///
  /// @todo In due time, use caching ? (but that's in Vector)
  void getClosestIndices(const Vector & xvalues, int * left, int * right, 
                         bool ensureGrowing = false) const;
};

/// A rectangle
class CurveRectangle : public CurveItem {
public:
  QPointF p1;
  QPointF p2;
  QBrush brush;

  void setRect(const QRectF &r);
  
  CurveRectangle() : brush(QBrush(Qt::NoBrush)) {;};

  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget) override;
};

/// Plain cacheless data.
class CurveData : public CurveItem {
public:
  Vector xvalues;
  Vector yvalues;
  // QBrush brush;

  bool histogram;

  /// Whether or not it should be cached by default... Defaults to false.
  bool shouldCache;

  bool shouldBeCached() const override;

  void setRect(const QRectF &r);
  
  CurveData() : histogram(false), shouldCache(false) {;};
  CurveData(const Vector & x, const Vector & y) : 
    xvalues(x), yvalues(y), histogram(false), shouldCache(false) {;};

  virtual QRectF boundingRect() const override;
  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget) override;

  QString legend;
  virtual QRect paintLegend(QPainter * painter, 
                            const QRect & placement) override;


};

#endif
