/**
   \file data-processing-commands.cc
   Commands to extract information from datasets
   Copyright 2011 by Vincent Fourmond
             2011,2013,2014,2015 by CNRS/AMU

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
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <argument-templates.hh>
#include <terminal.hh>

#include <dataset.hh>
#include <soas.hh>
#include <curveview.hh>
#include <curveeventloop.hh>
#include <curvemarker.hh>
#include <curvedataset.hh>
#include <curveitems.hh>
#include <curvepanel.hh>
#include <math.h>
#include <datastack.hh>

#include <utils.hh>
#include <outfile.hh>

#include <eventhandler.hh>

#include <graphicssettings.hh>

#include <peaks.hh>
#include <idioms.hh>
#include <curve-effectors.hh>




//////////////////////////////////////////////////////////////////////

static void displayPeaks(QList<PeakInfo> peaks, const DataSet * ds,
                         const CommandOptions & opts,
                         int maxnb = -1, bool write = true)
{
  const GraphicsSettings & gs = soas().graphicsSettings();
  CurveView & view = soas().view();
  view.disableUpdates();
  Terminal::out << "Found " << peaks.size() << " peaks" << endl;
  if(maxnb < 0 || maxnb > peaks.size())
    maxnb = peaks.size();
  peaks = peaks.mid(0, maxnb);
  PeakInfo::computeArea(peaks, ds->x(), ds->y());
  
  for(int i = 0; i < maxnb; i++) {
    ValueHash hsh;
    hsh << "buffer" << ds->name 
        << "what" << (peaks[i].isMin ? "min" : "max" )
        << "x" << peaks[i].x << "y" << peaks[i].y 
        << "index" << peaks[i].index
        << "width" << peaks[i].width()
        << "left_width" << peaks[i].leftHHWidth
        << "right_width" << peaks[i].rightHHWidth
        << "area" << peaks[i].area;
    hsh.handleOutput(ds, opts, write);

    if(! i)
      Terminal::out << hsh.keyOrder.join("\t") << endl;
    Terminal::out << hsh.toString() << endl;
    CurveLine * v= new CurveLine;

    v->p1 = QPointF(peaks[i].x, 0);
    v->p2 = QPointF(peaks[i].x, peaks[i].y);
    v->pen = gs.getPen(GraphicsSettings::PeaksPen);
    view.addItem(v);

    v = new CurveLine;
    v->p1 = QPointF(peaks[i].x - peaks[i].leftHHWidth, peaks[i].y/2);
    v->p2 = QPointF(peaks[i].x + peaks[i].rightHHWidth, peaks[i].y/2);
    v->pen = gs.getPen(GraphicsSettings::PeaksPen);
    view.addItem(v);
  }
  view.enableUpdates();
}

static void findPeaksCommand(const QString &name, const CommandOptions & opts)
{
  int window = 8;
  int nb = -1;
  bool write = true;
  if(name == "1")
    nb = 1;
  else if(name == "2")
    nb = 2;
  else
    write = false;

  bool includeBorders = false;

  bool trim = false;
  bool max = false;
  QString which;
  

  updateFromOptions(opts, "window", window);
  updateFromOptions(opts, "peaks", nb);
  updateFromOptions(opts, "include-borders", includeBorders);
  updateFromOptions(opts, "which", which);
  
  if(which == "min") {
    trim = true;
    max = false;
  }
  else if(which == "max") {
    trim = true;
    max = true;
  }
  else
    trim = false;


  const DataSet * ds = soas().currentDataSet();
  Peaks pk(ds, window);

  QList<PeakInfo> peaks = pk.findPeaks(includeBorders);
  if(trim)
    PeakInfo::removeMinMax(peaks, !max);
  if(nb >= 0)
    PeakInfo::sortByMagnitude(peaks);
  displayPeaks(peaks, ds, opts, nb, write);
}


static ArgumentList 
fpBaseOps(QList<Argument *>() 
          << new IntegerArgument("window", 
                                 "Peak window",
                                 "width of the window")
          << new BoolArgument("include-borders",
                              "Include borders",
                              "whether or not to include borders")
          << new ChoiceArgument(QStringList() 
                                << "min" << "max" << "both",
                                "which",
                                "Min or max",
                                "selects which of minima and/or maxima "
                                "to find")
       );

static ArgumentList 
fpOps(QList<Argument *>(fpBaseOps) 
      << new IntegerArgument("peaks", 
                             "Number of peaks",
                             "Display only that many peaks (by order of intensity)")
      << ValueHash::outputOptions()
      );

static ArgumentList 
fpbOps(QList<Argument *>(fpBaseOps) 
      << ValueHash::outputOptions(true)
      );
      
static Command 
fp("find-peaks", // command name
   effector(findPeaksCommand), // action
   "peaks",  // group name
   NULL, // arguments
   &fpOps, // options
   "Find peaks",
   "Find all peaks");

static Command 
fp1("1", // command name
   effector(findPeaksCommand), // action
   "peaks",  // group name
    NULL, // arguments
    &fpbOps, // options
    "Find peak",
    "Find the largest peak");

static Command 
fp2("2", // command name
   effector(findPeaksCommand), // action
   "peaks",  // group name
    NULL, // arguments
    &fpbOps, // options
    "Find two peaks",
    "Find the two largest peaks");

//////////////////////////////////////////////////////////////////////

static void echemPeaksCommand(const QString &, const CommandOptions & opts)
{
  int window = 8;

  int peaks = 10000000;
  updateFromOptions(opts, "pairs", peaks);

  const DataSet * ds = soas().currentDataSet();
  const GraphicsSettings & gs = soas().graphicsSettings();
  Peaks pk(ds, window);

  QList<EchemPeakPair> prs = pk.findPeakPairs();
  QList<EchemPeakPair> pairs;
  /// We select only reversible stuff.
  for(int i = 0; i < prs.size(); i++)
    if(prs[i].isReversible())
      pairs << prs[i];
  
  {
    CurveView & view = soas().view();
    view.disableUpdates();
    Terminal::out << "Found " << pairs.size() << " peaks pairs" << endl;

    int mx = std::min(pairs.size(), peaks);
    for(int i = 0; i < mx; i++) {
      ValueHash hsh;
      hsh << "buffer" << ds->name 
          << "forw_x" << pairs[i].forward.x
          << "forw_y" << pairs[i].forward.y;


      CurveLine * v = new CurveLine;
    
      v->p1 = QPointF(pairs[i].forward.x, 0);
      v->p2 = QPointF(pairs[i].forward.x, pairs[i].forward.y);
      v->pen = gs.getPen(GraphicsSettings::PeaksPen);
      view.addItem(v);

      hsh << "back_x" << pairs[i].backward.x
          << "back_y" << pairs[i].backward.y
          << "delta_x" << pairs[i].deltaX()
          << "delta_y" << pairs[i].deltaY()
          << "avg_x" << pairs[i].midX();
          
      v = new CurveLine;
      v->p1 = QPointF(pairs[i].forward.x, 0);
      v->p2 = QPointF(pairs[i].backward.x, 0);
      v->pen = gs.getPen(GraphicsSettings::PeaksPen);
      view.addItem(v);

      v = new CurveLine;
      v->p1 = QPointF(pairs[i].backward.x, 0);
      v->p2 = QPointF(pairs[i].backward.x, pairs[i].backward.y);
      v->pen = gs.getPen(GraphicsSettings::PeaksPen);
      view.addItem(v);

      Terminal::out << hsh.keyOrder.join("\t") << endl;
      Terminal::out << hsh.toString() << endl;
      hsh.handleOutput(ds, opts, true);
    }
    view.enableUpdates();
  }
}

static ArgumentList 
epOps(QList<Argument *>(fpBaseOps) 
      << new IntegerArgument("pairs", 
                             "Number of peak pairs",
                             "Display (and output) only that many peak pairs (by order of intensity)")
      << ValueHash::outputOptions()
      );


static Command 
ep("echem-peaks", // command name
   effector(echemPeaksCommand), // action
   "peaks",  // group name
   NULL, // arguments
   &epOps, // options
   "Find peaks pairs",
   "Find all peaks pairs");

