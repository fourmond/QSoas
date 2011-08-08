/**
   \file curvevector.hh
   The CurveVector class, ie an object representing a 2D curve on a graphics scene
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

#ifndef __CURVEVECTOR_HH
#define __CURVEVECTOR_HH

#include <curveitem.hh>

class DataSet;

/// A representation of a fit value/residual, based on:
/// \li a DataSet, containing the reference X and Y values
/// \li a vector view
///
/// @todo if the drawing becomes too slow, it may be possible to setup
/// a cache and invalidate it when we know the things are changing ?
class CurveVector : public CurveItem {
  gsl_vector_view view;
  const DataSet * dataSet;

  /// If true, we are drawing the residuals, else the fit
  bool residuals;

  /// The y value for the given index
  double yValue(int i) const;
  
public:
  CurveVector(const DataSet * ds, gsl_vector_view v, bool res = false) : 
    CurveItem(true), view(v), dataSet(ds), residuals(res)
  {;};

  virtual ~CurveVector();

  virtual QRectF boundingRect() const;

  /// Paint the curve. The painter is setup so that the coordinate are
  /// the curves coordinates.
  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget);

  virtual QRect paintLegend(QPainter * painter, 
                            const QRect & placement);

};


#endif
