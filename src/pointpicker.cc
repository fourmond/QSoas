/*
  pointpicker.cc: implementation of the PointPicker class
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
#include <pointpicker.hh>

#include <curveeventloop.hh>
#include <curvepanel.hh>
#include <dataset.hh>
#include <curvedataset.hh>

#include <exceptions.hh>

#include <soas.hh>

/// Should this be a customization item ? NO
PointPicker::Method PointPicker::lastMethodUsed = PointPicker::Exact;

PointPicker::PointPicker(CurveEventLoop * l, const DataSet * ds, 
                         CurvePanel * p) :
  loop(l), trackedDataSet(ds), panel(p), 
  method(lastMethodUsed),
  lastButton(Qt::NoButton),
  trackedButtons(Qt::LeftButton) 
{
  loop->ppMessage = pointPickerMessage();
}

PointPicker::~PointPicker()
{
  lastMethodUsed = method;
}

void PointPicker::pickPoint()
{
  switch(method) {
  case Off:
    lastPos = loop->position();
    lastIndex = -1;
    break;
  case Exact:
  case Smooth:
    if(trackedDataSet) {
      /// @todo distance checking ?
      ///
      /// @todo handle the case of multiple datasets.
      QPair<double, int> dst = loop->distanceToDataSet(trackedDataSet);
      if(0 <= dst.second) { 
        if(method == Exact) {
          lastIndex = dst.second;
          lastPos = trackedDataSet->pointAt(lastIndex);
        }
        else {
          lastIndex = dst.second;
          lastPos = trackedDataSet->smoothPick(lastIndex);
        }
      }
      break;
    }
  }
}

bool PointPicker::processEvent()
{
  lastButton = Qt::NoButton;
  if(loop->type() == QEvent::MouseButtonPress && 
     trackedButtons.testFlag(loop->button())) {
    lastButton = loop->button();
    pickPoint();
    return true;
  }
  else if(loop->type() == QEvent::KeyPress) {
    switch(loop->key()) {
    case 'x':
    case 'X':                   // eXact
      method = Exact;
      break;
    case 'o':
    case 'O':                   // Off
      method = Off;
      break;
    case 's':
    case 'S':                   // Smooth
      method = Smooth;
      break;
    default:
      return false;
    }
    loop->ppMessage = pointPickerMessage();
    loop->updateMessage();
    return true;
  }
  return false;
}

QString PointPicker::pointPickerMessage() const
{
  switch(method) {
  case Exact: 
    return "(marker: exact)";
  case Smooth: 
    return "(marker: smooth)";
  case Off: 
    return "(marker: off)";
  }
  return QString();
}

QString PointPicker::helpText() const 
{
  return QObject::tr("Markers:\n"
                     "x: eXact\n"
                     "s: smooth\n"
                     "o: Off data\n");
}

QList<QPointF> PointPicker::pickBetween(int firstIndex, int lastIndex, 
                                        int number)
{
  QList<QPointF> points;
  if(number < 2)
    throw InternalError("Requires at least to pick two points");
  for(int i = 0; i < number; i++) {
    int idx = firstIndex + (lastIndex - firstIndex) * i / (number - 1);
    switch(method) {
    case Exact:
    case Off:
      points << trackedDataSet->pointAt(idx);
      break;
    case Smooth: 
      points << trackedDataSet->smoothPick(idx);
      break;
    default:
      ;
    }
  }
  return points;
}
