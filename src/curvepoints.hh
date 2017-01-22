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
#include <vector.hh>


/// A series of data points drawn as markers, optionally with error bars.
/// Data is specified using a gsl_vector object.
///
/// Maybe this restriction can be lif
class CurvePoints : public CurveItem {
  /// The gsl_vector holding the X points
  gsl_vector * xvalues;

  /// The gsl_vector holding the Y points
  gsl_vector * yvalues;

  /// The gsl_vector holding the errors.
  /// If NULL, then no errors will be displayed
  gsl_vector * errors;

  /// Draws an error bar at the given position
  void drawErrorBar(QPainter * painter, double x, double y, double err,
                    const QTransform & ctw) const;

public:


  QBrush brush;
  double size;

  /// Size in points of one half of the error bar.
  double errorBarSize;

  CurveMarker::MarkerType type;

  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget) override;

  virtual QRectF boundingRect() const override;

  CurvePoints(gsl_vector * xvalues, gsl_vector * yvalues,
              gsl_vector * errors = NULL);
};


#endif
