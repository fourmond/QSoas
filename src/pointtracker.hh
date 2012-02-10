/**
   \file pointtacker.hh
   The PointTracker class, to have the mouse follow points from a DataSet
   Copyright 2012 by Vincent Fourmond

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

#ifndef __POINTTRACKER_HH
#define __POINTTRACKER_HH

#include <curvemarker.hh>

class CurveEventLoop;
class CurvePanel;
class DataSet;


/// This class follows 
class PointTracker : public CurveMarker {

  /// The loop we're getting events from
  CurveEventLoop * loop;

  /// The DataSet we're interested in, or NULL if we're interested in
  /// all.
  const DataSet * trackedDataSet;


public:
  PointTracker(CurveEventLoop * l, const DataSet * ds = NULL);
  
  virtual ~PointTracker();

  /// Process events from the loop. Returns \a true if the event
  /// shouldn't be processed anymore.
  ///
  /// @todo Maybe I could even add hooks to the CurveEventLoop
  /// framework so that the user wouldn't even have to call manually
  /// processEvents ?
  bool processEvent();

  /// The index of the last point tracked.
  int lastIndex;

};

#endif
