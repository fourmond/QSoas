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

#include <datastackhelper.hh>
#include <box.hh>

#include <dataseteditor.hh>
#include <statistics.hh>

#include <file-arguments.hh>
#include <metadatafile.hh>

#include <unsplicer.hh>

static Group grp("buffer", 2,
                 "Dataset",
                 "Dataset manipulations");

static Group g1("split", 3,
                "Split...",
                "Cut datasetss into smaller pieces", &grp);

static Group g2("peaks", 5,
                "Peaks...",
                "Find peaks", &grp);

static Group g0("filters", 6,
                "Filters...",
                "Filter data", &grp);



static Group g3("math", 7,
                "Mathematical operations...",
                "Mathematical operrations on datasets", &grp);

static Group g4("mbuf", 7,
                "Multi-dataset...",
                "Operations involving several datasets", &grp);

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
   "Split first");
    
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
   "Split second");

//////////////////////////////////////////////////////////////////////

static void splitMonotonicCommand(const QString &, 
                                  const CommandOptions & opts)
{
  DataStackHelper pusher(opts);

  const DataSet * ds = soas().currentDataSet();
  int group = 1;
  int keepFirst = -1;
  int keepLast = -1;
  updateFromOptions(opts, "group", group);
  updateFromOptions(opts, "keep-first", keepFirst);
  updateFromOptions(opts, "keep-last", keepLast);

  QList<DataSet *> nds = ds->splitIntoMonotonic(0, group);


  int sz = nds.size();
  for(int i = 0; i < sz; i++) {
    nds[i]->setMetaData("segment_index", i);
    
    if((keepFirst < 0 && keepLast < 0)
       || (i < keepFirst) || (sz -i <= keepLast)) {
      pusher << nds[i];
    }
    else
      delete nds[i];
  }
}
        


static ArgumentList 
smOpts(QList<Argument *>() 
           << DataStackHelper::helperOptions()
           << new IntegerArgument("group", 
                                  "Group segments",
                                  "Group that many segments into one dataset")
           << new IntegerArgument("keep-first", 
                                  "Keep only first",
                                  "Keep only the first n elements of the results")
           << new IntegerArgument("keep-last", 
                                  "Keep only last",
                                  "Keep only the last n elements of the results")
       );


static Command 
sm("split-monotonic", // command name
   effector(splitMonotonicCommand), // action
   "split",  // group name
   NULL, // arguments
   &smOpts, // options
   "Split into monotonic parts");


//////////////////////////////////////////////////////////////////////


static void splitOnValues(const QString &,
                          QStringList meta,
                          ColumnListSpecification cspec,
                          const CommandOptions & opts)
{
  DataStackHelper pusher(opts);
  const DataSet * ds = soas().currentDataSet();
  QList<int> cols = cspec.getValues(ds);
  if(meta.size() != cols.size())
    throw RuntimeError("The list of meta and columns must be matched");

  QHash<int, QString> cls;
  for(int i = 0; i < meta.size(); i++)
    cls[cols[i]] = meta[i];

  QList<DataSet*> nds = ds->autoSplit(cls);

  for(int i = 0; i < nds.size(); i++) {
    nds[i]->setMetaData("subset-index", i);
    pusher << nds[i];
  }
}

static ArgumentList 
spvArgs(QList<Argument *>() 
        << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                      "meta", 
                                      "Meta",
                                      "Names of the meta to be created")
        << new SeveralColumnsArgument("columns", 
                                      "Columns",
                                      "Columns whose values one should split on", false)
        );



static ArgumentList 
spvOpts(QList<Argument *>() 
        << DataStackHelper::helperOptions()
       );


static Command 
spv("split-on-values", // command name
    effector(splitOnValues), // action
    "split",  // group name
    &spvArgs, // arguments
    &spvOpts, // options
    "Split on column values");

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
   "Transpose");

//////////////////////////////////////////////////////////////////////

static void unwrapCommand(const QString &,
                          const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  Vector newX;

  bool useSR = false;
  bool reverse = false;
  updateFromOptions(opts, "reverse", reverse);
  double sr = 0;
  if(opts.contains("scan-rate")) {
    updateFromOptions(opts, "scan-rate", sr);
    useSR = true;
  }
  else {
    if(ds->hasMetaData("sr")) {
      sr = ds->getMetaData("sr").toDouble(&useSR);
      if(useSR)
        Terminal::out << "Using scan rate from meta-data: " << sr << endl;
    }
  }
  
  const Vector & oldX = ds->x();

  DataSet * nds;
  if(reverse) {
    int cs = 0;                 // cur segment
    double sr = 1;
    if(ds->hasMetaData("sr"))
      sr = ds->getMetaData("sr").toDouble();
    double ei = 0;
    if(ds->hasMetaData("E_init"))
      ei = ds->getMetaData("E_init").toDouble();

    double sign = 1;
    if(ds->hasMetaData("dE_init"))
      sign = ds->getMetaData("dE_init").toDouble();
    if(sign < 0)
      sign = -1;
    else
      sign = 1;

    for(int i = 0; i < oldX.size(); i++) {
      if(i) {
        double dx = oldX[i] - oldX[i-1];
        if(ds->segments.value(cs, -1) == i) {
          sign *= -1;
        }
        ei += dx * sign *sr;
      }
      newX << ei;
    }            
    nds = ds->derivedDataSet(ds->y(), "_rewrap.dat", newX);
  }
  else {
    double olddx = 0;
    OrderedList segs;
    for(int i = 0; i < oldX.size(); i++) {
      if(i) {
        double dx = oldX[i] - oldX[i-1];
        if(useSR)
          dx /= sr;

        if(dx * olddx < 0) {
          segs.insert(i);
        }
        newX << newX.last() + fabs(dx);
        olddx = dx;
      }
      else
        newX << 0;
    }
    nds = ds->derivedDataSet(ds->y(), "_unwrap.dat", newX);
    nds->setMetaData("E_init", oldX.first());
    nds->setMetaData("dE_init", oldX[1] - oldX[0]);
    if(useSR)
      nds->setMetaData("sr", sr);
    nds->segments = segs;
  }
  soas().pushDataSet(nds);
}

