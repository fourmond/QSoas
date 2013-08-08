/**
   \file pointiterator.hh
   An iterator to go over a series of points
   Copyright 2013 by Vincent Fourmond

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
#ifndef __POINTITERATOR_HH
#define __POINTITERATOR_HH

class Vector;
class DataSet;

/// An java-style iterator to go over a collection of points in the
/// form of a X and Y Vector. This class provides various iterative
/// styles.
///
/// @todo It would be nice if the iterator could implement clipping on
/// its own, that is, given a bounding box, return only the points
/// inside and the points "immediately next" (ie just before/just
/// after). It may be difficult when there is not a single point within
/// the region, though ?
///
/// @todo Handle broken paths, ie when a point is invalid.
class PointIterator  {

  const Vector & x;
  const Vector & y;
  const DataSet * ds;

public:

  typedef enum {
    Normal,                     // ie, normal
    ErrorsAbove,                // errors above
    ErrorsBelow,                // errors below
    Errors,                     // errors both ways (ie first above
                                // forward and then below backwards
    Steps                       // ie, histogram
  } Type;


private:
  Type type;

  int index;

  int sub;

  int total;

  /// Advance to next point
  void advance();
public:

  PointIterator(const Vector & x, const Vector & y, Type t = Normal);
  PointIterator(const DataSet * ds, Type t = Normal);

  /// If there is a next point.
  bool hasNext() const;

  /// Peek at next point without advancing
  QPointF peek() const;

  /// Next point
  QPointF next();

  /// Next (transformed) point !
  QPointF next(const QTransform & trans);

  /// Adds all the points to a given path.
  void addToPath(QPainterPath & path, const QTransform & trans);

};


#endif
