/**
   \file curveview.hh
   The widget handling all "terminal" interaction
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

#ifndef __CURVEVIEW_HH
#define __CURVEVIEW_HH


#include <vector.hh>
class DataSet;

/// This widget displays CurveItem (or children thereof)
/// 
class CurveView : public QAbstractScrollArea {

  Q_OBJECT;

  /// The application-wide CurveView
  static CurveView * theCurveView;


  /// Returns the rectangle in which the graph is displayed, in widget
  /// coordinates.
  QRect internalRectangle() const;

  QList<QRectF> zoomStack;

  /// Returns the currently displayed rectangle.
  QRectF currentZoom() const;

  /// Sets the view transformation for sceneRectangle to match
  /// windowRectangle (the latter most probably being
  /// internalRectangle()).
  ///
  /// \p windowRectangle is understood in terms of widget coordinate
  /// (Y top to bottom) and \p sceneRectangle in terms of usual
  /// scientific coordinates (Y bottom to top);
  void setTransform(const QRect & windowRectangle,
                    const QRectF & sceneRectangle);

  /// Sets the transformation to that internalRectangle() shows
  /// currentZoom().
  void setTransform();

  Vector xTicks;
  Vector yTicks;

  /// Chooses the location for the X and Y ticks
  void pickTicks();

  /// Invalidate ticks
  void invalidateTicks();

  /// The pen used to draw backgroundLines
  QPen bgLinesPen;

  /// The underlying widget
  QWidget * w;

public:

  CurveView();
  virtual ~CurveView();

  /// Zooms to the given rectangle, or the full scene if the rectangle
  /// is empty
  void zoomTo(const QRectF &r = QRectF());

  /// Returns the application-wide CurveDisplayWidget
  static CurveView * displayWidget() {
    return theCurveView;
  };

  /// Adds a DataSet to the display.
  void addDataSet(const DataSet * ds);


protected:
  virtual void resizeEvent(QResizeEvent * event);
  virtual void paintEvent(QPaintEvent * event);
};

#endif