static ArgumentList 
  uwOpts(QList<Argument *>() 
         << new NumberArgument("scan-rate", 
                               "Scan rate",
                               "Sets the scan rate")
         << new BoolArgument("reverse", 
                             "Reverse",
                             "If true, reverses the effect of a previous unwrap command")
         );
 
 
 static Command 
   uw("unwrap", // command name
      effector(unwrapCommand), // action
   "buffer",  // group name
   NULL, // arguments
   &uwOpts, // options
   "Unwrap");

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
  DataStackHelper pusher(opts);
  const DataSet * ds = soas().currentDataSet();
  if(ds->nbColumns() <= 2) {
    Terminal::out << "No more than 2 columns in '" << ds->name 
                  << "', nothing to do" << endl;
    return;
  }
  QString pc;
  updateFromOptions(opts, "perp-meta", pc);
  int xevery = 0;
  updateFromOptions(opts, "x-every-nth", xevery);

  int group = 1;
  updateFromOptions(opts, "group-columns", group);

  if(xevery > 1 && group > 1)
    throw RuntimeError("Cannot use both /x-every-nth and /group-columns "
                       "at the same time");

  // Vector ppcd = ds->perpendicularCoordinates();
  int xvs = 0;
  int nb = 0;
  for(int i = 1; i < ds->nbColumns(); ) {
    if(xevery > 0 && ((i % xevery) == 0)) {
      xvs = i++;
      continue;
    }
    QList<int> cols;
    cols << xvs;
    QStringList colnames;
    for(int k = 0; k < group; ++k, ++i) {
      if(ds->nbColumns() > i) {
        cols << i;
        colnames << QString("%1").arg(i+1);
      }
    }
    nb += 1;
    DataSet * s = ds->derivedDataSet(QString("_col_%1.dat").
                                     arg(colnames.join("+")));
    s->selectColumns(cols);
    if(s->perpendicularCoordinates().size() > 0)
      s->setMetaData(pc, s->perpendicularCoordinates()[0]);
    pusher.pushDataSet(s);
  }
  Terminal::out << "Expanded '" << ds->name 
                << "' into " << nb << " datasets" << endl;
}

static ArgumentList 
expandOpts(QList<Argument *>() 
           << new StringArgument("perp-meta", 
                                 "Perpendicular coordinate",
                                 "defines meta-data from perpendicular coordinate")
           << new IntegerArgument("x-every-nth", 
                                  "X column every nth column",
                                  "specifies the number of columns between successive X values")
           << new IntegerArgument("group-columns", 
                                  "Group several Y columns in created datasets",
                                  "specifies the number of Y columns in the created datasets")
           << DataStackHelper::helperOptions()
           );


static Command 
expand("expand", // command name
       effector(expandCommand), // action
       "buffer",  // group name
       NULL, // arguments
       &expandOpts, // options
       "Expand",
       "Expands a multi-Y dataset");



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
          "", "a");


//////////////////////////////////////////////////////////////////////

static void chopCommand(const QString &, QList<double> values, 
                        const CommandOptions & opts)
{
  DataStackHelper pusher(opts);

  const DataSet * ds = soas().currentDataSet();
  QList<DataSet *> splitted;
  
  /// target for indices
  QList<int> * indices = NULL;
  bool setSegs = false;
  updateFromOptions(opts, "set-segments", setSegs);
  if(setSegs)
    indices = new QList<int>();

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
    pusher << nds;
    for(int i = 0; i < splitted.size(); i++)
      delete splitted[i];
    delete indices;
  }
  else {
    for(int i = splitted.size() - 1; i >= 0; i--)
      pusher << splitted[i];
  }
}

static ArgumentList 
chopA(QList<Argument *>() 
      << new SeveralNumbersArgument("lengths", 
                                    "Lengths",
                                    "Lengths of the subsets"));

static ArgumentList 
chopO(QList<Argument *>() 
      << DataStackHelper::helperOptions()
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
      "split",  // group name
      &chopA, // arguments
      &chopO, // options
      "Chop dataset");

//////////////////////////////////////////////////////////////////////

static void chopIntoSegmentsCommand(const QString &, const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  DataStackHelper pusher(opts);
  QList<DataSet *> splitted = ds->chopIntoSegments();
  for(int i = splitted.size() - 1; i >= 0; i--)
    pusher << splitted[i];
}

static ArgumentList 
scOpts(QList<Argument *>() 
    << DataStackHelper::helperOptions()
    );

static Command 
chopS("segments-chop", // command name
      effector(chopIntoSegmentsCommand), // action
      "segments",  // group name
      NULL, // arguments
      &scOpts, // options
      "Chop into segments");

//////////////////////////////////////////////////////////////////////

namespace __cu {

  typedef enum {
    PickPoint,
    PickRef,
    QuitSubtracting,
    QuitSubtractingX,
    QuitDividing,
    QuitSubtractingRespRef,
    QuitDividingRespRef,
    WriteToOutput,
    PickAndWrite,
    ShiftX,
    ScaleX,
    ShiftY,
    ScaleY,
    PlaceCross,
    RemoveCross,
    VerticalSymmetry,
    HorizontalSymmetry,
    CentralSymmetry,
    Abort,
    Quit
  } CursorActions;

