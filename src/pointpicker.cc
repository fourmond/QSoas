/*
  pointpicker.cc: implementation of the PointPicker class
  Copyright 2011 by Vincent Fourmond
            2013, 2014 by CNRS/AMU

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
#include <curveview.hh>

#include <exceptions.hh>
#include <eventhandler.hh>

#include <soas.hh>
#include <datastack.hh>

/// Should this be a customization item ? NO
PointPicker::Method PointPicker::lastMethodUsed = PointPicker::Exact;

PointPicker::PointPicker(CurveEventLoop * l, const DataSet * ds, 
                         CurvePanel * p) :
  loop(l), panel(p), 
  method(lastMethodUsed),
  lastButton(Qt::NoButton),
  trackedButtons(Qt::LeftButton) 
{
  pickDataSet(ds);
  loop->ppMessage = pointPickerMessage();

  if(! panel)
    panel = l->getView()->mainPanel();
}

PointPicker::~PointPicker()
{
  lastMethodUsed = method;
}

void PointPicker::addToHandler(EventHandler & handler)
{
  handler.
    addKey('x', ExactMethod, "pick marker: exact").
    alsoKey('X').
    addKey('o', OffMethod, "...off").
    alsoKey('O').
    addKey('s', SmoothMethod, "...smooth").
    alsoKey('S').
    addKey('n', NextDataset, "next dataset").
    addKey('N', PrevDataset, "previous dataset");
}

void PointPicker::pickAt(int idx)
{
  if(method == Off) {
    lastPos = QPointF();
    lastIndex = -1;
  } else {
    if(trackedDataSet) {
      lastIndex = idx;
      if(method == Exact)
        lastPos = trackedDataSet->pointAt(lastIndex);
      else if(method == Smooth)
        lastPos = trackedDataSet->smoothPick(lastIndex);
    }
  }
}

void PointPicker::pickDataSet(const DataSet * ds)
{
  trackedDataSet = ds;
  if(ds) {
    int idx = 0;
    if(soas().stack().indexOf(ds, &idx))
      datasetIndex = (idx >= 0 ? idx + 1 : idx);
    else
      datasetIndex = 0;
  }
}

void PointPicker::pickAtX(double x)
{
  if(! trackedDataSet)
    return;
  int idx = trackedDataSet->x().closestPoint(x);
  pickAt(idx);
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
      /// @todo handle the case of multiple datasets.
      QPair<double, int> dst = loop->distanceToDataSet(trackedDataSet);
      pickAt(dst.second);
      break;
    }
  }
}

bool PointPicker::processEvent(int action)
{
  lastButton = Qt::NoButton;
  if(loop->type() == QEvent::MouseButtonPress && 
     trackedButtons.testFlag(loop->button())) {
    lastButton = loop->button();
    pickPoint();
    return true;
  }
  
  switch(action) {
  case ExactMethod:
    method = Exact;
    break;
  case OffMethod:
    method = Off;
    break;
  case SmoothMethod:
    method = Smooth;
    break;
  case NextDataset:
    nextDataSet();
    break;
  case PrevDataset:
    nextDataSet(-1);
    break;
  default:
    return false;
  }
  loop->ppMessage = pointPickerMessage();
  loop->updateMessage();
  return true;
}

void PointPicker::nextDataSet(int delta)
{
  if(! panel)
    return;
  QList<DataSet *> datasets = panel->displayedDataSets();

  int curIdx = -1;
  for(int i = 0; i < datasets.size(); i++) {
    if(datasets[i] == trackedDataSet) {
      curIdx = i;
      break;
    }
  }

  curIdx += delta;
  if(curIdx < 0)
    curIdx += datasets.size();
  if(curIdx >= datasets.size())
    curIdx = curIdx % datasets.size();
  pickDataSet(datasets.value(curIdx));
}

QString PointPicker::pointPickerMessage() const
{
  QString mk, ds;
  switch(method) {
  case Exact: 
    mk = "exact";
    break;
  case Smooth: 
    mk ="smooth";
    break;
  case Off: 
    mk = "off";
  }

  if(datasetIndex != 0)
    ds = QString(" #%1").
      arg(datasetIndex > 0 ? datasetIndex - 1 : datasetIndex);
  return QString("(%1%2)").arg(mk).arg(ds);
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
