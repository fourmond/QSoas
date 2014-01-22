/**
   \file dataset-commands.cc commands for tweaking datasets
   Copyright 2011, 2012, 2013 by Vincent Fourmond

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

#include <dataseteditor.hh>
#include <statistics.hh>


static Group grp("buffer", 2,
                 "Buffer",
                 "Buffer manipulations");

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
   "buffer",  // group name
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
   "buffer",  // group name
   NULL, // arguments
   NULL, // options
   "Split second",
   "Gets buffer after first dx sign change",
   "Returns the part of the buffer after "
   "the first change of sign of dx");

//////////////////////////////////////////////////////////////////////

static void splitMonotonicCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  QList<DataSet *> nds = ds->splitIntoMonotonic();
  for(int i = 0; i < nds.size(); i++)
    soas().pushDataSet(nds[i]);
}
        

static Command 
sm("split-monotonic", // command name
   optionLessEffector(splitMonotonicCommand), // action
   "buffer",  // group name
   NULL, // arguments
   NULL, // options
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

static void expandCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  if(ds->nbColumns() <= 2) {
    Terminal::out << "No more than 2 columns in '" << ds->name 
                  << "', nothing to do" << endl;
    return;
  }
  for(int i = 1; i < ds->nbColumns(); i++) {
    QList<Vector> cols;
    cols << ds->x();
    cols << ds->column(i);
    DataSet * s = ds->derivedDataSet(cols, QString("_col_%1.dat").arg(i+1));
    soas().stack().pushDataSet(s, true);
    if(i > 1)
      soas().view().addDataSet(s);
    else
      soas().view().showDataSet(s);
  }
}


static Command 
expand("expand", // command name
     optionLessEffector(expandCommand), // action
     "buffer",  // group name
     NULL, // arguments
     NULL, // options
     "Expand",
     "??",
     "??");

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
    delete indices;
  }
  else
    for(int i = splitted.size() - 1; i >= 0; i--)
      soas().pushDataSet(splitted[i]);
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
      << new BoolArgument("set-segments", 
                          "Set segments",
                          "Whether to actually cut the dataset, or just "
                          "to set segments where the cuts would have "
                          "been")
      );

static Command 
chopC("chop", // command name
      effector(chopCommand), // action
      "buffer",  // group name
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
      "buffer",  // group name
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
  while(! loop.finished()) {

    int action = cursorHandler.nextAction(loop);
    pick.processEvent(action);
    switch(action) {
    case PickPoint:
        m.p = pick.point();
        cur = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7").
          arg(m.p.x()).arg(m.p.y()).arg(pick.pointIndex()).
          arg(m.p.x() - r.p.x()).arg(m.p.y() - r.p.y()).
          arg(m.p.x()/r.p.x()).arg(m.p.y()/r.p.y());
        Terminal::out << cur << endl;
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
      soas().pushDataSet(ds->derivedDataSet(ny, "_sub.dat"));
      return;
    }
    case WriteToOutput:
      OutFile::out.setHeader("# Point positions:\n"
                             "## buffer\tX\tY\t\tidx\tx-xr\ty-yr\tx/xr\ty/yr");
      Terminal::out << "Writing position to output file: '" 
                    << OutFile::out.fileName() << "'" << endl;
      
      OutFile::out << ds->name << "\t" << cur << "\n" << flush;
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

namespace Cut {
  typedef enum {
    LeftPick,
    RightPick,
    SelectInside,
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
    view.addItem(&r);
    view.addItem(&d);

    d.pen = gs.getPen(GraphicsSettings::ResultPen);

    view.mainPanel()->xLabel = "Index";

    loop.setHelpString("Cut:\n" + cutHandler.buildHelpString());

    d.countBB = true;
    d.yvalues = ds->y();
    d.xvalues = d.yvalues;
    for(int i = 0; i < d.xvalues.size(); i++)
      d.xvalues[i] = i;
    r.xleft = 0;
    r.xright = d.xvalues.size()-1;

    while(! loop.finished()) {
      switch(cutHandler.nextAction(loop)) {
      case SelectInside: 
        soas().pushDataSet(ds->subset(r.xleft, r.xright, true));
        return;
      case LeftPick:
      case RightPick:
        r.setX(round(loop.position().x()), loop.button());
        break;
      case Abort:
        if(ds != origDs)
          delete ds;
        view.addDataSet(origDs);  // To turn its display back on
        return;
      case SelectOutside:
        soas().pushDataSet(ds->subset(r.xleft, r.xright, false));
        return;
      case RemoveInside: {
        DataSet * nds = ds->subset(r.xleft, r.xright, false);
        nds->name = origDs->cleanedName() + "_perf.dat";
        if(ds != origDs)
          delete ds;
        ds = nds;
        d.yvalues = ds->y();
        d.xvalues = d.yvalues;
        for(int i = 0; i < d.xvalues.size(); i++)
          d.xvalues[i] = i;
        r.xleft = 0;
        r.xright = d.xvalues.size()-1;
      }
      default:
        ;
      }
    }
  }

  static Command 
  cut("cut", // command name
      optionLessEffector(cutCommand), // action
      "buffer",  // group name
      NULL, // arguments
      NULL, // options
      "Cut",
      "Cuts the current curve",
      "Cuts bits from the current curve",
      "c");
};

//////////////////////////////////////////////////////////////////////


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
    "buffer",  // group name
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
     "buffer",  // group name
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
       "buffer",  // group name
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
  
  DataSet * cur = new DataSet(*a[0]);
  if(pc.size() > 0)
    cur->setPerpendicularCoordinates(cur->getMetaData(pc).toDouble());
  
  if(a.size() < 2)
    throw RuntimeError("You need more than one buffer to run contract");
  for(int i = 1; i < a.size(); i++) {
    DataSet * ds = new DataSet(*a[i]);
    if(pc.size() > 0)
      ds->setPerpendicularCoordinates(ds->getMetaData(pc).toDouble());

    DataSet * n = cur->contract(ds, naive, useSteps);
    delete cur;
    delete ds;
    cur = n;
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
                                   "Define the perpendicular coordinate from meta-data"));

static Command 
contractc("contract", // command name
          effector(contractCommand), // action
          "buffer",  // group name
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
                            "split into monotonic parts before averaging."));


static Command 
ave("average", // command name
    effector(avgCommand), // action
    "buffer",  // group name
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
    "buffer",  // group name
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
       "buffer",  // group name
       NULL, // arguments
       NULL, // options
       "Shift X values",
       "Shift X values so that x[0] = 0",
       "Shift X values so that x[0] = 0");

//////////////////////////////////////////////////////////////////////


static void statsOn(const DataSet * ds, bool output)
{
  Statistics stats(ds);

  ValueHash os;
  QList<ValueHash> byCols = stats.statsByColumns(&os);

  Terminal::out << "Statistics on buffer: " << ds->name << ":";
  for(int i = 0; i < ds->nbColumns(); i++)
    Terminal::out << "\n" << byCols[i].prettyPrint();

  Terminal::out << endl;
  if(output) {
    Terminal::out << "Writing stats to output file" << endl;
    OutFile::out.writeValueHash(os);
  }
}

static void statsCommand(const QString &, const CommandOptions & opts)
{
  DataSet * ds = soas().currentDataSet();
  bool output = false;
  updateFromOptions(opts, "buffer", ds);
  updateFromOptions(opts, "to-file", output);
  bool bySegments = false;
  updateFromOptions(opts, "use-segments", bySegments);

  if(bySegments) {
    QList<DataSet * > segs = ds->chopIntoSegments();
    for(int i = 0; i < segs.size(); i++) {
      statsOn(segs[i], output);
      delete segs[i];
    }
  }
  else
    statsOn(ds, output);
}

static ArgumentList 
statsO(QList<Argument *>() 
       << new DataSetArgument("buffer", 
                              "Buffer",
                              "An alternative buffer to get information on",
                              true)
       << new BoolArgument("to-file", 
                           "To file",
                           "Also write stats to output file")
       << new BoolArgument("use-segments", 
                           "Use segments",
                           "Make statistics segments by segment")
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
    "buffer",  // group name
    &gDSA, // arguments
    &gDSO, // options
    "Generate buffer",
    "Generate a ramp",
    "...");

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
      cols.removeAt(toRemove[i] - 1);
  }

  DataSet * nds = ds->derivedDataSet(cols, "_tweaked.dat");
  soas().pushDataSet(nds);
}

static ArgumentList 
tcA;

static ArgumentList 
tcO(QList<Argument *>() 
    /// @todo a 1..4 syntax for 
    << new SeveralIntegersArgument("remove", 
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

