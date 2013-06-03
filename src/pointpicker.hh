/**
   \file pointpicker.hh
   The PointPicker class, representing a data set (most of the time one data file)
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
#ifndef __POINTPICKER_HH
#define __POINTPICKER_HH

class CurveEventLoop;
class CurvePanel;
class DataSet;


/// A class for picking points from a dataset in a CurvePanel. It
/// supports three point picking modes:
/// \li off (anywhere)
/// \li exact (the exact point location)
/// \li smooth (a smoothed position from the curve)
///
/// @todo Support other point selection modes ?
class PointPicker {

  /// The loop we're getting events from
  CurveEventLoop * loop;

  /// The DataSet we're interested in, or NULL if we're interested in
  /// all.
  const DataSet * trackedDataSet;
  
  /// The panel where the curve lies
  CurvePanel * panel;


  typedef enum {
    Off,
    Exact,
    Smooth
  } Method;

  /// The current method for point picking.
  Method method;

  QPointF lastPos;
  Qt::MouseButton lastButton;
  int lastIndex;

  static Method lastMethodUsed;

  /// Returns the message string appropriate to the current method.
  QString pointPickerMessage() const;

public:
  PointPicker(CurveEventLoop * l, const DataSet * ds = NULL, 
              CurvePanel * p = NULL);

  ~PointPicker();

  /// The buttons that are being followed for this event.
  Qt::MouseButtons trackedButtons;


  /// Process events from the loop. Returns \a true if the event
  /// shouldn't be processed anymore.
  ///
  /// @todo Maybe I could even add hooks to the CurveEventLoop
  /// framework so that the user wouldn't even have to call manually
  /// processEvents ?
  bool processEvent();

  /// Returns the last button pressed for point picking, or
  /// Qt::NoButton if the last point wasn't a tracked button pressed.
  Qt::MouseButton button() { return lastButton; };

  // /// Returns the dataset corresponding to the last press (meaningful
  // /// only if lastPress isn't Qt::MouseButton)
  // ///
  // /// Always NULL for off
  // DataSet * dataset() { return ;

  /// Returns the last point (according to the method currently in
  /// order)
  QPointF point() { return lastPos; };

  /// Returns the index of the point in the last dataset, or -1 if not
  /// applicable.
  int pointIndex() { return lastIndex;};

  /// Returns a small text to insert into the loop's help string
  QString helpText() const;

  /// Picks a number of points between the given indices.
  QList<QPointF> pickBetween(int firstIndex, int lastIndex, int number);
  
};

#endif
