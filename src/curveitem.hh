/**
   \file curveitem.hh
   The CurveItem class, ie something displayed by CurveView
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
#ifndef __CURVEITEM_HH
#define __CURVEITEM_HH

/// An object displayed by a CurveView class.
///
/// @todo Choose between widget and curve coordinates through a
/// virtual function.
///
/// @todo Implement various displays: XY display, 0Y display (index
/// instead of X), X and/or Y log... This should come as an argument
/// to paint().
///
/// @todo Distance computation.
class CurveItem : public QObject {
public:

  CurveItem(bool count = false);
  virtual ~CurveItem();

  virtual QRectF boundingRect() const;

  /// Paint the curve. The \a painter is in \b widget coordinates, and
  /// the \a curveToWidget transformation can be used to compute the
  /// widget points from the curve points. \a bbox is also in curve
  /// coordinates !
  virtual void paint(QPainter * painter, const QRectF & bbox,
                     const QTransform & curveToWidget) = 0;
  
  /// Whether the bounding box of this item counts or not.
  bool countBB;

  /// Whether the item is hidden or not
  bool hidden;

  /// Whether or not the CurvePanel takes ownership of the CurveItem
  /// or not.
  bool shouldDelete;

  /// The pen used to draw the item
  QPen pen;

  /// Paint a legend for the object, while trying to fit reasonably in
  /// \a placement (but it is not required). This function is called
  /// with a \a painter in \b screen coordinates
  ///
  /// The actual space taken by the legend is returned. Null rectangle
  /// for no legend drawn
  virtual QRect paintLegend(QPainter * painter, 
                            const QRect & placement);

  /// Returns the distance from the object to the \p point, or a
  /// negative value if the object doesn't support distance
  /// computation.
  virtual double distanceTo(const QPointF & point, 
                            double xscale,
                            double yscale);

  /// Returns a tooltip text to be shown at the given point. Functions
  /// may rely on data cached during the last call to distanceTo();
  ///
  /// @todo This isn't so great for now, it may have to be eventually
  /// enhanced with a
  virtual QString toolTipText(const QPointF & point);

  /// Destroy the object in \p milliseconds
  ///
  /// @todo Add fine grain to fade out ?
  void timeOut(int milliseconds);


  /// @name Cache handling
  ///
  /// CurvePanel handles caching using a bitmap cache the drawing for
  /// the "hard parts"
  ///
  /// @{
protected:

  /// When true, the cache isn't up-to-date. Defaults to true.
  bool dirty;

public:

  /// Returns true if the item should be drawn in a bitmap cache.
  /// (defaults to false)
  virtual bool shouldBeCached() const;

  /// Returns true if the cache should be redrawn. This value has no
  /// meaning if shouldBeCached() is false
  virtual bool shouldRedraw() const;


  /// Clears the dirty flag
  virtual void clearDirty();

  /// Sets the dirty flag
  virtual void setDirty();


  /// @}
};


#endif
