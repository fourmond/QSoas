/**
   \file curveitem.hh
   The CurveItem class, ie something displayed by CurveView
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

  /// Paint the curve. The \a painter is in \b curve coordinates.
  virtual void paint(QPainter * painter) = 0;
  
  /// Whether the bounding box of this item counts or not.
  bool countBB;

  /// Paint a legend for the object, while trying to fit reasonably in
  /// \a placement (but it is not required). This function is called
  /// with a \a painter in \b screen coordinates
  ///
  /// The actual space taken by the legend is returned. Null rectangle
  /// for no legend drawn
  virtual QRect paintLegend(QPainter * painter, 
                            const QRect & placement);
};


#endif
