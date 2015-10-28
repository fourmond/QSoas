/**
   \file box.hh
   A class describing a rectangle
   Copyright 2015 by CNRS/AMU

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
#ifndef __BOX_HH
#define __BOX_HH

/// This class serves a very similar purpose to QRectF, but with much
/// simpler arithmetics...
class Box {
protected:

  /// The values, in the order left, right, top, bottom, with the idea
  /// that, if things are fine, top > bottom.
  double values[4];
  
public:

  Box(double left, double right, double top, double bottom);
  Box(const QRectF & other);

  /// Normalizes the box, so that left < right and top >
  /// bottom. (which is the other way to the
  void normalize();

  double left() const { return values[0]; };
  void setLeft(double v) { values[0] = v; };
  double right() const { return values[1]; };
  void setRight(double v) { values[1] = v; };
  double top() const { return values[2]; };
  void setTop(double v) { values[2] = v; };
  double bottom() const { return values[3]; };
  void setBottom(double v) { values[3] = v; };

  /// Returns as a normalized QRectF (i.e. top < bottom).
  QRectF toRectF() const;

  QString toStr() const;

};


#endif
