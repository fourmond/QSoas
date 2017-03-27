/**
   \file boundingbox.hh
   The Boundingbox class, a better behaved QRectF
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
#ifndef __BOUNDINGBOX_HH
#define __BOUNDINGBOX_HH


/// BoundingBox is almost a QRectF, but with important differences
/// that make it easier to use them for computing bounding boxes:
/// @li isNull() is only true if one of the coordinates is invalid
/// @li an "empty" bounding box, i.e. reduced to one point, does not
/// satisfy isNull(), but isPoint()
/// @li isValid() is the opposite of isNull()
///
/// The coordinates are "natural", i.e. the bottom Y value is smaller
/// than the top Y value, if the rectangle is normalized()
class BoundingBox {
  /// The coordinates. The bounding box is Null if any of those is not
  /// finite.
  double lx,by,rx,ty;
public:

  /// Creates a null bounding box, i.e. one that verifies isNull()
  BoundingBox();
  
  BoundingBox(const QPointF & bottomLeft, const QSizeF & size);
  BoundingBox(double x, double y, double width, double height);
  BoundingBox(const BoundingBox & bb);

  explicit BoundingBox(const QRectF & rect); // explicit to avoid troubles !

  // operator QRectF() const;

  bool isNull() const;
  bool isPoint() const;

  /// The minimum value of X
  double xMin() const;

  /// The minimum value of X
  double yMin() const;

  /// The maximum value of X
  double xMax() const;

  /// The maximum value of X
  double yMax() const;

  /// Unite with the given BoundingBox, so that the resulting
  /// BoundingBox encompasses both original ones.
  ///
  /// This function has the side effect of normalizing the
  /// BoundingBox.
  void uniteWith(const BoundingBox & bbox);
};




#endif
