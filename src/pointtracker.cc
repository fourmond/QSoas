/*
  pointtracker.cc: implementation of the PointTracker class
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
#include <pointtracker.hh>

#include <curveeventloop.hh>
#include <curvepanel.hh>
#include <dataset.hh>
#include <curvedataset.hh>

#include <soas.hh>

PointTracker::PointTracker(CurveEventLoop * l, const DataSet * ds) :
  loop(l), trackedDataSet(ds)
{
}

PointTracker::~PointTracker()
{
}

bool PointTracker::processEvent()
{
  if(loop->type() == QEvent::MouseMove && loop->currentPanel())  {
    lastIndex = loop->distanceToDataSet(trackedDataSet).second;
    if(lastIndex >= 0)
      p = trackedDataSet->pointAt(lastIndex);
    else
      p = QPointF();
  }
  return false;
}

