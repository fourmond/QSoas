/**
   \file debug.hh
   Code useful for debugging.
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


#ifndef __DEBUG_HH
#define __DEBUG_HH

/// Functions useful for debugging.
namespace Debug {

  /// Writes some information about the widget currently holding the
  /// focus to standard output.
  void dumpCurrentFocus(const QString & str = "Current focus");


  /// Returns a string used to represent a QPointF
  inline QString dumpPoint(const QPointF & point) {
    return QString("x: %1, y: %2").arg(point.x()).arg(point.y());
  };

  /// Returns a string used to represent a QRectF
  inline QString dumpRect(const QRectF & rect) {
    return QString("[%1 to %2, w: %3, h: %4, %5, %6]").
      arg(dumpPoint(rect.topLeft())).
      arg(dumpPoint(rect.bottomRight())).
      arg(rect.width()).arg(rect.height()).
      arg(rect.isValid() ? "valid" : "invalid").
      arg(rect.isEmpty() ? "empty" : "non-empty");
  };
};

#endif
