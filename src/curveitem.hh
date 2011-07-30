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
class CurveItem : public QObject {
public:
  virtual ~CurveItem();

  virtual QRectF boundingRect() const;

  /// Paint the curve. The painter is setup so that the coordinate are
  /// the curves coordinates.
  virtual void paint(QPainter * painter) = 0;
};


#endif
