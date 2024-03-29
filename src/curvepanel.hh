/**
   \file curvepanel.hh
   The subwidget display a set of curves (in a given "box")
   Copyright 2011 by Vincent Fourmond
             2012 by CNRS/AMU

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
#ifndef __CURVEPANEL_HH
#define __CURVEPANEL_HH


class CurveItem;
class CurveDataSet;
class DataSet;
#include <vector.hh>

/// This object displays CurveItem (or children thereof). It is not a
/// widget by itself.
///
/// Its position is chosen using the setGeometry function. \b Note:
/// this is only the position of the inner frame. Legends and tick
/// labels will be drawn outside.
///
/// @todo A way to ensure that several CurvePanel will share some of
/// their BB (X axis, Y axis, and the like...) (this is done for X)
///
/// @todo Handle double Y axis display (ie to display both the data
/// and the derivative in one go). THIS REALLY SHOULD BE DONE !
///
/// @todo Find a way to set 'maximum bounds' for a display, distinct
/// from the BBox, which ensures that whatever happens, the drawing
/// rect always end up there ?
class CurvePanel : public QObject {

  Q_OBJECT;


  /// The internal position.
  QRect internalRectangle;
  
  /// All displayed items.
  QList<QPointer<CurveItem> > displayedItems;

  /// Sets the panel transformation for sceneRectangle to match
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

  /// @todo Maybe xTicks and xTicksFactor should be merged into a
  /// single class ?
  Vector xTicks;
  double xTicksFactor;

  Vector yTicks;
  double yTicksFactor;

  /// Chooses the location for the X and Y ticks
  void pickTicks();

  /// Invalidate ticks
  void invalidateTicks();

  /// Remove all items that are NULL
  void cleanItems();
  

  /// The pen used to draw backgroundLines
  QPen bgLinesPen;

  /// The pen used to draw background lines for X and Y = 0
  QPen orgPen;

  /// Paint all the curves.
  void paintCurves(QPainter * p);

  /// Paints the "classical" view
  void normalPaint(QPainter * painter);

  
  /// The bounding box of all items displayed. 
  QRectF boundingBox;

  /// The zoom
  QRectF zoom;

  /// Updates the bounding box.
  void updateBB();

  /// If that value isn't NULL, this panel will use in permanence the
  /// X values of the other one
  QPointer<CurvePanel> xTracking;

  /// Sets the zoom in such a way that the information is
  /// forwarded to panels we track.
  void setZoom(const QRectF & newBB, bool force = false);

  /// @name The pixmap cache
  ///
  /// @{

  /// The pixmap cache
  QPixmap cache;

  /// The zoom at which the last pixmap was drawn
  QRectF zoomedCache;

  /// @}
  
  
public:

  CurvePanel();
  virtual ~CurvePanel();

  /// Returns the currently displayed rectangle.
  QRectF currentZoom() const;

  /// Zooms to the given rectangle
  void zoomTo(const QRectF &r = QRectF());

  /// Adds a transient item
  void addItem(CurveItem * item, bool takeOwnership = false);

  /// Remove everything from the display
  void clear();

  /// Sets the position of the CurvePanel
  void setGeometry(const QRect & rect);

  /// Paints to the given painter
  void paint(QPainter * painter);

  /// Returns the bounding box of all the elements
  QRectF overallBB() const;


  bool drawingXTicks;
  bool drawingYTicks;

  /// Whether or not to draw the legend
  bool drawingLegend;

  /// Whether or not zoom out of the bbox are allowed
  bool anyZoom;

  /// The X label of the panel
  QString xLabel;

  /// The Y label of the panel (always counted in)
  QString yLabel;

  /// Returns a QMargins item showing how much space the CurveView
  /// should reserve around the geometry rectangle.
  ///
  /// Space is counted positive.
  QMargins panelMargins() const;

  /// Whether the given point is contained in the plot region.
  bool contains(const QPoint &p) const {
    return internalRectangle.contains(p);
  };

  /// Maps from widget coordinates to curve coordinates.
  QPointF fromWidget(const QPoint & p) const {
    return reverseTransform.map(QPointF(p));
  };

  /// Returns the CurveItem closest to the given point. If not NULL,
  /// the value pointed to by \p distance will be set to the distance.
  CurveItem * closestItem(const QPointF &point, 
                          double * distance = NULL) const;


  /// Returns the list of displayed datasets
  QList<DataSet *> displayedDataSets() const;

  /// Returns the (last) CurveDataSet object displaying the given
  /// dataset, or NULL if the dataset is NULL or not on display
  CurveDataSet * findDataSet(const DataSet * dataset) const;

  /// The stretching factor of this panel with respect to the others.
  /// Defaults to 100.
  int stretch;

  /// Returns the current XY scale factors from curve coordinates to
  /// screen coordinates. 
  QPointF scaleFactors() const;

  /// Zooms in on the given point by 1.3 to the power \a by 
  ///
  /// @todo Find a way to zoom in on either X or Y but not both ?
  /// @li this zomm in is fine
  /// @li another zoom (plain mouse wheel ?) to shift Y only
  /// @li another zoom (shift mouse wheel or horiz) to shift X only.
  void zoomIn(const QPointF & pos, double by = 1);
  
  /// Zooms only along the given axis/
  void zoomIn(const QPointF & pos, Qt::Orientation orient, 
              double by = 1);

  /// Zooms in on the given rectangle (or cancel current zoom on empty
  /// \a rect).
  void zoomIn(const QRectF & rect, bool force = false);

  /// Resets zoom to the actual bounding box.
  void resetZoom();

  /// Sets the Y values of the bounding box, taking the X values from
  /// the current zoom of the given panel (or this one if NULL).
  ///
  /// If the \a panel argument isn't NULL, this panel will track in
  /// permanence the X values of the given target panel.
  void setYRange(double ymin, double ymax, 
                 CurvePanel * panel = NULL);

  /// Render the panel into the given painter.
  /// 
  /// \a innerPanelHeight is the height of the inner panel, in natural
  /// units (pixels, or font points)
  /// \a targetRectangle is the target rectangle of the page
  /// \a title is a title for the page, if needs be.
  ///
  /// \warning For now, it doesn't handle the case when
  /// targetRectangle is offset from 0,0.
  void render(QPainter * painter,
              int innerPanelHeight, const QRect & targetRectangle,
              const QString & title = "");

  /// Returns the list of the currently displayed items.
  QList<CurveItem *> items();

  /// @name Cache-related functions
  ///
  /// @{

  /// regroup cacheable items at the bottom. Preserve the order among
  /// cacheable and non-cacheable items.
  void regroupCacheable();

  /// Invalidates the cache. Useful when a global display option has
  /// changed.
  void invalidateCache();

  /// @}
};

#endif
