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
class CurveItem;

/// This widget displays CurveItem (or children thereof)
/// 
class CurveView : public QAbstractScrollArea {

  Q_OBJECT;

  /// Returns the rectangle in which the graph is displayed, in widget
  /// coordinates.
  QRect internalRectangle() const;

  /// The last used zooms
  QList<QRectF> zoomStack;

  /// The CurveItem in display
  QList<CurveItem *> displayedItems;

  /// The internal bounding box.
  QRectF boundingBox;

  /// Returns the currently displayed rectangle.
  QRectF currentZoom() const;

  /// Sets the view transformation for sceneRectangle to match
  /// windowRectangle (the latter most probably being
  /// internalRectangle()).
  ///
  /// \p windowRectangle is understood in terms of widget coordinate
  /// (Y top to bottom) and \p sceneRectangle in terms of usual
  /// scientific coordinates (Y bottom to top);
  void computeTransform(const QRect & windowRectangle,
                    const QRectF & sceneRectangle);

  /// Sets the transformation to that internalRectangle() shows
  /// currentZoom().
  void computeTransform();

  /// The transform to be used to scale from the 
  QTransform transform;

  Vector xTicks;
  Vector yTicks;

  /// Chooses the location for the X and Y ticks
  void pickTicks();

  /// Invalidate ticks
  void invalidateTicks();

  /// The pen used to draw backgroundLines
  QPen bgLinesPen;

  /// Paint all the curves.
  void paintCurves(QPainter * p);
  
  /// Number of curves that got a style so far, also counting the ones
  /// that were potentially removed. 
  int nbStyled;

  /// Returns a pen for the next curve to be added.
  QPen penForNextCurve();

public:

  CurveView();
  virtual ~CurveView();

  /// Zooms to the given rectangle, or the full scene if the rectangle
  /// is empty
  void zoomTo(const QRectF &r = QRectF());

  /// Adds a DataSet to the display.
  void addDataSet(const DataSet * ds);

  /// Shows the given DataSet (and forget about the other things)
  void showDataSet(const DataSet * ds);

  /// Remove everything from the display
  void clear();

protected:
  virtual void resizeEvent(QResizeEvent * event);
  virtual void paintEvent(QPaintEvent * event);
};

#endif
