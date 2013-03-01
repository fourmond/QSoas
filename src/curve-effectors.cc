/*
  curve-effectors.cc: implementation of the scoped modifications
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
#include <curve-effectors.hh>
#include <curveitems.hh>
#include <curvepanel.hh>
#include <curveview.hh>

CEHideAll::CEHideAll(CurvePanel * tg) :
  target(tg)
{
  QList<CurveItem *> items = target->items();
  for(int i = 0; i < items.size(); i++) {
    CurveItem * it = items[i];
    if(it && (!it->hidden)) {
      initiallyVisibleItems << it;
      it->hidden = true;
    }
  }
}

CEHideAll::~CEHideAll()
{
  for(int i = 0; i < initiallyVisibleItems.size(); i++) {
    CurveItem * it = initiallyVisibleItems[i];
    if(it)
      it->hidden = false;
  }
  /// @todo How to make sure the view is updated ?
}
