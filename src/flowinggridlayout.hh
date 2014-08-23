/**
   \file flowinggridlayout.hh
   A QLayout child arranging items in a flowing grid
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
#ifndef __FLOWINGGRIDLAYOUT_HH
#define __FLOWINGGRIDLAYOUT_HH

/// A QLayout implementing a flowing grid, ie items layed out in a
/// grid that arranges themselves in a grid with a varying number of
/// columns (a bit like an icon view)
class FlowingGridLayout : public QLayout {

  QList<QLayoutItem *> managedItems;
public:
  virtual void addItem(QLayoutItem * item);
  virtual QLayoutItem * itemAt(int idx) const;
  virtual QLayoutItem * takeAt(int idx);

  virtual QSize sizeHint() const;
  virtual QSize minimumSize() const;
  virtual void setGeometry(const QRect & r);

  virtual bool hasHeightForWidth() const;
  virtual int heightForWidth(int height) const;

  ~FlowingGridLayout();

  virtual int count() const;

protected:
  QSize itemsMinimumSize() const;

  /// Computes the layout and return the height
  int computeLayout(const QRect & r, bool doPlacement = false) const;
};

#endif
