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

#include <soas.hh>

PointPicker::PointPicker(CurveEventLoop * l, const DataSet * ds, 
                         CurvePanel * p) :
  loop(l), trackedDataSet(ds), panel(p), 
  method(Exact),
  lastButton(Qt::NoButton),
  trackedButtons(Qt::LeftButton) 
{
}

bool PointPicker::processEvent()
{
  lastButton = Qt::NoButton;
  if(loop->type() == QEvent::MouseButtonPress && 
     trackedButtons.testFlag(loop->button())) {
    lastButton = loop->button();
    // here, pick points
    switch(method) {
    case Off:
      lastPos = loop->position();
      lastIndex = -1;
      break;
    case Exact:
      if(trackedDataSet) {
        /// @todo distance checking ?
        ///
        /// @todo handle the case of multiple datasets.
        QPair<double, int> dst = loop->distanceToDataSet(trackedDataSet);
        if(60 > dst.first && 0 <= dst.second) { // operations in
                                                // reverse to avoid
                                                // confusing emacs
          lastIndex = dst.second;
          lastPos = trackedDataSet->pointAt(lastIndex);
        }
        break;
      }
    }

    return true;
  }
  else if(loop->type() == QEvent::KeyPress) {
    switch(loop->key()) {
    case 'x':
    case 'X':                   // eXact
      method = Exact;
      soas().showMessage(QObject::tr("Marker set to exact"));
      return true;
    case 'o':
    case 'O':                   // Off
      method = Off;
      soas().showMessage(QObject::tr("Marker set to off"));
      return true;
    default:
      return false;
    }
  }
  return false;
}

QString PointPicker::helpText() const 
{
  return QObject::tr("Markers:\nx: eXact\no: Off\n");
}
