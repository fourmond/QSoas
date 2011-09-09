/*
  flowinggridlayout.cc: implementation of the FlowingGridLayout class
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


#include <headers.hh>
#include <flowinggridlayout.hh>


void FlowingGridLayout::addItem(QLayoutItem * item)
{
  managedItems << item;
}

QLayoutItem * FlowingGridLayout::itemAt(int idx)
{
  return managedItems.value(idx, NULL);
}

QLayoutItem * FlowingGridLayout::takeAt(int idx)
{
  if(idx >= 0 && idx < managedItems.size())
    return managedItems.takeAt(idx);
  return 0;
}


FlowingGridLayout::~FlowingGridLayout()
{
  for(int i = 0; i < managedItems.size(); i++)
    delete managedItems[i];
}

QSize FlowingGridLayout::itemsMinimumSize() const
{
  QSize size;
  for(int i = 0; i < managedItems.size(); i++)
    size = size.expandedTo(managedItems[i]->minimumSize());
  return size;
}

QSize FlowingGridLayout::minimumSize() const
{
  QSize size = itemsMinimumSize();
  size += QSize(2*margin(), 2*margin());
  return size;
}

QSize FlowingGridLayout::sizeHint() const
{
  return minimumSize();
}

bool FlowingGridLayout::hasHeightForWidth() const
{
  return true;
}

void FlowingGridLayout::setGeometry(const QRect &rect)
{
  QLayout::setGeometry(rect);
  computeLayout(rect, true);
}


int FlowingGridLayout::computeLayout(const QRect & geom, bool doPlacement) const
{
  int left, top, right, bottom;
  getContentsMargins(&left, &top, &right, &bottom);
  QRect inner = geom.adjusted(+left, +top, -right, -bottom);

  // Now, we decide the number of columns:
  QSize widgetSize = itemsMinimumSize();
  int colWidth = widgetSize.width();
  int nbCols = inner.width()/colWidth;
  if(nbCols < 1)
    nbCols = 1;
  int curColHeight = 0;
  int curY = inner.top();
  for(int i = 0; i < managedItems.size(); i++) {
    if(! (i % nbCols)) {
      curY += curColHeight;
      curColHeight = 0;
    }

    QLayoutItem * it = managedItems[i];
    QSize sz = it->minimumSize();
    sz.setWidth(colWidth);
    if(curColHeight < sz.height())
      curColHeight = sz.height();
    int x = inner.left() + (i % nbCols) * colWidth;
    if(doPlacement)
      it->setGeometry(QRect(QPoint(x, curY), sz));
  }
  
  return curY + curColHeight;
}

int FlowingGridLayout::heightForWidth(int width) const
{
  int height = computeLayout(QRect(0, 0, width, 0));
  return height;
}
