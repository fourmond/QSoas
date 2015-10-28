/**
   \file dataset-commands.cc commands for tweaking datasets
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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
#include <terminal.hh>
#include <outfile.hh>

#include <dataset.hh>
#include <soas.hh>
#include <curveview.hh>
#include <curveeventloop.hh>
#include <curvemarker.hh>
#include <curveitems.hh>
#include <curvepanel.hh>

#include <graphicssettings.hh>

#include <pointpicker.hh>
#include <math.h>

#include <expression.hh>

#include <eventhandler.hh>

#include <valuehash.hh>

#include <datastack.hh>
#include <box.hh>

#include <dataseteditor.hh>
#include <statistics.hh>


static Group grp("buffer", 2,
                 "Buffer",
                 "Buffer manipulations");

static Group g1("split", 3,
                "Split...",
                "Cut buffers into bits", &grp);

static Group g2("peaks", 5,
                "Peaks...",
                "Find peaks", &grp);

static Group g0("filters", 6,
                "Filters...",
                "Filter data", &grp);



static Group g3("math", 7,
                "Mathematical operations...",
                "Mathematical operrations on buffers", &grp);

static Group g4("mbuf", 7,
                "Multi-buffer...",
                "Operations involving several buffers", &grp);

static Group g5("segments", 10,
                "Segments...",
                "Handling and use of segments", &grp);

static Group ga("norm", 11,
                "Normalize...",
                "Normalize data", &grp);

static Group gb("stack", 11,
                "Stack...",
                "Manipulate the stack", &grp);

static Group gc("flags", 11,
                "Flags...",
                "Sets and clear flags", &grp);



//////////////////////////////////////////////////////////////////////

/// Splits the given data at dx sign change
static void splitDataSet(const DataSet * ds, bool first)
{
  int idx = ds->deltaSignChange(0);
  if(idx < 0) {
    Terminal::out << QObject::tr("No dx sign change, nothing to do !") 
                  << endl;
    return ;
  }
  DataSet * newDS;
  ds->splitAt(idx, (first ? &newDS : NULL), (first ? NULL : &newDS));
  soas().pushDataSet(newDS);
}
  
static void splitaCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  splitDataSet(ds, true);
}


static Command 
sa("splita", // command name
   optionLessEffector(splitaCommand), // action
   "split",  // group name
   NULL, // arguments
   NULL, // options
   "Split first",
   "Gets buffer until dx sign change",
   "Returns the first part of the buffer, until "
   "the first change of sign of dx");
    
static void splitbCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  splitDataSet(ds, false);
}
        

static Command 
sb("splitb", // command name
   optionLessEffector(splitbCommand), // action
   "split",  // group name
   NULL, // arguments
   NULL, // options
   "Split second",
   "Gets buffer after first dx sign change",
   "Returns the part of the buffer after "
   "the first change of sign of dx");

//////////////////////////////////////////////////////////////////////

static void splitMonotonicCommand(const QString &, 
                                  const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  QStringList flags;
  int group = 1;
  updateFromOptions(opts, "flags", flags);
  updateFromOptions(opts, "group", group);

  QList<DataSet *> nds = ds->splitIntoMonotonic(0, group);


  for(int i = 0; i < nds.size(); i++) {
    if(flags.size() > 0)
      nds[i]->setFlags(flags);
    nds[i]->setMetaData("segment-number", i);
    soas().pushDataSet(nds[i]);
  }
}
        


static ArgumentList 
smOpts(QList<Argument *>() 
           << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                         "flags", 
                                         "Flags",
                                         "Flags to set on the results")
           << new IntegerArgument("group", 
                                  "Group segments",
                                  "Group that many segments into one dataset")
       );


static Command 
sm("split-monotonic", // command name
   effector(splitMonotonicCommand), // action
   "split",  // group name
   NULL, // arguments
   &smOpts, // options
   "Split into monotonic parts",
   "Splits a buffer into subparts where the change in X are monotonic");

//////////////////////////////////////////////////////////////////////

static void transposeCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  soas().pushDataSet(ds->transpose());
}
        

static Command 
tp("transpose", // command name
   optionLessEffector(transposeCommand), // action
   "buffer",  // group name
   NULL, // arguments
   NULL, // options
   "Transpose",
   "Transpose, ie converts a X,Y,Z grid into a Y,X,Z grid");

//////////////////////////////////////////////////////////////////////

static void unwrapCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  Vector newX;
  const Vector & oldX = ds->x();
  for(int i = 0; i < oldX.size(); i++) {
    if(i)
      newX << newX.last() + fabs(oldX[i] - oldX[i-1]);
    else
      newX << oldX[i];
  }
  soas().pushDataSet(ds->derivedDataSet(ds->y(), "_unwrap.dat", newX));
}
        

static Command 
uw("unwrap", // command name
   optionLessEffector(unwrapCommand), // action
   "buffer",  // group name
   NULL, // arguments
   NULL, // options
   "Unwrap",
   "Unwrap the buffer so that X is always increasing");

//////////////////////////////////////////////////////////////////////

static void reverseCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  DataSet * nds = ds->derivedDataSet("_rev.dat");
  nds->reverse();
  soas().pushDataSet(nds);
}
        

static Command 
rv("reverse", // command name
   optionLessEffector(reverseCommand), // action
   "buffer",  // group name
   NULL, // arguments
   NULL, // options
   "Reverse",
   "Reverse the order of all points (changes nothing but the indices)");



//////////////////////////////////////////////////////////////////////

static void sortCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  soas().pushDataSet(ds->sort());
}


static Command 
sort("sort", // command name
     optionLessEffector(sortCommand), // action
     "buffer",  // group name
     NULL, // arguments
     NULL, // options
     "Sort",
     "Sort with ascending X values",
     "");

//////////////////////////////////////////////////////////////////////

static void expandCommand(const QString &,
                          const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  if(ds->nbColumns() <= 2) {
    Terminal::out << "No more than 2 columns in '" << ds->name 
                  << "', nothing to do" << endl;
    return;
  }
  QString pc;
  updateFromOptions(opts, "perp-meta", pc);

  QStringList flags;
  updateFromOptions(opts, "flags", flags);

  Vector ppcd = ds->perpendicularCoordinates();
  for(int i = 1; i < ds->nbColumns(); i++) {
    QList<Vector> cols;
    cols << ds->x();
    cols << ds->column(i);
    DataSet * s = ds->derivedDataSet(cols, QString("_col_%1.dat").arg(i+1));
    if(ppcd.size() >= i) {
      s->setPerpendicularCoordinates(ppcd[i-1]);
      if(!pc.isEmpty())
        s->setMetaData(pc, ppcd[i-1]);
    }

    if(flags.size() > 0)
      s->setFlags(flags);

    
    soas().stack().pushDataSet(s, true);
    if(i > 1)
      soas().view().addDataSet(s);
    else
      soas().view().showDataSet(s);
  }
  Terminal::out << "Expanded '" << ds->name 
                << "' into " << ds->nbColumns() - 1 << " buffers" << endl;
}

static ArgumentList 
expandOpts(QList<Argument *>() 
           << new StringArgument("perp-meta", 
                                 "Perpendicular coordinate",
                                 "Define meta-data from perpendicular coordinate")
           << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                         "flags", 
                                         "Buffers",
                                         "Buffers to flag/unflag"));


static Command 
expand("expand", // command name
     effector(expandCommand), // action
     "buffer",  // group name
     NULL, // arguments
     &expandOpts, // options
     "Expand",
     "Expands a multi-Y dataset",
     "Expands a dataset with many Y columns into as many datasets with one Y column");



//////////////////////////////////////////////////////////////////////

static void renameCommand(const QString &, QString name)
{
  DataSet * ds = soas().currentDataSet();
  ds->name = name;
  soas().view().repaint();
}

static ArgumentList 
renameA(QList<Argument *>() 
        << new StringArgument("new-name", 
                              "New name",
                              "New name "));


static Command 
renameCmd("rename", // command name
          optionLessEffector(renameCommand), // action
          "buffer",  // group name
          &renameA, // arguments
          NULL, // options
          "Rename",
          "??",
          "??", "a");


//////////////////////////////////////////////////////////////////////

static void chopCommand(const QString &, QList<double> values, 
                        const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  QList<DataSet *> splitted;
  
  /// target for indices
  QList<int> * indices = NULL;
  bool setSegs = false;
  updateFromOptions(opts, "set-segments", setSegs);
  if(setSegs)
    indices = new QList<int>(ds->segments);

  QStringList flags;
  updateFromOptions(opts, "flags", flags);
  
  if(testOption<QString>(opts, "mode", "index") ||
     testOption<QString>(opts, "mode", "indices")) {
    QList<int> split;
    for(int i = 0; i < values.size(); i++)
      split << values[i];
    if(indices)
      *indices += split;
    else
      splitted = ds->chop(split);
  }
  else if(testOption<QString>(opts, "mode", "xvalues"))
    splitted = ds->chop(values, false, indices);
  else 
    splitted = ds->chop(values, true, indices);
  if(indices) {
    DataSet * nds = new DataSet(*ds);
    nds->segments = *indices;
    soas().pushDataSet(nds);
    for(int i = 0; i < splitted.size(); i++)
      delete splitted[i];
    delete indices;
  }
  else
    for(int i = splitted.size() - 1; i >= 0; i--) {
      if(flags.size() > 0)
        splitted[i]->setFlags(flags);
      soas().pushDataSet(splitted[i]);
    }
}

static ArgumentList 
chopA(QList<Argument *>() 
      << new SeveralNumbersArgument("lengths", 
                                    "Lengths",
                                    "Lengths of the subsets"));

static ArgumentList 
chopO(QList<Argument *>() 
      << new ChoiceArgument(QStringList()
                            << "deltax"
                            << "xvalues"
                            << "indices"
                            << "index",
                            "mode", 
                            "Mode",
                            "Whether to cut on index or x values (default)")
      << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                    "flags", 
                                    "Buffers",
                                    "Buffers to flag/unflag")      
      << new BoolArgument("set-segments", 
                          "Set segments",
                          "Whether to actually cut the dataset, or just "
                          "to set segments where the cuts would have "
                          "been")
      );

static Command 
chopC("chop", // command name
      effector(chopCommand), // action
      "split",  // group name
      &chopA, // arguments
      &chopO, // options
      "Chop Buffer",
      "Cuts buffer based on X values",
      "Cuts the buffer into several subsets of the lengths given "
      "as arguments");

//////////////////////////////////////////////////////////////////////

static void chopIntoSegmentsCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  QList<DataSet *> splitted = ds->chopIntoSegments();
  for(int i = splitted.size() - 1; i >= 0; i--)
    soas().pushDataSet(splitted[i]);
}

static Command 
chopS("segments-chop", // command name
      optionLessEffector(chopIntoSegmentsCommand), // action
      "segments",  // group name
      NULL, // arguments
      NULL, // options
      "Chop into segments",
      "Cuts buffer based on predefined segments",
      "Cuts the buffer into several ones based on the segments defined "
      "using set-segments or find-step /set-segments");

//////////////////////////////////////////////////////////////////////

namespace __cu {

  typedef enum {
    PickPoint,
    PickRef,
    QuitSubtracting,
    QuitDividing,
    QuitSubtractingRespRef,
    QuitDividingRespRef,
    WriteToOutput,
    Quit
  } CursorActions;

  static EventHandler cursorHandler = EventHandler("cursor").
    addClick(Qt::LeftButton, PickPoint, "place cursor").
    addClick(Qt::RightButton, PickRef, "place reference").
    addKey('v', QuitDividing, "quit dividing by Y value").
    alsoKey('V').
    addKey('u', QuitSubtracting, "quit subtracting Y value").
    alsoKey('U').
    addKey(Qt::CTRL + 'u', QuitSubtractingRespRef, "subtract y - yref").
    addKey(Qt::CTRL + 'v', QuitDividingRespRef, "divide by y/yref").
    addPointPicker().
    addKey(Qt::Key_Escape, Quit, "quit").
    alsoKey('q').alsoKey('Q').
    addClick(Qt::MiddleButton, WriteToOutput, "write to output").
    alsoKey(' ');



static void cursorCommand(CurveEventLoop &loop, const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  CurveMarker m;
  CurveMarker r;
  CurveView & view = soas().view();
  PointPicker pick(&loop, ds);
  pick.trackedButtons = Qt::LeftButton|Qt::RightButton;

  view.addItem(&m);
  m.size = 4;
  m.pen = QPen(Qt::NoPen);
  m.brush = QBrush(QColor(0,0,255,100)); // A kind of transparent blue

  view.addItem(&r);
  r.size = 4;
  r.pen = QPen(Qt::NoPen);
  r.brush = QBrush(QColor(0,128,0,100)); // A kind of transparent green
  loop.setHelpString("Cursor:\n" +
                     cursorHandler.buildHelpString());

  Terminal::out << "Point positions:\nx\ty\tindex\tx-xr\ty-yr\tx/xr\ty/yr" << endl;
  QString cur;
  ValueHash e;
  while(! loop.finished()) {

    int action = cursorHandler.nextAction(loop);
    pick.processEvent(action);
    switch(action) {
    case PickPoint:
      m.p = pick.point();
      e.clear();
      e << "x" << m.p.x() << "y" << m.p.y()
        << "index" << pick.pointIndex() 
        << "x-xr" << m.p.x() - r.p.x() 
        << "y-yr" << m.p.y() - r.p.y() 
        << "x/xr" << m.p.x()/r.p.x()
        << "y/yr" << m.p.y()/r.p.y();
      Terminal::out << e.toString() << endl;
      break;
    case PickRef:
      r.p = pick.point();
      Terminal::out << "Reference:\t"  << r.p.x() << "\t" 
                    << r.p.y() << endl;
      break;
    case Quit:
        return;
    case QuitSubtracting: {
      Vector ny = ds->y() - m.p.y();
      Terminal::out << "Subtracting Y value: " << m.p.y() << endl;
      soas().pushDataSet(ds->derivedDataSet(ny, "_sub.dat"));
      return;
    }
    case QuitDividing: {
      Vector ny = ds->y()/m.p.y();
      Terminal::out << "Dividing by Y value: " << m.p.y() << endl;
      soas().pushDataSet(ds->derivedDataSet(ny, "_div.dat"));
      return;
    }
    case QuitSubtractingRespRef: {
      Vector ny = ds->y() - (m.p.y() - r.p.y());
      Terminal::out << "Subtracting: " << m.p.y() - r.p.y() << endl;
      soas().pushDataSet(ds->derivedDataSet(ny, "_sub.dat"));
      return;
    }
    case QuitDividingRespRef: {
      Vector ny = ds->y()/(m.p.y()/r.p.y());
      Terminal::out << "Dividing by: " << m.p.y()/r.p.y() << endl;
      soas().pushDataSet(ds->derivedDataSet(ny, "_div.dat"));
      return;
    }
    case WriteToOutput:
      e.prepend("buffer", ds->name);
      Terminal::out << "Writing position to output file: '" 
                    << OutFile::out.fileName() << "'" << endl;
      
      OutFile::out.writeValueHash(e, ds);
    default:
      ;
    }
  }
}

static Command 
cu("cursor", // command name
   optionLessEffector(cursorCommand), // action
   "buffer",  // group name
   NULL, // arguments
   NULL, // options
   "Cursor",
   "Display cursors on the curve",
   "Displays cursors on the curve",
   "cu");
};

//////////////////////////////////////////////////////////////////////

namespace __cut {
  typedef enum {
    LeftPick,
    RightPick,
    SelectInside,
    ToggleXValues,
    SelectOutside,
    RemoveInside,
    Abort
  } CutActions;

  static EventHandler cutHandler = EventHandler("cut").
    addClick(Qt::LeftButton, LeftPick, "pick left bound").
    addClick(Qt::RightButton, RightPick, "pick right bound").
    conventionalAccept(SelectInside, "keep only the inside").
    addKey('u', SelectOutside, "keep only the outside").
    addKey('U', SelectOutside).
    addKey('r', RemoveInside, "remove inside and go on").
    addKey('R', RemoveInside).
    addKey('x', ToggleXValues, "toggles the use of real X values").
    addKey('X', ToggleXValues).
    addKey(Qt::Key_Escape, Abort, "cancel");

  static void cutCommand(CurveEventLoop &loop, const QString &)
  {
    const DataSet * ds = soas().currentDataSet();
    const DataSet * origDs = ds;
    const GraphicsSettings & gs = soas().graphicsSettings();

    CurveHorizontalRegion r;
    CurveView & view = soas().view();

    /// We remove the current display
    view.clear();
    CurveData d;
    Vector indices;
    view.addItem(&r);
    view.addItem(&d);

    d.pen = gs.getPen(GraphicsSettings::ResultPen);

    view.mainPanel()->xLabel = "Index";

    loop.setHelpString("Cut:\n" + cutHandler.buildHelpString());

    d.countBB = true;
    d.yvalues = ds->y();
    indices = d.yvalues;
    for(int i = 0; i < indices.size(); i++)
      indices[i] = i;
    d.xvalues = indices;
    r.xleft = 0;
    r.xright = d.xvalues.size()-1;

    bool showIndices = true;
    bool hasChanged = false;    // whether the selection changed or not

    int left;
    int right;

    while(! loop.finished()) {
      switch(cutHandler.nextAction(loop)) {
      case SelectInside: 
        r.getClosestIndices(showIndices ? indices : ds->x(), &left, &right);
        soas().pushDataSet(hasChanged ? ds->subset(left, right, true) : 
                           new DataSet(*ds));
        return;
      case LeftPick:
      case RightPick:
        hasChanged = true;
        r.setX(loop.position().x(), loop.button());
        break;
      case Abort:
        if(ds != origDs)
          delete ds;
        view.addDataSet(origDs);  // To turn its display back on
        return;
      case SelectOutside:
        r.getClosestIndices(showIndices ? indices : ds->x(), &left, &right);
        soas().pushDataSet(ds->subset(left, right, false));
        return;
      case ToggleXValues:
        d.xvalues = showIndices ? ds->x() : indices;
        showIndices = ! showIndices;
        break;
      case RemoveInside: {
        r.getClosestIndices(showIndices ? indices : ds->x(), &left, &right);
        DataSet * nds = ds->subset(left, right, false);
        nds->name = origDs->cleanedName() + "_perf.dat";
        if(ds != origDs)
          delete ds;
        ds = nds;
        d.yvalues = ds->y();
        indices = d.yvalues;
        for(int i = 0; i < indices.size(); i++)
          indices[i] = i;
        d.xvalues = showIndices ? indices : ds->x();
        r.xleft = d.xvalues[0];
        r.xright = d.xvalues.last();
        hasChanged = false;
        break;
      }
      default:
        ;
      }
    }
  }

  static Command 
  cut("cut", // command name
      optionLessEffector(cutCommand), // action
      "split",  // group name
      NULL, // arguments
      NULL, // options
      "Cut",
      "Cuts the current curve",
      "Cuts bits from the current curve",
      "c");
};

//////////////////////////////////////////////////////////////////////

namespace __ee {
  typedef enum {
    LeftPick,
    RightPick,
    SetInside,
    SetOutside,
    ToggleXValues,
    Accept,
    Abort
  } EEActions;

  static EventHandler eeHandler = EventHandler("edit-errors").
    addClick(Qt::LeftButton, LeftPick, "pick left bound").
    addClick(Qt::RightButton, RightPick, "pick right bound").
    addKey('i', SetInside, "set error inside").
    alsoKey('I').
    addKey('o', SetOutside, "set error outside").
    alsoKey('O').
    addKey('x', ToggleXValues, "toggles the use of real X values").
    addKey('X', ToggleXValues).
    conventionalAccept(Accept, "done editing").
    addKey(Qt::Key_Escape, Abort, "cancel");

  static void eeCommand(CurveEventLoop &loop, const QString &)
  {
    const DataSet * origDs = soas().currentDataSet();
    DataSet * ds = origDs->derivedDataSet(origDs->allColumns(), "_edit.dat");
    const GraphicsSettings & gs = soas().graphicsSettings();

    CurveHorizontalRegion r;
    CurveView & view = soas().view();

    /// We remove the current display
    view.clear();
    CurveDataSet d(ds);
    d.pen = gs.dataSetPen(0);
    view.addItem(&r);
    view.addItem(&d);

    view.mainPanel()->xLabel = "Index";

    loop.setHelpString("Edit errors:\n" + eeHandler.buildHelpString());

    { 
      Vector & x = ds->x();
      for(int i = 0; i < x.size(); i++)
        x[i] = i;
      r.xleft = 0;
      r.xright = x.size()-1;
    }

    bool showIndices = true;
    Vector indices = ds->x();

    if(! ds->options.hasYErrors()) {
      Vector ne = ds->y();
      double a,b;
      ne.stats(&a, &b);
      b = sqrt(b);
      Terminal::out << "Deducing error from standard deviation: " << b << endl;
      for(int i = 0; i < ne.size(); i++)
        ne[i] = b;
      ds->appendColumn(ne);
      ds->options.setYErrors(ds->nbColumns() - 1);
    }

    while(! loop.finished()) {
      int act = eeHandler.nextAction(loop);
      switch(act) {
      case LeftPick:
      case RightPick:
        r.setX(loop.position().x(), loop.button());
        break;
      case Abort:
        delete ds;
        view.addDataSet(origDs);  // To turn its display back on
        return;
      case ToggleXValues:
        ds->x() = showIndices ? origDs->x() : indices;
        showIndices = ! showIndices;
        break;
      case SetInside:
      case SetOutside: {
        bool ok = false;
        QString str = loop.promptForString("Set the error value:", &ok);
        if(! ok)
          break;
        double val = str.toDouble(&ok);
        if(! ok)
          break;

        
        int left, right;
        r.getClosestIndices(ds->x(), &left, &right, true);
        if(act == SetInside) {
          for(int i = left; i < right; i++)
            ds->options.setYError(ds, i, val);
        }
        else {
          for(int i = 0; i < left; i++)
            ds->options.setYError(ds, i, val);
          for(int i = right; i < ds->nbRows(); i++)
            ds->options.setYError(ds, i, val);
        }
        break;
      }
      case Accept:
        ds->x() = origDs->x();
        soas().pushDataSet(ds);
        return;
      default:
        ;
      }
    }
  }

  static Command 
  ee("edit-errors", // command name
     optionLessEffector(eeCommand), // action
     "buffer",  // group name
     NULL, // arguments
     NULL, // options
     "Edit errors",
     "Manually edit errors", "...");
};

//////////////////////////////////////////////////////////////////////

// ohhhh, an ugly macro !
#define limits_do(lcn, cn)                \
  if(std::isnan(lcn)) { lcn = bb.lcn();}; \
  if(std::isinf(lcn)) { lcn = cz.lcn();}; \
  fnl.set##cn(lcn)

#include <debug.hh>

/// @todo Pass that to BaselineHandler
static void limitsCommand(const QString &, double left, double right, double bottom, double top, const CommandOptions &)
{
  soas().currentDataSet(); // to ensure datasets are loaded
  CurveView & view = soas().view();
  CurvePanel * panel = view.mainPanel();
  Box cz = panel->currentZoom();
  Box fnl = cz;
  Box bb = panel->overallBB();

  limits_do(left, Left);
  limits_do(right, Right);
  limits_do(top, Top);
  limits_do(bottom, Bottom);

  panel->zoomIn(fnl.toRectF(), true);
  view.update();
}

static ArgumentList 
lA(QList<Argument *>() 
   << new NumberArgument("left", "Left", "Left limit", false, true)
   << new NumberArgument("right", "Right", "Right limit", false, true)
   << new NumberArgument("bottom", "Bottom", "Bottom limit", false, true)
   << new NumberArgument("top", "Top", "Top limit", false, true)
   );


static Command 
li("limits", // command name
   effector(limitsCommand), // action
   "buffer",  // group name
   &lA, // arguments
   NULL, // options
   "Set limits",
   "Set limits for the display");

//////////////////////////////////////////////////////////////////////


/// @todo Pass that to BaselineHandler
static void zoomCommand(CurveEventLoop &loop, const QString &)
{
  soas().currentDataSet(); // to ensure datasets are loaded
  CurveRectangle r;
  CurveView & view = soas().view();
  CurvePanel * panel = NULL;
  view.addItem(&r);
  r.pen = QPen(Qt::DotLine); /// @todo customize this
  r.brush = QBrush(QColor(0,0,255,50)); // A kind of transparent blue

  loop.setHelpString(QObject::tr("Zoom:\n"
                                 "click and drag\n"
                                 "c: reset\n"
                                 "z,Z,Ctrl wheel: in/out\n"
                                 "x,X,wheel: X in/out\n"
                                 "y,Y,Shift wheel: Y in/out\n"
                                 "q or ESC to quit"));
  while(! loop.finished()) {
    switch(loop.type()) {
    case QEvent::MouseMove:
      if(panel) {
        r.p2 = loop.position(panel);
        soas().
          showMessage(QObject::tr("Zoom from %1,%2 to %3,%4").
                      arg(r.p1.x()).arg(r.p1.y()).
                      arg(r.p1.x()).arg(r.p2.y()));
      }
      else
        soas().showMessage(QObject::tr("Point: %1,%2").
                           arg(loop.position().x()).
                           arg(loop.position().y()));
      break;
    case QEvent::MouseButtonPress: 
      /// @todo cancel zoom.
      if(panel) {
        QRectF z(r.p1, loop.position(panel));
        panel->zoomIn(z.normalized());
        panel = NULL;
        r.setRect(QRectF());
      }
      else {
        panel = loop.currentPanel();
        if(panel) {
          r.p1 = loop.position(panel);
          r.p2 = r.p1;
        }
      }
    case QEvent::KeyPress: {
      if(loop.key() == 'q' || loop.key() == 'Q')
        return;
      if(loop.key() == Qt::Key_Escape) {
        if(panel) {
          panel = NULL;
          r.setRect(QRectF());
        }
        else
          return;
        break;
      }
      CurvePanel * p = loop.currentPanel();
      if(!p)
        break;
      switch(loop.key()) {
      case 'c':
      case 'C':
        p->resetZoom();
        break;
      case 'z':
        p->zoomIn(loop.position(p));
        break;
      case 'Z':
        p->zoomIn(loop.position(p), -1.0);
        break;
      case 'x':
        p->zoomIn(loop.position(p), Qt::Horizontal);
        break;
      case 'X':
        p->zoomIn(loop.position(p), Qt::Horizontal, -1);
        break;
      case 'y':
        p->zoomIn(loop.position(p), Qt::Vertical);
        break;
      case 'Y':
        p->zoomIn(loop.position(p), Qt::Vertical, -1);
        break;
      default:
        ;
      }
    }
      break;
    default:
      ;
    }
  }
}

static Command 
zo("zoom", // command name
   optionLessEffector(zoomCommand), // action
   "buffer",  // group name
   NULL, // arguments
   NULL, // options
   "Zoom",
   "Zooms on the curve",
   "Zooms on the current curve",
   "z");

//////////////////////////////////////////////////////////////////////


/// @todo Maybe most of the code shared between this and divide should
/// be shared ?
/// (and some with average ?)
static void subCommand(const QString &, QList<const DataSet *> a, 
                       DataSet * b, const CommandOptions & opts)
{
  bool naive = testOption<QString>(opts, "mode", "indices");
  bool useSteps = false;
  updateFromOptions(opts, "use-segments", useSteps);

  for(int i = 0; i < a.size(); i++) {
    const DataSet * ds = a[i];
    Terminal::out << QObject::tr("Subtracting buffer '%1' from buffer '%2'").
      arg(b->name).arg(ds->name) 
                  << (naive ? " (index mode)" : " (xvalues mode)" ) 
                  << endl;

    soas().pushDataSet(ds->subtract(b, naive, useSteps));
  }
}

static ArgumentList 
operationArgs(QList<Argument *>() 
              << new SeveralDataSetArgument("first", 
                                            "Buffer",
                                            "First buffer(s)")
              << new DataSetArgument("second", 
                                     "Buffer",
                                     "Second buffer"));

static ArgumentList 
operationOpts(QList<Argument *>() 
              << new ChoiceArgument(QStringList() 
                                    << "xvalues"
                                    << "indices",
                                    "mode", 
                                    "Operation mode",
                                    "Whether operations try to match x "
                                    "values or indices")
              << new BoolArgument("use-segments", 
                                  "Use segments ?",
                                  "If on, operations are performed "
                                  "segment-by-segment"));


static Command 
sub("subtract", // command name
    effector(subCommand), // action
    "mbuf",  // group name
    &operationArgs, // arguments
    &operationOpts, // options
    "Subtract",
    "Subtract on buffer from another",
    "Subtract the second buffer from the first. "
    "Works with several 'first' buffers.",
    "S");

//////////////////////////////////////////////////////////////////////


static void divCommand(const QString &, QList<const DataSet *> a,
                       DataSet * b, const CommandOptions & opts)
{
  bool naive = testOption<QString>(opts, "mode", "indices");
  bool useSteps = false;
  updateFromOptions(opts, "use-segments", useSteps);

  for(int i = 0; i < a.size(); i++) {
    const DataSet * ds = a[i];
    Terminal::out << QObject::tr("Dividing buffer '%2' by buffer '%1'").
      arg(b->name).arg(ds->name) 
                  << (naive ? " (index mode)" : " (xvalues mode)" ) 
                  << endl;
    soas().pushDataSet(ds->divide(b, naive, useSteps));
  }
}

static Command 
divc("div", // command name
     effector(divCommand), // action
     "mbuf",  // group name
     &operationArgs, // arguments
     &operationOpts, // options
     "Divide",
     "Divide one buffer by another",
     "Divide the first buffer by the second");

//////////////////////////////////////////////////////////////////////


static void mergeCommand(const QString &, QList<const DataSet *> a,
                         DataSet * b, const CommandOptions & opts)
{
  bool naive = testOption<QString>(opts, "mode", "indices");
  bool useSteps = false;
  updateFromOptions(opts, "use-segments", useSteps);
  
  for(int i = 0; i < a.size(); i++) {
    const DataSet * ds = a[i];
    Terminal::out << QObject::tr("Merging buffer '%2' with buffer '%1'").
      arg(b->name).arg(ds->name) 
                  << (naive ? " (index mode)" : " (xvalues mode)" ) 
                  << endl;
    soas().pushDataSet(ds->merge(b, naive, useSteps));
  }
}

static Command 
mergec("merge", // command name
       effector(mergeCommand), // action
       "mbuf",  // group name
       &operationArgs, // arguments
       &operationOpts, // options
       "Merge buffers on X values",
       "Merge two buffer based on X values",
       "Merge the second buffer with the first one, and keep Y "
       "of the second as a function of Y of the first");

//////////////////////////////////////////////////////////////////////


static void contractCommand(const QString &, QList<const DataSet *> a,
                            const CommandOptions & opts)
{
  bool naive = testOption<QString>(opts, "mode", "indices");
  bool useSteps = false;
  updateFromOptions(opts, "use-segments", useSteps);

  QString pc;
  updateFromOptions(opts, "perp-meta", pc);
  QList<int> useCols;
  updateFromOptions(opts, "use-columns", useCols);
  

  if(a.size() < 2)
    throw RuntimeError("You need more than one buffer to run contract");
  
  DataSet * cur = new DataSet(*a[0]);
  if(pc.size() > 0)
    cur->setPerpendicularCoordinates(cur->getMetaData(pc).toDouble());
  
  for(int i = 1; i < a.size(); i++) {
    DataSet * ds = new DataSet(*a[i]);
    if(pc.size() > 0)
      ds->setPerpendicularCoordinates(ds->getMetaData(pc).toDouble());

    DataSet * n = cur->contract(ds, naive, useSteps, useCols);
    delete cur;
    delete ds;
    cur = n;
  }
  if(! pc.isEmpty()) {
    QList<QVariant> lst;
    for(int i = 0; i < cur->perpendicularCoordinates().size(); i++)
      lst << cur->perpendicularCoordinates()[i];
    cur->setMetaData(pc, lst);
  }
  soas().pushDataSet(cur);
}

static ArgumentList 
contractArgs(QList<Argument *>() 
             << new SeveralDataSetArgument("buffers", 
                                           "Buffers",
                                           "Buffers to contract"));

static ArgumentList 
contractOpts(QList<Argument *>(operationOpts) 
             << new StringArgument("perp-meta", 
                                   "Perpendicular coordinate",
                                   "Define the perpendicular coordinate from meta-data")
             << new SeveralColumnsArgument("use-columns", 
                                           "The columns to use",
                                           "If specified, use only the given columns for the contraction"));

static Command 
contractc("contract", // command name
          effector(contractCommand), // action
          "mbuf",  // group name
          &contractArgs, // arguments
          &contractOpts, // options
          "Group buffers on X values",
          "Group buffers into a X,Y1,Y2",
          "");

//////////////////////////////////////////////////////////////////////

static void avgCommand(const QString &, QList<const DataSet *> all,
                       const CommandOptions & opts)
{
  bool naive = testOption<QString>(opts, "mode", "indices");
  bool useSteps = false;
  updateFromOptions(opts, "use-segments", useSteps);
  bool autosplit = (all.size() == 1);
  updateFromOptions(opts, "split", autosplit);
  bool count = false;
  updateFromOptions(opts, "count", count);

  if(naive && autosplit)
    Terminal::out << "Using mode indices and split at the same "
      "time probably isn't a very good idea. "
      "Proceeding nonetheless" << endl;


  // The idea behind the average is add a column with the number 1
  QList<DataSet * > data;
  if(autosplit) {
    for(int i = 0; i < all.size(); i++) {
      QList<DataSet * > splt = all[i]->splitIntoMonotonic();
      Terminal::out << "Split " << all[i]->name << " into " 
                    << splt.size() << " parts" << endl;
      data.append(splt);
    }
  }
  else {
    for(int i = 0; i < all.size(); i++)
      data << new DataSet(*all[i]);
  }

  Terminal::out << "Averaging over " << data.size() <<  " buffers" << endl;

  // Now, we modify all the datasets to add a column filled with 1 as
  // the third column (we'll shift that as last later on).

  for(int i = 0; i < data.size(); i++)
    data[i]->insertColumn(2, Vector(data[i]->nbRows(), 1));

  // Now, perform all the additions
  DataSet * ret = data.takeFirst();
  for(int i = 0; i < data.size(); i++) {
    DataSet * nr = ret->add(data[i]);
    delete ret;                 // What a memory waste !
    delete data[i];             // Same here
    ret = nr;
  }

  Vector avg = ret->takeColumn(2);   // The number column
  for(int i = 0; i < ret->nbRows(); i++) {
    for(int j = 1; j < ret->nbColumns(); j++)
      ret->column(j)[i] /= avg[i];
  }

  if(count)
    ret->insertColumn(ret->nbColumns(), avg);        // For memory

  QStringList names;
  for(int i = 0; i < all.size(); i++)
    names << all[i]->cleanedName();
  names << "avg";
  ret->name = names.join("_") + ".dat";
  soas().pushDataSet(ret);
}

static ArgumentList 
aveArgs(QList<Argument *>() 
              << new SeveralDataSetArgument("buffers", 
                                            "Buffer",
                                            "Buffers"));

static ArgumentList 
aveOpts(QList<Argument *>(operationOpts)
        << new BoolArgument("split", 
                            "Split into monotonic parts",
                            "If on, buffers are automatically "
                            "split into monotonic parts before averaging.")
        << new BoolArgument("count", 
                            "Adds a number count",
                            "If on, a last column contains the number "
                            "of averaged points for each value")
        );


static Command 
ave("average", // command name
    effector(avgCommand), // action
    "mbuf",  // group name
    &aveArgs, // arguments
    &aveOpts, // options
    "Average",
    "Average buffers",
    "Average all buffers, possibly splitting them into monotonic parts if "
    "applicable");

//////////////////////////////////////////////////////////////////////



static void catCommand(const QString &, DataSet * a, 
                       QList<const DataSet *> b, const CommandOptions & opts)
{
  b.prepend(a);
  bool setSegs = true;
  updateFromOptions(opts, "add-segments", setSegs);
  soas().pushDataSet(DataSet::concatenateDataSets(b, setSegs));
}

static ArgumentList 
catArgs(QList<Argument *>() 
        << new DataSetArgument("first", 
                               "Buffer",
                               "First buffer")
        << new SeveralDataSetArgument("second", 
                                      "Buffer",
                                      "Second buffer(s)"));


static ArgumentList 
catOpts(QList<Argument *>()
        << new BoolArgument("add-segments", 
                            "Add segments",
                            "If on (default) segments are added between "
                            "the old buffers"));

static Command 
cat("cat", // command name
    effector(catCommand), // action
    "mbuf",  // group name
    &catArgs, // arguments
    &catOpts, // options
    "Concatenate",
    "Conc",
    "Conc",
    "i");

//////////////////////////////////////////////////////////////////////


static void shiftxCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  Vector nx = ds->x();
  double x = nx[0];
  Terminal::out << QObject::tr("Shifting X axis by %1").
    arg(x) << endl;
  nx -= x;
  soas().pushDataSet(ds->derivedDataSet(ds->y(), "_shiftx.dat", nx));
}

static Command 
shiftx("shiftx", // command name
       optionLessEffector(shiftxCommand), // action
       "norm",  // group name
       NULL, // arguments
       NULL, // options
       "Shift X values",
       "Shift X values so that x[0] = 0",
       "Shift X values so that x[0] = 0");

//////////////////////////////////////////////////////////////////////


static void statsOn(const DataSet * ds, bool output, const ValueHash & meta)
{
  Statistics stats(ds);

  ValueHash os;
  QList<ValueHash> byCols = stats.statsByColumns(&os);

  Terminal::out << "Statistics on buffer: " << ds->name << ":";
  for(int i = 0; i < ds->nbColumns(); i++)
    Terminal::out << "\n" << byCols[i].prettyPrint();

  Terminal::out << endl;
  if(output) {
    os.merge(meta);
    Terminal::out << "Writing stats to output file" << endl;
    OutFile::out.writeValueHash(os, ds);
  }
}

static void statsCommand(const QString &, const CommandOptions & opts)
{
  DataSet * ds = soas().currentDataSet();
  bool output = false;
  updateFromOptions(opts, "buffer", ds);
  updateFromOptions(opts, "output", output);
  bool bySegments = false;
  updateFromOptions(opts, "use-segments", bySegments);
  QStringList metaNames;
  updateFromOptions(opts, "meta", metaNames);

  ValueHash meta;
  if(metaNames.size() > 0) {
    const ValueHash & origMeta = ds->getMetaData();
    for(int i = 0; i < metaNames.size(); i++) {
      const QString & n = metaNames[i];
      if(origMeta.contains(n))
        meta << n << origMeta[n];
      else 
        Terminal::out << "Requested meta '" << n 
                      << "' but it is missing from buffer" << endl;
    }
  }

  if(bySegments) {
    QList<DataSet * > segs = ds->chopIntoSegments();
    for(int i = 0; i < segs.size(); i++) {
      statsOn(segs[i], output, meta);
      delete segs[i];
    }
  }
  else
    statsOn(ds, output, meta);
}

static ArgumentList 
statsO(QList<Argument *>() 
       << new DataSetArgument("buffer", 
                              "Buffer",
                              "An alternative buffer to get information on",
                              true)
       << new BoolArgument("output", 
                           "To output file",
                           "Also write stats to output file")
       << new BoolArgument("use-segments", 
                           "Use segments",
                           "Make statistics segments by segment")
       << new SeveralStringsArgument(QRegExp("\\s*,\\s*"), "meta", 
                                     "Meta-data",
                                     "When writing to output file, also print the listed meta-data")
       );


static Command 
stats("stats", // command name
      effector(statsCommand), // action
      "buffer",  // group name
      NULL, // arguments
      &statsO, // options
      "Statistics",
      "Statistics",
      "...");

//////////////////////////////////////////////////////////////////////


static void generateDSCommand(const QString &, double beg, double end, 
                              const CommandOptions & opts)
{
  int samples = 1000;
  updateFromOptions(opts, "samples", samples);

  QString formula;
  updateFromOptions(opts, "formula", formula);

  Vector x = Vector::uniformlySpaced(beg, end, samples);
  Vector y = x;
  if(! formula.isEmpty()) {
    QStringList vars;
    vars << "x" << "i";
    Expression expr(formula, vars);
    /// @todo have a global way to incorporate all "constants"
    /// (temperature and fara) into that.
    double v[2];
    for(int i = 0; i < x.size(); i++) {
      v[0] = x[i];
      v[1] = i;
      y[i] = expr.evaluate(v);
    }
  }

  DataSet * newDs = new DataSet(x,y);
  newDs->name = "generated.dat";
  soas().pushDataSet(newDs);
}

static ArgumentList 
gDSA(QList<Argument *>() 
       << new NumberArgument("start", 
                             "Start",
                             "The first X value")
       << new NumberArgument("end", 
                             "End",
                             "The last X value")
       );

static ArgumentList 
gDSO(QList<Argument *>() 
       << new IntegerArgument("samples", 
                              "Number of samples",
                              "The number of samples")
       << new StringArgument("formula", 
                             "The Y values",
                             "Formula to generate the Y values",
                             true)
     );


static Command 
gDS("generate-buffer", // command name
    effector(generateDSCommand), // action
    "math",  // group name
    &gDSA, // arguments
    &gDSO, // options
    "Generate buffer",
    "Generate a ramp",
    "...");

//////////////////////////////////////////////////////////////////////


static void setMetaCommand(const QString &, QString meta, QString value, 
                           const CommandOptions & opts)
{
  DataSet * ds = soas().currentDataSet();
  // Attempt to convert to double
  bool ok;
  double val = value.toDouble(&ok);
  if(ok)
    ds->setMetaData(meta, val);
  else
    ds->setMetaData(meta, value);
}

static ArgumentList 
sMA(QList<Argument *>() 
    << new StringArgument("name", 
                          "Name",
                          "The name of the meta-data")
    << new StringArgument("value", 
                          "Value",
                          "The meta-data value")
   );

// static ArgumentList 
// sMO(QList<Argument *>() 
//     );


static Command 
sM("set-meta", // command name
   effector(setMetaCommand), // action
   "buffer",  // group name
   &sMA, // arguments
   NULL, // options
   "Set meta-data",
   "Set meta-data", "...");

//////////////////////////////////////////////////////////////////////


static void setPerpCommand(const QString &, QList<double> coords,
                           const CommandOptions & opts)
{
  DataSet * ds = soas().currentDataSet();
  ds->setPerpendicularCoordinates(coords.toVector());
}

static ArgumentList 
sPA(QList<Argument *>() 
    << new SeveralNumbersArgument("coords", 
                                  "Coordinates",
                                  "The values of the coordinates (one for each Y column)")
   );

// static ArgumentList 
// sMO(QList<Argument *>() 
//     );


static Command 
sP("set-perp", // command name
   effector(setPerpCommand), // action
   "buffer",  // group name
   &sPA, // arguments
   NULL, // options
   "Set perpendicular",
   "Set perpendicular coordinates", "...");

//////////////////////////////////////////////////////////////////////


static void editCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  DatasetEditor e(ds);
  e.exec();
}

static Command 
edit("edit", // command name
     optionLessEffector(editCommand), // action
     "buffer",  // group name
     NULL, // arguments
     NULL, // options
     "Edit dataset",
     "Display a table to edit the dataset");

//////////////////////////////////////////////////////////////////////

static void tweakColumnsCommand(const QString &, 
                                const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  QList<Vector> cols = ds->allColumns();

  /// @todo Swap columns

  bool flip = false;
  bool flipAll = false;
  
  updateFromOptions(opts, "flip", flip);
  updateFromOptions(opts, "flip-all", flipAll);
  if(flipAll) {
    Utils::reverseList(cols);
  }
  else if(flip) {
    Vector x = cols.takeFirst();
    Utils::reverseList(cols);
    cols.insert(0, x);
  }


  // Removing last
  QList<int> toRemove;
  updateFromOptions(opts, "remove", toRemove);
  if(toRemove.size()) {
    qSort(toRemove);
    for(int i = toRemove.size() - 1; i >= 0; i--)
      cols.removeAt(toRemove[i]);
  }

  DataSet * nds = ds->derivedDataSet(cols, "_tweaked.dat");
  soas().pushDataSet(nds);
}

static ArgumentList 
tcA;

static ArgumentList 
tcO(QList<Argument *>() 
    << new SeveralColumnsArgument("remove", 
                                  "Columns to remove",
                                  "The column numbers to remove "
                                  "(X = 1, Y = 2, etc...)", true)
    << new BoolArgument("flip", 
                        "Flip Y columns",
                        "If true, flips all the Y columns")
    << new BoolArgument("flip-all", 
                        "Flip all columns",
                        "If true, flips all the columns, including the X column")
    );


static Command 
tc("tweak-columns", // command name
   effector(tweakColumnsCommand), // action
   "buffer",  // group name
   &tcA, // arguments
   &tcO, // options
   "Tweak columns",
   "Tweak columns");

