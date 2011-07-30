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
class CurveDataSet;

class CurveEventLoop;

/// This widget displays CurveItem (or children thereof).
///
/// It is the main graphical window of Soas (if not the only one !)
///
/// @todo Neat things to do:
/// 
/// @li setup a way to add transient indications (markers, XY
/// data,...) that would get displayed and possibly dropped altogether
/// with an appropriate function that probably would be called upon
/// leaving the loop ?
/// 
class CurveView : public QAbstractScrollArea {

  Q_OBJECT;

  /// Returns the rectangle in which the graph is displayed, in widget
  /// coordinates.
  QRect internalRectangle() const;

  /// The last used zooms
  QList<QRectF> zoomStack;

  /// The CurveItem in display
  QList<CurveDataSet *> displayedDataSets;

  /// Temporary objects. They can be deleted at any time, we don't
  /// care.
  QList<QPointer<CurveItem> > transientItems;

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

  /// Transforms curve coordinate into widget coordinates
  QTransform transform;

  /// Does the reverse.
  QTransform reverseTransform;

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

  /// Maps from widget coordinates to curve coordinates.
  QPointF fromWidget(const QPoint & p) {
    return reverseTransform.map(QPointF(p));
  };
  

  /// @name Event loop related functions/attributes
  ///
  /// 
  /// Handling of event loops.
  /// @{

  friend class CurveEventLoop;
  CurveEventLoop * eventLoop;

  void enterLoop(CurveEventLoop * loop);
  void leaveLoop();

  /// @}

  /// Returns the closest DataSet to the given point.
  const DataSet * closestDataSet(const QPointF &point, 
                                 double * dist, int * idx) const;

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

  /// Adds a transient item
  void addTransientItem(CurveItem * item);

  /// Remove everything from the display
  void clear();

protected:
  virtual void resizeEvent(QResizeEvent * event);
  virtual void paintEvent(QPaintEvent * event);


  virtual bool event(QEvent * event);

  virtual void mouseMoveEvent(QMouseEvent * event);
  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseReleaseEvent(QMouseEvent * event);

  virtual void helpEvent(QHelpEvent * event);

  virtual void keyPressEvent(QKeyEvent * event);
};

#endif
