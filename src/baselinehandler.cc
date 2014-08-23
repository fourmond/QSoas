/*
  baselinehandler.cc: implementation of the BaselineHandler class
  Copyright 2013 by CNRS/AMU

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
#include <baselinehandler.hh>

#include <eventhandler.hh>
#include <curveeventloop.hh>
#include <curveview.hh>

#include <soas.hh>
#include <graphicssettings.hh>
#include <utils.hh>


EventHandler & BaselineHandler::addToEventHandler(EventHandler & target, 
                                                  BaselineHandler::Options opts)
{
  target.conventionalAccept(QuitSubtracting, "quit subtracting").
    addKey('v', QuitDividing, "quit dividing").
    addKey('V', ToggleDivision, "show divided signal").
    addKey('u', QuitReplacing, "quit replacing").
    alsoKey('U');
  if(! opts.testFlag(NoDerivative))
    target.addKey('d', ToggleDerivative, "toogle derivative").
      alsoKey('D');
  if(! opts.testFlag(NoPush))
    target.addKey(Qt::CTRL + 'p', PushCurrent, "push current to stack");
  target.addKey('h', HideDataset, "hide/show original data").
    alsoKey('H');
  return target;
}

BaselineHandler::BaselineHandler(CurveView & view, const DataSet * ds,
                                 const QString & sf, 
                                 Options opts) : 
  options(opts), signal(ds), showingDerivative(false), 
  showingDivided(false), suffix(sf)
  
{
  const GraphicsSettings & gs = soas().graphicsSettings();

  bottom.drawingXTicks = false;
  bottom.stretch = 35;        // 3/10ths of the main panel.

  bottom.yLabel = Utils::deltaStr("Y");


  datasetDisplay = view.getCurrentDataSet();


  // Setup of the modified signal display:
  modified.pen = gs.getPen(GraphicsSettings::BaselinePen);
  modified.xvalues = ds->x();
  modified.yvalues = QVector<double>(modified.xvalues.size(), 0);
  modified.countBB = true;
  view.addItem(&modified);

  diff.pen = gs.getPen(GraphicsSettings::ResultPen);
  diff.xvalues = modified.xvalues;
  diff.yvalues = ds->y();       // at the beginning ;-)...
  diff.countBB = true;
  bottom.addItem(&diff);

  divided.pen = gs.getPen(GraphicsSettings::ResultPen);
  divided.xvalues = modified.xvalues;
  divided.yvalues = ds->y();       // at the beginning ;-)...
  divided.countBB = true;
  divided.hidden = true;;
  bottom.addItem(&divided);

  view.addPanel(&bottom);

  derivative.pen = gs.getPen(GraphicsSettings::ResultPen);
  derivative.xvalues = modified.xvalues;
  derivative.yvalues = modified.yvalues;
  derivative.countBB = true;
  derivative.hidden = true;     // by default
  view.addItem(&derivative);

}

bool BaselineHandler::nextAction(int action, bool * shouldQuit,
                                 bool * shouldComputeDerivative,
                                 bool * shouldRecompute)
{
  *shouldQuit = false;
  if(shouldRecompute)
    *shouldRecompute = false;
  if(shouldComputeDerivative)
    *shouldComputeDerivative = showingDerivative;
  switch(action) {
  case HideDataset:
    datasetDisplay->hidden = ! datasetDisplay->hidden;
    return true;

  case ToggleDerivative:
    if(! options.testFlag(NoDerivative)) {
      showingDerivative = ! showingDerivative;
      modified.hidden = showingDerivative;
      derivative.hidden = ! showingDerivative;
      if(shouldRecompute)
        *shouldRecompute = true;
      if(shouldComputeDerivative)
        *shouldComputeDerivative = showingDerivative;
      return true;
    }
    else
      return false;

  case ToggleDivision:
    showingDivided = ! showingDivided;
    diff.hidden = showingDivided;
    divided.hidden = ! showingDivided;
    return true;

    // Now the quitting commands
  case QuitSubtracting:
    *shouldQuit = true;
  case PushCurrent:
    if(showingDerivative)
      soas().pushDataSet(signal->derivedDataSet(derivative.yvalues, 
                                                QString("_%1_diff.dat").
                                                arg(suffix)), !*shouldQuit);
    else
      soas().pushDataSet(signal->derivedDataSet(diff.yvalues, 
                                                QString("_%1_sub.dat").
                                                arg(suffix)), !*shouldQuit);
    return true;
  case QuitReplacing:
    soas().pushDataSet(signal->derivedDataSet(modified.yvalues, 
                                              QString("_%1.dat").
                                              arg(suffix)));
    *shouldQuit = true;
    return true;
  case QuitDividing:
    soas().pushDataSet(signal->derivedDataSet(divided.yvalues, 
                                              QString("_%1_div.dat").
                                              arg(suffix)));
    *shouldQuit = true;
    return true;
  default:
    return false;
  };
  
  return false;
}

void BaselineHandler::updateBottom()
{
  // Do not update if the size do not match
  if(modified.yvalues.size() == signal->y().size()) {
    diff.yvalues = signal->y() - modified.yvalues;
    divided.yvalues = signal->y()/modified.yvalues;
  }
}

BaselineHandler::~BaselineHandler()
{
}
