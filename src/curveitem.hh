/**
   \file curveitem.hh
   The CurveItem class, ie an object representing a 2D curve on a graphics scene
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

class DataSet;
/// A 2D curve on a ...
///
/// 
class CurveItem : public QGraphicsItem {

  const DataSet * dataSet;

  QPainterPath * cachedPath;

  /// @todo With this scheme, things won't happen too well
  void createPath();
public:
  CurveItem(const DataSet * ds): dataSet(ds), 
                                 cachedPath(NULL) {;};

  virtual ~CurveItem();

  virtual QRectF boundingRect() const;
  virtual void paint(QPainter * painter, 
                     const QStyleOptionGraphicsItem * option, 
                     QWidget * widget);
};


#endif