  static EventHandler cursorHandler = EventHandler("cursor").
    addClick(Qt::LeftButton, PickPoint, "place cursor").
    addClick(Qt::RightButton, PickRef, "place reference").
    addKey(' ', WriteToOutput, "write to output").
    addClick(Qt::MiddleButton, PickAndWrite, "place and write to output").
    addKey('v', QuitDividing, "quit dividing by Y value").
    alsoKey('V').
    addKey('u', QuitSubtracting, "quit subtracting Y value").
    alsoKey('U').
    addKey(Qt::CTRL + 's', QuitSubtractingX, "quit subtracting X value").
    addKey(Qt::CTRL + 'u', QuitSubtractingRespRef, "subtract y - yref").
    addKey(Qt::CTRL + 'v', QuitDividingRespRef, "divide by y/yref").
    addKey(Qt::CTRL + 'x', ShiftX, "subtract x-xref from X of current and keep going").
    addKey(Qt::CTRL + Qt::SHIFT + 'x', ScaleX, "divide X of current by x/xref and keep going").
    addKey(Qt::CTRL + 'y', ShiftY, "subtract y-yref from Y of current and keep going").
    addKey(Qt::CTRL + Qt::SHIFT + 'y', ScaleY, "divide X of current by y/yref and keep going").
    addKey('a', VerticalSymmetry, "vertical symmetry around the current point").
    addKey('A', HorizontalSymmetry, "horizontal symmetry around the current point").
    addKey('c', CentralSymmetry, "central symmetry around the current point").
    alsoKey('C').
    addKey('+', PlaceCross, "place cross at the latest cursor position").
    addKey('-', RemoveCross, "remove latest cross").
    addPointPicker().
    addKey(Qt::Key_Escape, Abort, "abort").
    addKey('q', Quit, "quit").
    alsoKey('Q');



static void cursorCommand(CurveEventLoop &loop, const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  const GraphicsSettings & gs = soas().graphicsSettings();

    // std::unique_ptr<DataSet> nds = NULL;         // for in-place edition
  DataSet *  nds = NULL;         // for in-place edition
  CurveMarker m;
  CurveMarker r;
  CurveView & view = soas().view();
  PointPicker pick(&loop, ds);
  QList<CurveCross*> crosses;
  pick.trackedButtons = Qt::LeftButton|Qt::RightButton|Qt::MiddleButton;

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

  bool first = true;
  Terminal::out << "Point positions:" << endl;
  QString cur;
  ValueHash e;

  QList<DataSet *> newDatasets;

  auto ensureEditableDS = [&nds, &view, &pick, &newDatasets] () -> bool {
    DataSet * cds = const_cast<DataSet *>(pick.dataset());
    if(! cds) {
      Terminal::out << "No current dataset to work on, pick a point with exact or smooth" << endl;
      return false;
    }
    if(newDatasets.indexOf(cds) >= 0) {
      nds = cds;
    }
    else {
      nds = cds->derivedDataSet("_cu_mod.dat");
      view.addDataSet(nds);
      view.removeDataSet(cds);
      newDatasets << nds;
      pick.pickDataSet(nds);    // so we keep on working with this one.
    }
    return true;
  };
  
  while(! loop.finished()) {

    int action = cursorHandler.nextAction(loop);
    pick.processEvent(action);
    switch(action) {
    case PickPoint:
    case PickAndWrite: {
      m.p = pick.point();
      e.clear();
      int idx = pick.pointIndex();
      e << "x" << m.p.x() << "y" << m.p.y()
        << "index" <<  idx
        << "x-xr" << m.p.x() - r.p.x() 
        << "y-yr" << m.p.y() - r.p.y() 
        << "x/xr" << m.p.x()/r.p.x()
        << "y/yr" << m.p.y()/r.p.y();
      const DataSet * cds = pick.dataset();
      if(cds) {
        for(int j = 2; j < cds->nbColumns(); j++)
          e << QString("y%1").arg(j) << cds->column(j)[idx];
      }
      if(first) {
        first = false;
        Terminal::out << e.keyOrder.join("\t") << endl;
      }
      Terminal::out << e.toString() << endl;
      if(action == PickPoint)
        break;
    }
    case WriteToOutput:
      e.prepend("buffer", ds->name);
      Terminal::out << "Writing position to output file: '" 
                    << OutFile::out.fileName() << "'" << endl;
      
      OutFile::out.writeValueHash(e, ds);
      break;
    case PickRef:
      r.p = pick.point();
      Terminal::out << "Reference:\t"  << r.p.x() << "\t" 
                    << r.p.y() << endl;
      break;
    case PlaceCross:
      if(Utils::isPointFinite(m.p)) {
        CurveCross * cr = new CurveCross;
        cr->p = m.p;
        cr->pen = gs.getPen(GraphicsSettings::SeparationPen);
        crosses << cr;
        view.mainPanel()->addItem(cr);
      }
      break;
    case RemoveCross:
      if(crosses.size() > 0)
        delete crosses.takeLast();
      break;
    case Quit:
      if(newDatasets.size() > 0) {
        CommandOptions opts;
        DataStackHelper pusher(opts);

        Terminal::out << "Pushing modified datasets" << endl;
        pusher.pushDataSets(newDatasets);
      }
    case Abort:
      return;

    case QuitSubtracting: {
      Vector ny = ds->y() - m.p.y();
      Terminal::out << "Subtracting Y value: " << m.p.y() << endl;
      soas().pushDataSet(ds->derivedDataSet(ny, "_sub.dat"));
      return;
    }
    case QuitSubtractingX: {
      Vector nx = ds->x() - m.p.x();
      Terminal::out << "Subtracting X value: " << m.p.x() << endl;
      DataSet * nds = ds->derivedDataSet("_shifted.dat");
      nds->x() = nx;
      soas().pushDataSet(nds);
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
    case ShiftX: {
      double dx = m.p.x() - r.p.x();
      if(! std::isfinite(dx)) {
        Terminal::out << "Not doing anything as delta x, " << dx
                      << ", is not finite" << endl;
        break;
      }
      if(ensureEditableDS()) {
        Terminal::out << "Subtracting " << dx << " from X" << endl;
        nds->x() = nds->x() - dx;
      }
      break;
    }
    case ScaleX: {
      double xs = m.p.x()/r.p.x();
      if(! std::isfinite(xs)) {
        Terminal::out << "Not doing anything as x factor, " << xs
                      << ", is not finite" << endl;
        break;
      }
      if(ensureEditableDS()) {
        Terminal::out << "Dividing X by: " << xs << endl;
        nds->x() = nds->x()/xs;
      }
      break;
    }
    case ShiftY: {
      double dy = m.p.y() - r.p.y();
      if(! std::isfinite(dy)) {
        Terminal::out << "Not doing anything as delta y, " << dy
                      << ", is not finite" << endl;
        break;
      }
      if(ensureEditableDS()) {
        Terminal::out << "Subtracting " << dy << " from Y" << endl;
        nds->y() = nds->y() - dy;
      }
      break;
    }
    case ScaleY: {
      double ys = m.p.y()/r.p.y();
      if(! std::isfinite(ys)) {
        Terminal::out << "Not doing anything as y factor, " << ys
                      << ", is not finite" << endl;
        break;
      }
      if(ensureEditableDS()) {
        Terminal::out << "Dividing Y by: " << ys << endl;
        nds->y() = nds->y()/ys;
      }
      break;
    }
    case VerticalSymmetry: {
      if(ensureEditableDS()) {
        Terminal::out << "Vertical symmetry around X = " << m.p.x() << endl;
        nds->x() *= - 1;
        nds->x() += 2*m.p.x();
      }
      break;
    }
    case HorizontalSymmetry: {
      if(ensureEditableDS()) {
        Terminal::out << "Horizontal symmetry around Y = " << m.p.y() << endl;
        nds->y() *= - 1;
        nds->y() += 2*m.p.y();
      }
      break;
    }
    case CentralSymmetry: {
      if(ensureEditableDS()) {
        Terminal::out << "Horizontal symmetry around (" << m.p.x()
                      << "," << m.p.y() << ")" << endl;
        nds->x() *= - 1;
        nds->x() += 2*m.p.x();
        nds->y() *= - 1;
        nds->y() += 2*m.p.y();
      }
      break;
    }
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
    r.autoSwap = true;
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
     "Manually edit errors");
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
                                 "l,L: show limits\n"
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
      case 'l':
      case 'L': {
        QRectF cz = p->currentZoom();
        Terminal::out << "You can reproduce the current zoom with: \n"
                      << "limits " << cz.left() << " " << cz.right() << " "
                      << cz.top() << " " << cz.bottom() << endl;
        break;
      }
        
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
   "z");

//////////////////////////////////////////////////////////////////////

static void handleMissingDS(QList<const DataSet *> * lst)
{
  for(int i = 0; i < lst->size(); i++) {
    if(lst->value(i) == 0)
      lst->takeAt(i--);
  }
  if(lst->size() == 0)
    throw RuntimeError("No dataset provided");
}


/// @todo Maybe most of the code shared between this and divide should
/// be shared ?
/// (and some with average ?)
static void subCommand(const QString &, QList<const DataSet *> a,
                       const CommandOptions & opts)
{
  bool naive = testOption<QString>(opts, "mode", "indices");
  bool useSteps = false;
  updateFromOptions(opts, "use-segments", useSteps);

  handleMissingDS(&a);
  const DataSet * b = a.takeLast();

  for(int i = 0; i < a.size(); i++) {
    const DataSet * ds = a[i];
    Terminal::out << QObject::tr("Subtracting dataset '%1' from dataset '%2'").
      arg(b->name).arg(ds->name) 
                  << (naive ? " (index mode)" : " (xvalues mode)" ) 
                  << endl;

    soas().pushDataSet(ds->subtract(b, naive, useSteps));
  }
}

static ArgumentList 
operationArgs(QList<Argument *>() 
              << new SeveralDataSetArgument("buffers", 
                                            "Datasets",
                                            "The datasets of the operation")
              );

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
    "Subtract one dataset from others",
    "S");


//////////////////////////////////////////////////////////////////////


static void divCommand(const QString &, QList<const DataSet *> a,
                       const CommandOptions & opts)
{
  bool naive = testOption<QString>(opts, "mode", "indices");
  bool useSteps = false;
  updateFromOptions(opts, "use-segments", useSteps);

  handleMissingDS(&a);
  const DataSet * b = a.takeLast();
    
  for(int i = 0; i < a.size(); i++) {
    const DataSet * ds = a[i];
    Terminal::out << QObject::tr("Dividing dataset '%2' by dataset '%1'").
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
     "Divide");

//////////////////////////////////////////////////////////////////////

static void mopCommand(const QString &, QList<const DataSet *> a,
                       const CommandOptions & opts,
                       const QString & pref, const QString & cat,
                       DataSet * (DataSet::*ope)(const DataSet * dataset,
                                                 bool naive, 
                                                 bool useSteps) const )
{
  bool naive = testOption<QString>(opts, "mode", "indices");
  bool useSteps = false;
  handleMissingDS(&a);
  if(a.size() < 2)
    throw RuntimeError("You need to specify more than one dataset");
  
  updateFromOptions(opts, "use-segments", useSteps);

  const DataSet * op = a[0];
  DataSet * n = NULL;

  QStringList names;
  names << "'" + op->name + "'";
  for(int i = 1; i < a.size(); i++) {
    const DataSet * ds = a[i];
    names << "'" + ds->name + "'";
    DataSet * nn = (op->*ope)(ds, naive, useSteps);
    if(n)
      delete n;
    n = nn;
    op = n;
  }
  Terminal::out << pref << names.join(cat) << endl;
  soas().pushDataSet(n);
}

//////////////////////////////////////////////////////////////////////

static void addCommand(const QString &n, QList<const DataSet *> a,
                       const CommandOptions & opts)
{
  mopCommand(n, a, opts, "Adding datasets: ", " + ",
             &DataSet::add);
}

static ArgumentList 
aArgs(QList<Argument *>() 
              << new SeveralDataSetArgument("buffers", 
                                            "Datasets",
                                            "Datasets to add"));
static Command 
add("add", // command name
    effector(addCommand), // action
    "mbuf",  // group name
    &aArgs, // arguments
    &operationOpts, // options
    "Add");


//////////////////////////////////////////////////////////////////////

static void mulCommand(const QString &n, QList<const DataSet *> a,
                       const CommandOptions & opts)
{
  mopCommand(n, a, opts, "Multiplying datasets: ", " + ",
             &DataSet::multiply);
}

static Command 
mul("multiply", // command name
    effector(mulCommand), // action
    "mbuf",  // group name
    &aArgs, // arguments
    &operationOpts, // options
    "Multiply",
    "Multiply datasets", "mul");


//////////////////////////////////////////////////////////////////////


static void mergeCommand(const QString &, QList<const DataSet *> a,
                         const CommandOptions & opts)
{
  bool naive = testOption<QString>(opts, "mode", "indices");
  bool useSteps = false;
  updateFromOptions(opts, "use-segments", useSteps);

  handleMissingDS(&a);
  const DataSet * b = a.takeLast();

  for(int i = 0; i < a.size(); i++) {
    const DataSet * ds = a[i];
    Terminal::out << QObject::tr("Merging dataset '%2' with dataset '%1'").
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
       "Merge datasets based on X values");

//////////////////////////////////////////////////////////////////////


static void contractCommand(const QString &, QList<const DataSet *> a,
                            const CommandOptions & opts)
{
  bool naive = testOption<QString>(opts, "mode", "indices");
  bool useSteps = false;
  updateFromOptions(opts, "use-segments", useSteps);

  QString pc;
  updateFromOptions(opts, "perp-meta", pc);
  ColumnListSpecification useCols;
  updateFromOptions(opts, "use-columns", useCols);
  

  handleMissingDS(&a);
  if(a.size() < 2)
    throw RuntimeError("You need more than one dataset to run contract");

  auto trimColumns = [useCols](DataSet * ds) {
                       if(useCols.columns.size() > 0) {
                         QList<int> cls = useCols.getValues(ds);
                         cls.insert(0,0);
                         ds->selectColumns(cls);
                       }
                       
                     };

  QStringList names;
  names << a[0]->name;
  DataSet * cur = new DataSet(*a[0]);
  if(pc.size() > 0)
    cur->setPerpendicularCoordinates(cur->getMetaData(pc).toDouble());
  trimColumns(cur);
  
  for(int i = 1; i < a.size(); i++) {
    DataSet * ds = new DataSet(*a[i]);
    names << a[i]->name;
    if(pc.size() > 0)
      ds->setPerpendicularCoordinates(ds->getMetaData(pc).toDouble());
    trimColumns(ds);

    DataSet * n = cur->contract(ds, naive, useSteps);
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
  cur->name = Utils::smartConcatenate(names, "+", "(", ")");
  soas().pushDataSet(cur);
}

static ArgumentList 
contractArgs(QList<Argument *>() 
             << new SeveralDataSetArgument("buffers", 
                                           "Datasets",
                                           "Datasets to contract"));

static ArgumentList 
contractOpts(ArgumentList()
             << operationOpts
             << new StringArgument("perp-meta", 
                                   "Perpendicular coordinate",
                                   "defines the perpendicular coordinate from meta-data")
             << new SeveralColumnsArgument("use-columns", 
                                           "The columns to use",
                                           "if specified, uses only the given columns for the contraction"));

static Command 
contractc("contract", // command name
          effector(contractCommand), // action
          "mbuf",  // group name
          &contractArgs, // arguments
          &contractOpts, // options
          "Group datasets on X values");

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

  handleMissingDS(&all);

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

  if(data.size() == 0)
    throw RuntimeError("No datasets to make the average of");

  Terminal::out << "Averaging over " << data.size() <<  " datasets" << endl;

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
                                            "Datasets",
                                            "Datasets to average"));

static ArgumentList 
aveOpts(ArgumentList()
        << operationOpts
        << new BoolArgument("split", 
                            "Split into monotonic parts",
                            "If on, the datasets are automatically "
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
    "Average");

//////////////////////////////////////////////////////////////////////



static void catCommand(const QString &, QList<const DataSet *> b, const CommandOptions & opts)
{
  bool setSegs = true;
  updateFromOptions(opts, "add-segments", setSegs);
  handleMissingDS(&b);

  if(b.size() > 0)
    soas().pushDataSet(DataSet::concatenateDataSets(b, setSegs));
  else
    throw RuntimeError("No dataets to concatenate");
}

static ArgumentList 
catArgs(QList<Argument *>() 
        << new SeveralDataSetArgument("buffers",
                                      "Datasets",
                                      "Datasets to concatenate"));


static ArgumentList 
catOpts(QList<Argument *>()
        << new BoolArgument("add-segments", 
                            "Add segments",
                            "If on (default) segments are added between "
                            "the old datasets"));

static Command 
cat("cat", // command name
    effector(catCommand), // action
    "mbuf",  // group name
    &catArgs, // arguments
    &catOpts, // options
    "Concatenate",
    "Concatenate the given buffers",
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
       "Shift X values so that x[0] = 0");

//////////////////////////////////////////////////////////////////////

static QStringList allStatsNames()
{
  const DataSet * ds = soas().currentDataSet(true);
  if(!ds)
    return QStringList();
  return StatisticsValue::statsAvailable(ds);
}

static void statsOn(const DataSet * ds, const CommandOptions & opts,
                    const QStringList & sns)
{
  Statistics stats(ds);

  ValueHash os;
  QList<ValueHash> byCols = stats.statsByColumns(&os);

  if(sns.isEmpty()) {
    Terminal::out << "Statistics on dataset: " << ds->name << ":";
    for(int i = 0; i < ds->nbColumns(); i++)
      Terminal::out << "\n" << byCols[i].prettyPrint();
  }
  else {
    os = os.select(sns);
    Terminal::out << ds->name << os.toString(QString("\t")) << endl;
  }

  Terminal::out << endl;
  /// @todo We should be able to use handleOutput as well for the
  /// display to the terminal ?
  os.handleOutput(ds, opts);
}

static void statsCommand(const QString &, const CommandOptions & opts)
{
  QList<const DataSet *> datasets;
  updateFromOptions(opts, "buffers", datasets);
  if(datasets.isEmpty())
    datasets <<  soas().currentDataSet();

  bool bySegments = false;
  updateFromOptions(opts, "use-segments", bySegments);
  QStringList statsNames;
  updateFromOptions(opts, "stats", statsNames);


  for(const DataSet * ds : datasets) {
    if(bySegments) {
      QList<DataSet * > segs = ds->chopIntoSegments();
      for(int i = 0; i < segs.size(); i++) {
        statsOn(segs[i], opts, statsNames);
        delete segs[i];
      }
    }
    else
      statsOn(ds, opts, statsNames);
  }
}

/// @todo Validation will be a pain when buffer is specified
/// and it has more columns than the current one
class StatsArgument : public SeveralChoicesArgument {
public:
  StatsArgument(const char * cn, const char * pn,
                const char * d = "", bool g = true, 
                bool def = false) : 
    SeveralChoicesArgument(::allStatsNames, ',',
                           cn, pn, d, g, def){
  }; 

  virtual QString typeName() const override {
    return "stats-names";
  };
  virtual QString typeDescription() const override {
    return "One or more name of statistics (as displayed by stats), separated by `,`.";
  };

};

static ArgumentList 
statsO(QList<Argument *>() 
       << new SeveralDataSetArgument("buffers", 
                                     "Datasets",
                                     "datasets to work on", true, true)
       << new StatsArgument("stats",
                            "Select stats",
                            "writes only the given stats")
       << new BoolArgument("use-segments", 
                           "Use segments",
                           "makes statistics segment by segment (defaults to false)")
       << ValueHash::outputOptions()
       );


static Command 
stats("stats", // command name
      effector(statsCommand), // action
      "buffer",  // group name
      NULL, // arguments
      &statsO, // options
      "Statistics",
      "Statistics");

//////////////////////////////////////////////////////////////////////


static void generateDSCommand(const QString &, double beg, double end, 
                              const CommandOptions & opts)
{
  DataStackHelper pusher(opts);

  int samples = 1000;
  updateFromOptions(opts, "samples", samples);

  QStringList formulas;
  updateFromOptions(opts, "formula", formulas);

  int nb = 1;
  updateFromOptions(opts, "number", nb);

  int columns = 2;
  updateFromOptions(opts, "columns", columns);

  if(formulas.size() > 0 && opts.contains("columns")) {
    Terminal::out << "The /columns option is ignored when there "
                  << "are one or more formulas specified" << endl;
  }

  QString name;
  updateFromOptions(opts, "name", name);

  bool logSpace = false;
  updateFromOptions(opts, "log", logSpace);

  for(int k = 0; k < nb; k++) {
    Vector x = logSpace ? Vector::logarithmicallySpaced(beg, end, samples)
      : Vector::uniformlySpaced(beg, end, samples);
    QList<Vector> cols;
    cols << x;
    Vector y = x;
    if(formulas.size() > 0) {
      QStringList vars;
      vars << "x" << "i" << "number";
      for(const QString & formula : formulas) {
        Expression expr(formula, vars);
        /// @todo have a global way to incorporate all "constants"
        /// (temperature and fara) into that.
        double v[3];
        for(int i = 0; i < x.size(); i++) {
          v[0] = x[i];
          v[1] = i;
          v[2] = k;
          y[i] = expr.evaluate(v);
        }
        cols << y;
      }
    }
    else {
      while(cols.size() < columns)
        cols << y;
    }

    DataSet * newDs = new DataSet(cols);
    if(nb > 1) {
      if(name.isEmpty())
        name = "generated_%d.dat";
      newDs->name = QString::asprintf(name.toUtf8(), k);
      newDs->setMetaData("generated-number", k);
    }
    else
      newDs->name = name.isEmpty() ? "generated.dat" : name;
    pusher.pushDataSet(newDs);
  }
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
                            "number of data points")
     << new IntegerArgument("number",
                            "Number of generated datasets",
                            "generates that many datasets")
     << new BoolArgument("log",
                         "Log",
                         "uses logarithmically spaced X values")
     << new IntegerArgument("columns",
                            "Number of columns",
                            "number of columns of the generated datasets")
     << new SeveralStringsArgument("formula",
                                   "The Y values",
                                   "Formula to generate the Y values",
                                   true, true)
     << new StringArgument("name",
                           "The new buffer name",
                           "The name of the newly generated bufffers (may include a %d specification for the number)")
     << DataStackHelper::helperOptions()
     );


static Command 
gDS("generate-dataset", // command name
    effector(generateDSCommand), // action
    "math",  // group name
    &gDSA, // arguments
    &gDSO, // options
    "Generate dataset", "",
    "generate-buffer");

//////////////////////////////////////////////////////////////////////

static void recordMeta(const QString & file, const QString & meta,
                       const QVariant & value)
{
  MetaDataFile md(file);
  md.read(false);
  md.metaData[meta] = value;
  md.write();
}

static void recordMetaCommand(const QString &, QString meta, QString value,
                              QStringList files,
                              const CommandOptions & opts)
{
  // Attempt to convert to double
  QVariant val = ValueHash::variantFromText(value, opts);

  QStringList exclude;
  updateFromOptions(opts, "exclude", exclude);

  for(const QString f : files) {
    if(MetaDataFile::isMetaDataFile(f)) {
      Terminal::out << "Skipping '" << f
                    << "', which is a meta-data file" << endl;
      continue;
    }
    if(exclude.contains(f)) {
      Terminal::out << "Skipping '" << f
                    << "', excluded" << endl;
      continue;
    }
    try {
      Terminal::out << "Setting meta-data for file '" << f
                    << "'" << endl;
      ::recordMeta(f, meta, val);
    }
    catch(const RuntimeError & re) {
      Terminal::out << "Error with file '" << f << "': "
                    << re.message() << endl;
    }
  }
}

static ArgumentList 
rMA(QList<Argument *>() 
    << new StringArgument("name", 
                          "Name",
                          "name of the meta-data")
    << new StringArgument("value", 
                          "Value",
                          "value of the meta-data")
    << new SeveralFilesArgument("files", 
                                "Files",
                                "files on which to set the meta-data",
                                true)
    );

static ArgumentList 
rMO(QList<Argument *>()
    << ValueHash::variantConversionOptions()
    << new SeveralFilesArgument("exclude", 
                                "Exclude",
                                "exclude files")
            
     );


static Command 
rM("record-meta", // command name
   effector(recordMetaCommand), // action
   "buffer",  // group name
   &rMA, // arguments
   &rMO, // options
   "Set meta-data",
   "Manually set meta-data");

//////////////////////////////////////////////////////////////////////

// Takes a dataset, and uses column names/row names to set meta to a
// series of files.
//
// Takes all columns and files
static void exportMetaCommand(const QString &, 
                              const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  if(ds->rowNames.size() < 1)
    throw RuntimeError("Current dataset has no row names");
  QStringList cns = ds->mainColumnNames();
  for(int i = 0; i < ds->nbRows(); i++) {
    // First find matching files:
    QString rn = ds->rowNames.first().value(i, "");
    if(rn.isEmpty())
      continue;
    // For now, exact match
    Terminal::out << "Setting meta-data for file: " << rn << "\n -> ";
    for(int j = 0; j < ds->nbColumns(); j++) {
      double v = ds->column(j)[i];
      Terminal::out << cns[j] << " = " << v << ", ";
      recordMeta(rn, cns[j], v);
    }
    Terminal::out << endl;
  }
}


/*static ArgumentList 
eMO(QList<Argument *>()
    << new SeveralFilesArgument("exclude", 
                                "Exclude",
                                "exclude files")
            
     );
*/


static Command 
eM("export-meta", // command name
   effector(exportMetaCommand), // action
   "buffer",  // group name
   NULL, // arguments
   NULL, // options
   "Export meta-data from dataset");

//////////////////////////////////////////////////////////////////////


static void setMetaCommand(const QString &, QString meta, QString value, 
                           const CommandOptions & opts)
{
  bool alsoRecord = false;
  updateFromOptions(opts, "also-record", alsoRecord);
  DataSet * ds = soas().currentDataSet();
  QVariant val = ValueHash::variantFromText(value, opts);
  ds->setMetaData(meta, val);
  if(alsoRecord) {
    QString f = ds->getMetaData("original_file").toString();
    if(f.isEmpty()) {
      Terminal::out << "Cannot set meta-data as there is no source "
                    << "file for the data" << endl;

    }
    else {
      Terminal::out << "Also setting the meta-data for original file '" << f
                    << "'" << endl;
      ::recordMeta(f, meta, val);
    }
  }

}

static ArgumentList 
sMA(QList<Argument *>() 
    << new StringArgument("name", 
                          "Name",
                          "name of the meta-data")
    << new StringArgument("value", 
                          "Value",
                          "value of the meta-data")
   );

static ArgumentList 
sMO(QList<Argument *>()
    << ValueHash::variantConversionOptions()
    << new BoolArgument("also-record", 
                        "Also record",
                        "also record the meta-data as if one had used record-meta on the original file")
    );


static Command 
sM("set-meta", // command name
   effector(setMetaCommand), // action
   "buffer",  // group name
   &sMA, // arguments
   &sMO, // options
   "Set meta-data",
   "Manually set meta-data");

//////////////////////////////////////////////////////////////////////


static void setPerpCommand(const QString &, 
                           const CommandOptions & opts)
{
  DataSet * ds = soas().currentDataSet();
  QList<double> coords;
  int row = -1;
  updateFromOptions(opts, "coords", coords);
  updateFromOptions(opts, "from-row", row);
  if(row >= 0) {
    // Mmmm, I really don't like the idea of modifying in place...
    Vector perp;
    for(int i = 0; i < ds->nbColumns(); i++) {
      double v = ds->column(i)[row];
      if(i > 0)
        perp << v;
    }
    ds->setPerpendicularCoordinates(perp);
    ds->removeRow(row);
  }
  else
    ds->setPerpendicularCoordinates(coords.toVector());
}

static ArgumentList 
sPO(QList<Argument *>() 
    << new SeveralNumbersArgument("coords", 
                                  "Coordinates",
                                  "The values of the coordinates (one for each Y column)", true, true)
    << new IntegerArgument("from-row", 
                           "From row",
                           "Sets the values from the given row (and delete it)")
   );

// static ArgumentList 
// sMO(QList<Argument *>() 
//     );


static Command 
sP("set-perp", // command name
   effector(setPerpCommand), // action
   "buffer",  // group name
   NULL, // arguments
   &sPO, // options
   "Set perpendicular",
   "Set perpendicular coordinates");

//////////////////////////////////////////////////////////////////////


static void editCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  // this seems necessary to work around what appears to be a Qt bug,
  // a spurious crash on closing edit
  static DatasetEditor * editor = NULL;
  if(editor)
    delete editor;
  editor = new DatasetEditor(ds);
  editor->exec();
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

  QList<int> cols;
  for(int i = 0; i < ds->nbColumns(); i++)
    cols << i;

  /// @todo Swap columns

  bool flip = false;
  bool flipAll = false;
  ColumnListSpecification toRemove;
  ColumnListSpecification toSelect;
  
  updateFromOptions(opts, "flip", flip);
  updateFromOptions(opts, "flip-all", flipAll);
  updateFromOptions(opts, "remove", toRemove);
  updateFromOptions(opts, "select", toSelect);
  if(toSelect.columns.size() > 0) {
    cols = toSelect.getValues(ds);
  }
  else if(toRemove.columns.size() > 0) {
    QList<int> tr = toRemove.getValues(ds);
    qSort(tr);
    for(int i = tr.size() - 1; i >= 0; i--)
      cols.removeAt(tr[i]);
  }
  else if(flipAll) {
    Utils::reverseList(cols);
  }
  else if(flip) {
    cols.takeFirst();
    Utils::reverseList(cols);
    cols.insert(0, 0);
  }

  DataSet * nds = ds->derivedDataSet("_tweaked.dat");
  nds->selectColumns(cols);
  soas().pushDataSet(nds);
}

static ArgumentList 
tcA;

static ArgumentList 
tcO(QList<Argument *>() 
    << new SeveralColumnsArgument("remove", 
                                  "Columns to remove",
                                  "the columns to remove ", true)
    << new SeveralColumnsArgument("select", 
                                  "Select the columns",
                                  "select the columns to keep", true)
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


//////////////////////////////////////////////////////////////////////

static void correlationMatrixCommand(const QString &, 
                                     const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  QList<Vector> cols = ds->allColumns();

  int sz = cols.size();

  QList<Vector> mat;
  for(int i = 0; i < sz; i++)
    mat << Vector(sz, 0);
  for(int i = 0; i < sz; i++) {
    // for now we keep i = j, but it's 1 -- unless we use that to
    // indicate the variance ?
    for(int j = i; j < sz; j++) {
      double v = Vector::correlation(cols[i], cols[j]);
      mat[i][j] = v;
      mat[j][i] = v;
    }
  }


  DataSet * nds = ds->derivedDataSet(mat, "_cm.dat");
  soas().pushDataSet(nds);
}

static ArgumentList 
cmA;

static ArgumentList 
cmO;


static Command 
cm("correlation-matrix", // command name
   effector(correlationMatrixCommand), // action
   "buffer",  // group name
   &cmA, // arguments
   &cmO, // options
   "Correlation matrix",
   "Correlation matrix between the various columns");

//////////////////////////////////////////////////////////////////////
// Series of commands to change the names of rows or columns

static void setColumnNamesCommand(const QString &,
                                  const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  DataSet * nds = ds->derivedDataSet(".dat");

  QStringList names;
  updateFromOptions(opts, "names", names);


  bool sanitize = false;
  updateFromOptions(opts, "sanitize-names", sanitize);

  bool clear = false;
  updateFromOptions(opts, "clear", clear);

  QList<int> cols;
  ColumnListSpecification spec;
  updateFromOptions(opts, "columns", spec);
  cols = spec.getValues(ds);
  if(cols.size() > 0) {
    if(cols.size() != names.size())
      throw RuntimeError("Mismatch between number of names (%1) "
                         "and number of columns (%2)").
        arg(names.size()).arg(cols.size());
  }
  else {
    for(int i = 0; i < names.size(); i++)
      cols << i;
  }

  if(clear)
    nds->columnNames.clear();

  for(int i = 0; i < names.size(); i++)
    nds->setColumnName(cols[i], names[i]);

  if(sanitize) {
    names = nds->mainColumnNames();
    QSet<QString> usedNames;
    QRegExp valid("^[_a-z]\\w*$");
    for(int i = 0; i < names.size(); i++) {
      QString n = names[i];
      if(valid.indexIn(n) != 0) {
        QString n2 = n;
        if(n2[0].isUpper())
          n2[0] = n2[0].toLower();
        if(n2[0].isDigit())
          n2 = "_" + n2;
        n2.replace(QRegExp("[^A-Za-z_0-9]"), "_");
        QString n3 = n2;
        int idx = 0;
        while(usedNames.contains(n3)) {
          idx += 1;
          n3 = QString("%1_%2").arg(n2).arg(idx);
        }
        Terminal::out << "Replaced '" << n << "' (corresponding to "
                      << DataSet::standardNameForColumn(i) << ") by '"
                      << n3 << "'" << endl;
        nds->setColumnName(i, n3);
        usedNames.insert(n3);
      }
    }
  }
  
  
  soas().pushDataSet(nds);
}


static ArgumentList 
scO(QList<Argument *>() 
    << new SeveralStringsArgument("names", 
                                  "Names",
                                  "Names of the columns", true, true)
    << new SeveralColumnsArgument("columns", 
                                  "Columns",
                                  "Sets the names of these columns only")
    << new BoolArgument("sanitize-names", 
                        "Sanitize names",
                        "Adapts the names so that they can be used with apply-formula /use-names=true")
    << new BoolArgument("clear", 
                        "Clear names",
                        "Removes all the names")
    );

static Command 
scn("set-column-names", // command name
    effector(setColumnNamesCommand), // action
    "buffer",  // group name
    NULL, // arguments
    &scO, // options
    "Set column names",
    "Sets the column names of the buffer");


static QHash<int, QString> parseNameSpecs(const QStringList & names)
{
  QHash<int, QString> spcs;
  if(names.size() == 0)
    return spcs;                // or throw ?
  QRegExp cn("^#(\\d+):(.*)");
  if(cn.indexIn(names.first()) >= 0) {
    // Advanced parsing
    for(const QString & n : names) {
      if(! cn.indexIn(n) == 0)
        throw RuntimeError("Invalid column/row name: '%1'").arg(n);
      spcs[cn.cap(1).toInt()] = cn.cap(2);
    }
  }
  else {
    for(int i = 0; i < names.size(); i++)
      spcs[i] = names[i];
  }
  return spcs;
}

// Two modes: sets all the names, or if the strings start with #\d:,
// then set only those
static void setRowNamesCommand(const QString &,
                               const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  DataSet * nds = ds->derivedDataSet(".dat");

  QStringList names;
  updateFromOptions(opts, "names", names);

  bool clear = false;
  updateFromOptions(opts, "clear", clear);
  if(clear)
    nds->rowNames.clear();

  // Parses the spec:
  QHash<int, QString> sp = ::parseNameSpecs(names);

  for(int row : sp.keys())
    nds->setRowName(row, sp[row]);
  
  soas().pushDataSet(nds);
}

static ArgumentList 
srO(QList<Argument *>() 
    << new SeveralStringsArgument("names", 
                                  "Names",
                                  "Names of the columns", true, true)
    << new BoolArgument("clear", 
                        "Clear names",
                        "Removes all the names")
    );

static Command 
srn("set-row-names", // command name
    effector(setRowNamesCommand), // action
    "buffer",  // group name
    NULL, // arguments
    &srO, // options
    "Set row names",
    "Sets the row names of the buffer");





//////////////////////////////////////////////////////////////////////


// testing...
// QSoas> generate-buffer 0 10 x**2+10*sin(Pi*0.5*i)
// QSoas> unsplice
// Found 25 trends
// QSoas> generate-buffer 0 10 10+x**2+10*sin(Pi*0.5*i)
// QSoas> unsplice
// Found 14 trends
// QSoas> generate-buffer 0 30 10+x**2+10*sin(Pi*0.5*i)
// QSoas> generate-buffer 0 10 30+x**2+10*sin(Pi*0.5*i)
// QSoas> unsplice

static void unspliceCommand(const QString &, const CommandOptions & opts)
{
  DataStackHelper pusher(opts);
  const DataSet * ds = soas().currentDataSet();
  Unsplicer us(ds->x(), ds->y());
  us.unsplice();
  QList<UnsplicedData> trends = us.trends();
  QList<Vector> leftovers = us.leftovers();
  

  Terminal::out << "Found " << trends.size() << " trends, and "
                << leftovers.first().size() << " leftover points" << endl;
  int i = 0;
  for(const UnsplicedData & t : trends) {
    QList<Vector> cols;
    cols << t.xv << t.yv;
    DataSet * nds = ds->derivedDataSet(cols, QString("_trend_%1").arg(i));
    pusher << nds;
    ++i;
  }
  if(leftovers.first().size() > 0) {
    DataSet * nds = ds->derivedDataSet(leftovers, "_leftover");
    pusher << nds;
  }
  
}

static ArgumentList 
usOpts(QList<Argument *>() 
           << DataStackHelper::helperOptions()
       );


// I don't really think this command is so useful in the end...

// static Command 
// unsplice("unsplice", // command name
//          effector(unspliceCommand), // action
//          "buffer",  // group name
//          NULL, // arguments
//          &usOpts, // options
//          "Unsplice dataset",
//          "Tries to identify multiple trends in the data");

//////////////////////////////////////////////////////////////////////

static void classifyCommand(const QString &, const CommandOptions & opts)
{
  DataStackHelper pusher(opts);
  const DataSet * ds = soas().currentDataSet();
  double mag = 1.5;

  QList<QList<Vector> > mags =
    Vector::orderOfMagnitudeClassify(ds->x(), ds->y(), mag);
  

  Terminal::out << "Found " << mags.size() << " trends" << endl;
  int i = 0;
  for(const QList<Vector> & t : mags) {
    DataSet * nds = ds->derivedDataSet(t, QString("_trend_%1").arg(i));
    pusher << nds;
    ++i;
  }
}


// static Command 
// classify("classify", // command name
//          effector(classifyCommand), // action
//          "buffer",  // group name
//          NULL, // arguments
//          &usOpts, // options
//          "Classify dataset",
//          "Tries to identify multiple order of magnitude trends in the data");


//////////////////////////////////////////////////////////////////////

// Random generators
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>


#include <argument-templates.hh>

typedef double (*generator)(const gsl_rng *, double);

static double uniform(const gsl_rng * r, double sigma)
{
  return (0.5 - gsl_rng_uniform(r)) * sigma/(sqrt(3));
}

QHash<QString, generator> generators = {
  {"uniform", ::uniform},
  {"gaussian", ::gsl_ran_gaussian},
  {"cauchy", ::gsl_ran_cauchy}
};

static void addNoiseCommand(const QString &, double sigma,
                            const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  gsl_rng * r = gsl_rng_alloc(gsl_rng_default);
  generator gen = ::uniform;
  updateFromOptions(opts, "distribution", gen);

  int sd = -1;
  updateFromOptions(opts, "seed", sd);
  unsigned long seed;
  if(sd >= 0)
    seed = sd;
  else
    seed = QDateTime::currentDateTime().toSecsSinceEpoch();
  gsl_rng_set(r, seed);

  /// @todo Dump info as in the example in
  /// https://www.gnu.org/software/gsl/doc/html/rng.html ?

  // Add noise only on Y for now.
  Vector ny = ds->y();
  for(int i = 0; i < ny.size(); i++)
    ny[i] += gen(r, sigma);

  soas().pushDataSet(ds->derivedDataSet(ny, "_noise.dat"));

  gsl_rng_free(r);
}


static ArgumentList 
anA(QList<Argument *>() 
    << new NumberArgument("sigma", 
                          "Amplitude",
                          "'Amplitude' of the noise")
    );

static ArgumentList 
anO(QList<Argument *>() 
    << new IntegerArgument("seed", 
                           "Generator seed",
                           "The generator seed. If not specified or negative, uses the current time")
    << new TemplateChoiceArgument<generator>(generators,
                                             "distribution", 
                                             "Distribution",
                                             "The noise distribution")
    );

static Command 
an("add-noise", // command name
   effector(addNoiseCommand), // action
   "buffer",  // group name
   &anA, // arguments
   &anO, // options
   "Add noise",
   "Adds noise to the buffer");


