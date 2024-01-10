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

#include <spline.hh>
#include <bsplines.hh>
#include <pointpicker.hh>
#include <pointtracker.hh>

#include <eventhandler.hh>

#include <graphicssettings.hh>

#include <fft.hh>

#include <idioms.hh>
#include <curve-effectors.hh>

#include <msolver.hh>

#include <datastackhelper.hh>
#include <datasetlist.hh>


//////////////////////////////////////////////////////////////////////

namespace __reg {
  typedef enum {
    LeftPick,
    RightPick,
    Subtract,
    Divide,
    Write,
    Exponential,
    ShowExponential,
    Peak,
    Quit
  } ReglinActions;

  static EventHandler reglinHandler = EventHandler("reglin").
    addClick(Qt::LeftButton, LeftPick, "pick left bound").
    addClick(Qt::RightButton, RightPick, "pick right bound").
    addKey('q', Quit, "quit").
    alsoKey('Q').
    addKey(Qt::Key_Escape, Quit).
    addKey('u', Subtract, "subtract trend").
    alsoKey('U').
    addKey(' ', Write, "write to output file").
    addKey('e', Exponential, "divide by exponential").
    alsoKey('E').
    addKey('x', ShowExponential, "show exponential").
    addKey('p', Peak, "detect peak").
    alsoKey('P').
    addPointPicker(true).
    addKey('v', Divide, "divide by trend").
    alsoKey('V');

  static void reglinCommand(CurveEventLoop &loop, const QString &)
  {
    const DataSet * bds = soas().currentDataSet();
    const DataSet * ds = bds;
    const GraphicsSettings & gs = soas().graphicsSettings();

    CurveLine line;
    CurveHorizontalRegion r;
    CurveView & view = soas().view();
    CurvePanel bottom;
    CurveData d;
    CurveData d_exp;
    bottom.drawingXTicks = false;
    bottom.stretch = 30;        // 3/10ths of the main panel.
    view.addItem(&line);
    view.addItem(&r);
    view.addItem(&d_exp);
    bottom.addItem(&d);

    bottom.yLabel = Utils::deltaStr("Y");
    d.countBB = true;


    view.addPanel(&bottom);
    line.pen = gs.getPen(GraphicsSettings::ReglinPen);
    d.pen = gs.getPen(GraphicsSettings::ResultPen);
    d_exp.hidden = true;
    d_exp.pen = gs.getPen(GraphicsSettings::BaselinePen);
    d_exp.pen.setColor("#555");
      
    QPair<double, double> reg;
    double xleft = ds->x().min();
    double xright = ds->x().max();

    // Computed fields:
    double decay_rate = 0;


    // the lines used for peak determination
    CurveLine fp, hp, fpl, hpl;
    fp.hidden = true;

    hp.hidden = true;
    fpl.hidden = true;
    hpl.hidden = true;

    fp.pen = hp.pen = fpl.pen = hpl.pen =
      gs.getPen(GraphicsSettings::ResultPen);

    view.addItem(&fp);
    view.addItem(&hp);
    view.addItem(&fpl);
    view.addItem(&hpl);

    
    PointPicker pick(&loop, ds);
    pick.resetMethod();


    loop.setHelpString(QString("Linear regression: \n")
                       + reglinHandler.buildHelpString());

    Terminal::out << "Linear regression:"
      "a*x + b ~ c exp(-keff(x - xleft))\na\tb\tkeff\txleft\txright" 
                  << endl;

    /// @todo selection mode ? (do we need that ?)
    while(! loop.finished()) {
      int action = reglinHandler.nextAction(loop);
      pick.processEvent(action);        // We don't filter actions out.
      
      switch(action) {
      case LeftPick:
      case RightPick: {
        ds = pick.dataset();
        if(! ds)
          ds = bds;
        r.setX(loop.position(view.mainPanel()).x(), loop.button());
        reg = ds->reglin(r.xmin(), r.xmax());
        double y = reg.first * xleft + reg.second;
        line.p1 = QPointF(xleft, y);
        y = reg.first * xright + reg.second;
        line.p2 = QPointF(xright, y);

        // Apparent first-order rate constants
        decay_rate = - reg.first/(reg.second + reg.first * r.xleft);
        Terminal::out << reg.first << "\t" << reg.second 
                      << "\t" << decay_rate 
                      << "\t" << r.xleft << "\t" << r.xright << endl;
        soas().showMessage(QObject::tr("Regression between X=%1 and X=%2").
                           arg(r.xleft).arg(r.xright));

        // Now, we fill the d vector with data
        if(d.xvalues.size() == 0) {
          // First time in the loop
          d.xvalues = ds->x();
          d.yvalues = ds->y(); // Not really important, only size
          // matters
          d_exp.xvalues = ds->x();
          d_exp.yvalues = ds->x();
        }
        double dy_min = 0;
        double dy_max = 0;
        double y0 = reg.first * r.xmin() + reg.second;
        for(int i = 0; i < d.xvalues.size(); i++) {
          double x = d.xvalues[i];
          double y = ds->y()[i] -  x * reg.first - reg.second;
          d.yvalues[i] = y;
          if(x >= r.xleft && x <= r.xright) {
            if(y < dy_min)
              dy_min = y;
            if(y > dy_max)
              dy_max = y;
          }
          d_exp.yvalues[i] = y0*exp(decay_rate * (r.xmin() - x));
        }
        bottom.setYRange(dy_min, dy_max, view.mainPanel());
        break;
      }
      case Quit:
        return;
      case Subtract: {
        // Subtracting, the data is already computed in d
        DataSet * newds = ds->derivedDataSet(d.yvalues, "_linsub.dat");
        soas().pushDataSet(newds);
        return;
      }
      case Write: {
        ValueHash e;
        e << "dataset" << ds->name
          << "a" << reg.first << "b" << reg.second 
          << "keff" << decay_rate << "xleft" << r.xleft 
          << "xright" << r.xright ;
        OutFile::out.writeValueHash(e, ds);
        Terminal::out << "Writing to output file " << OutFile::out.fileName()
                      << endl;
        break;
      }
      case Divide: {
        // Dividing
        Vector newy = ds->y();
        for(int i = 0; i < newy.size(); i++)
          newy[i] /= (d.xvalues[i] * reg.first + reg.second);
        DataSet * newds = ds->derivedDataSet(newy, "_lindiv.dat");
        soas().pushDataSet(newds);
        return;
      }
      case Exponential: {
        // Dividing by exponential decay
        Vector newy = ds->y();
        for(int i = 1; i < newy.size(); i++)
          newy[i] *= exp(decay_rate * (d.xvalues[i] - d.xvalues[0]));
        DataSet * newds = ds->derivedDataSet(newy, "_expdiv.dat");
        soas().pushDataSet(newds);
        return;
      }
      case ShowExponential:
        d_exp.hidden = ! d_exp.hidden;
        break;
      case Peak: {
        /// @todo Some of the code here should be merged with code in
        /// Peak
        
        // We look for the first peak in the direction of the dx.
        int min_idx = ds->x().closestPoint(r.xmin());
        int max_idx = ds->x().closestPoint(r.xmax());

        // Looking left or right ? If left, we search for min, if
        // right, for max.

        if(d.xvalues.size() == 0)
          Terminal::out << "No regression yet" << endl;
          

        int i_hw = -1;
        int i_pk = -1;
        double pk_v;

        if(min_idx > max_idx) {
          // We look for minima below min_idx
          for(int i = min_idx; i < ds->x().size(); i++) {
            double v = d.yvalues[i];
            if(i_pk < 0 || v < pk_v) {
              i_pk = i;
              pk_v = v;
            }
            if(i_pk >= 0 && (v > 0 || v > 0.9 * pk_v) &&
               (v < 0.8*d.yvalues.min()) ) {
              break;
            }
          }

          // now look for half peak position.
          for(int i = i_pk; i > min_idx; i--) {
            double v = d.yvalues[i];
            if(v >= 0.5 * pk_v) {
              i_hw = i;
              break;
            }
          }
        }
        else {
          // We look for minima below min_idx
          for(int i = max_idx; i < ds->x().size(); i++) {
            double v = d.yvalues[i];
            if(i_pk < 0 || v > pk_v) {
              i_pk = i;
              pk_v = v;
            }
            if(i_pk >= 0 && (v < 0 || v < 0.9 * pk_v) &&
               (v > 0.8*d.yvalues.max()) ) {
              break;
            }
          }

          // now look for half peak position.
          for(int i = i_pk; i > max_idx; i--) {
            double v = d.yvalues[i];
            if(v <= 0.5 * pk_v) {
              i_hw = i;
              break;
            }
          }
        }
        if(i_pk >= 0) {
          Terminal::out << "Peak found: "
                        << "  x = " << ds->x()[i_pk]
                        << "\n  val = " << d.yvalues[i_pk];
          double y = reg.first * xleft + reg.second + d.yvalues[i_pk];
          fpl.p1 = QPointF(xleft, y);
          y = reg.first * xright + reg.second + d.yvalues[i_pk];
          fpl.p2 = QPointF(xright, y);
          fpl.hidden = false;


          y = reg.first * ds->x()[i_pk] + reg.second;
          fp.p1 = QPointF(ds->x()[i_pk], y);
          fp.p2 = QPointF(ds->x()[i_pk], ds->y()[i_pk]);
          fp.hidden = false;

          
          if(i_hw >= 0) {
            Terminal::out << "\n  hw = " << d.xvalues[i_hw];
            /* 
            y = reg.first * xleft + reg.second + d.yvalues[i_pk] * 0.5;
            hp.p1 = QPointF(xleft, y);
            y = reg.first * xright + reg.second + d.yvalues[i_pk] * 0.5;
            hp.p2 = QPointF(xright, y);
            hp.hidden = false;*/

            double y = reg.first * ds->x()[i_hw] + reg.second;
            hp.p1 = QPointF(ds->x()[i_hw], y);
            hp.p2 = QPointF(ds->x()[i_hw], ds->y()[i_hw]);
            hp.hidden = false;
          }
          Terminal::out << endl;

          
        }
      }
      default:
        ;
      }
    }
  }

  static Command 
  reg("reglin", // command name
      optionLessEffector(reglinCommand), // action
      "buffer",  // group name
      NULL, // arguments
      NULL, // options
      "Linear regression",
      "Performs linear regression",
      "reg");
};

//////////////////////////////////////////////////////////////////////

static void autoRegCommand(const QString &, const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  int window = 7;

  updateFromOptions(opts, "window", window);
  window = abs(window);

  bool filter = false;
  updateFromOptions(opts, "filter", filter);

  const Vector & x = ds->x();
  Vector ny = ds->y();

  int sz = ny.size();


  /// @todo This could be greatly optimized
  for(int i = 0; i < sz; i++) {
    int lx = i - window;
    if(lx < 0)
      lx = 0;
    int rx = i + window;
    if(rx >= sz)
      rx = sz-1;
    QPair<double, double> ab = ds->reglin(lx, rx);
    if(filter)
      ny[i] = ab.first * x[i] + ab.second;
    else
      ny[i] = ab.first;
  }
  soas().pushDataSet(ds->derivedDataSet(ny,"_reg.dat"));
}

static ArgumentList 
arOps(QList<Argument *>() 
      << new IntegerArgument("window", 
                             "Number of points",
                             "Number of points (after and before) over which to perform regression")
      << new BoolArgument("filter",
                          "Filter",
                          "If true (not the default), filter the data instead of computing the slope")
      );


static Command 
areg("auto-reglin", // command name
     effector(autoRegCommand), // action
     "filters",  // group name
     NULL, // arguments
     &arOps, // options
     "Automatic linear regression",
     "Automatic linear regression");



//////////////////////////////////////////////////////////////////////

namespace __fl {

  typedef enum {
    PickLeft,
    PickRight,
    ZoomIn,
    ZoomOut,
    QuitDividing,
    QuitReplacing,
    NextSegment,
    PreviousSegment,
    SetManually,
    RegressionMode,
    LineMode,
    Abort
  } FilmLossActions;

  static EventHandler filmLossHandler = EventHandler("film-loss").
    addClick(Qt::LeftButton, PickLeft, "pick left bound").
    addClick(Qt::RightButton, PickRight, "pick right bound").
    addKey('q', QuitDividing, "quit dividing by coverage").
    alsoKey('Q').
    addKey(Qt::Key_Escape, Abort).
    addKey('z', ZoomIn, "zoom to current segment").
    addKey('Z', ZoomOut, "zoom to overall view").
    addKey(' ', NextSegment, "next segment").
    alsoKey('n').alsoKey('N').
    addKey('p', PreviousSegment, "previous segment").
    alsoKey('P').alsoKey('b').alsoKey('B').
    addKey('k', SetManually, "set manually rate constant").
    alsoKey('K').
    addKey('r', RegressionMode, "regression mode").
    alsoKey('R').
    addKey('l', LineMode, "line mode").
    alsoKey('L')
    ;


/// @todo This should be turned into a lambda expression !
static void zoomToSegment(const DataSet * ds, int seg, CurveView & view)
{
  CurvePanel * p = view.mainPanel();
  QRectF r = p->currentZoom();
  
  int s = ds->segments.value(seg-1, 0);
  int e = ds->segments.value(seg, ds->x().size()) - 1;
  r.setLeft(std::min(ds->x()[s],ds->x()[e]));
  r.setRight(std::max(ds->x()[s],ds->x()[e]));

  Terminal::out << "Displaying segment #" << seg + 1 << endl;

  p->zoomIn(r);
}

static void filmLossCommand(CurveEventLoop &loop, const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  const GraphicsSettings & gs = soas().graphicsSettings();

  DataSet * nds = NULL;

  CurveLine line;
  CurveHorizontalRegion r;
  CurveView & view = soas().view();
  CurvePanel bottom;
  CurveData d;
  CurveData corr;
  d.xvalues = ds->x();
  d.yvalues = ds->x();
  corr.xvalues = ds->x();
  corr.yvalues = ds->y();
  bottom.drawingXTicks = false;
  bottom.stretch = 30;        // 3/10ths of the main panel.
  view.addItem(&line);
  view.addItem(&r);
  bottom.addItem(&d);
  view.addItem(&corr);

  CurveMarker lm;
  view.addItem(&lm);
  lm.setupCursorMarker(QColor(0,0,255,100)); // A kind of transparent blue
  CurveMarker rm;
  view.addItem(&rm);
  rm.setupCursorMarker(QColor(0,128,0,100)); // A kind of transparent blue


  bottom.yLabel = "Remaining coverage";
  d.countBB = true;


  view.addPanel(&bottom);
  line.pen = gs.getPen(GraphicsSettings::ReglinPen);
  d.pen = gs.getPen(GraphicsSettings::ResultPen);
  double xleft = ds->x().min();
  double xright = ds->x().max();

  PointPicker pick(&loop, ds);
  pick.trackedButtons = Qt::LeftButton|Qt::RightButton;
  pick.resetMethod();

  
  loop.setHelpString(QString("Film loss:\n")
                       + filmLossHandler.buildHelpString());
  bool needUpdate = false;

  bool regressionMode = true;

  int currentSegment = 0;
  zoomToSegment(ds, currentSegment, view);

  QVarLengthArray<double, 200> rateConstants(ds->segments.size() + 1);
  for(int i = 0; i <= ds->segments.size(); i++)
    rateConstants[i] = 0;

  auto makeMeta = [&rateConstants, ds]() -> QList<QVariant> {
    QList<QVariant> rv;
    for(int i = 0; i <= ds->segments.size(); i++)
      rv << rateConstants[i];
    return rv;
  };

  auto updateDecay =
    [&regressionMode, &rateConstants, &needUpdate,
     &currentSegment, &r, &line, &rm, &lm, xleft, xright, ds] {
    double decay;
    if(regressionMode) {
      QPair<double, double> reg = ds->reglin(r.xmin(), r.xmax());
      double y = reg.first * r.xleft + reg.second;
      line.p1 = QPointF(r.xleft, y);
      y = reg.first * r.xright + reg.second;
      line.p2 = QPointF(r.xright, y);
      decay = -reg.first/(reg.second + reg.first*line.p1.x());
    }
    else {
      line.p1 = lm.p;
      line.p2 = rm.p;
      if(line.p1.y() * line.p2.y() > 0)
        decay = log(line.p1.y()/line.p2.y())/
          (line.p2.x() - line.p1.x());
      else
        decay = 0;
    }
    double a = (line.p2.y() - line.p1.y())/
    (line.p2.x() - line.p1.x());
    double b = line.p1.y() - line.p1.x()*a;
    rateConstants[currentSegment] = decay;
    Terminal::out << "xleft = " << r.xleft << "\t" 
    << "xright = " << r.xright << "\t" 
    << "x1 = " << line.p1.x() << "\t" 
    << "x2 = " << line.p2.x() << "\t" 
    << a << "\t" << b 
    << "\t" << decay << endl;

    double y = a * xleft + b;
    line.p1 = QPointF(xleft, y);
    y = a * xright + b;
    line.p2 = QPointF(xright, y);

    // Now, update
    if(regressionMode)
      soas().showMessage(QString("Regression between X=%1 and X=%2 "
                                 "(segment #%3)").
                         arg(r.xleft).arg(r.xright).
                         arg(currentSegment + 1));
    else
      soas().showMessage(QString("Drawing line between the points "
                                 "(segment #%3)").
                         arg(currentSegment + 1));
    needUpdate = true;
  };

  while(! loop.finished()) {
    int action = filmLossHandler.nextAction(loop);
    pick.processEvent(action);        // We don't filter actions out.
      
    switch(action) {
    case PickLeft:
    case PickRight: {
      // r.setX(pick.point().x(), loop.button());
      if(action == PickLeft) {
        lm.p = pick.point();
        r.xleft = lm.p.x();
      }
      else {
        rm.p = pick.point();
        r.xright = rm.p.x();
      }
      updateDecay();

      break;
    }
    case RegressionMode:
      regressionMode = true;
      Terminal::out << "Using regression mode" << endl;
      updateDecay();
      break;
    case LineMode:
      regressionMode = false;
      Terminal::out << "Using line mode" << endl;
      updateDecay();
      break;
    case SetManually: {
      QString str = loop.promptForString("Decay rate constant:");
      bool ok = false;
      double val = str.toDouble(&ok);
      if(ok) {
        rateConstants[currentSegment] = val;
        needUpdate = true;
      }
      break;
    }
    case ZoomIn:
      zoomToSegment(ds, currentSegment, view);
      break;
    case ZoomOut:
      view.mainPanel()->zoomIn(QRectF());
      break;
    case QuitDividing:
      nds = ds->derivedDataSet(corr.yvalues, "_corr.dat");
      nds->setMetaData("filmloss_rates", makeMeta());
      soas().pushDataSet(nds);
      return;
    case QuitReplacing:
      nds = ds->derivedDataSet(d.yvalues, "_coverage.dat");
      nds->setMetaData("filmloss_rates", makeMeta());
      soas().pushDataSet(nds);
      return;
    case Abort:
      return;
    case NextSegment:
      currentSegment += 1;
      if(currentSegment > ds->segments.size())
        currentSegment = ds->segments.size();
      else
        zoomToSegment(ds, currentSegment, view);
      break;
    case PreviousSegment:
      currentSegment -= 1;
      if(currentSegment < 0)
        currentSegment = 0;
      else
        zoomToSegment(ds, currentSegment, view);
      break;
    default:
      ;
    }
    if(needUpdate) {
      // Here, we update the remaining coverage...

      double init = 1;
      for(int i = 0; i <= ds->segments.size(); i++) {
        double r = rateConstants[i];
        int s = ds->segments.value(i-1, 0);
        int e = ds->segments.value(i, d.xvalues.size()) - 1;
        double xs = (s == 0 ? d.xvalues[0] : d.xvalues[s-1]);
        for(int j = s; j <= e; j++) {
          d.yvalues[j] = init * exp(-r * (d.xvalues[j] - xs));
          corr.yvalues[j] = ds->y()[j] / d.yvalues[j];
        }
        init = d.yvalues[e];
      }
      
      bottom.setYRange(d.yvalues.min(), d.yvalues.max(), view.mainPanel());
    }
  }
}

static Command 
fl("film-loss", // command name
    optionLessEffector(filmLossCommand), // action
    "buffer",  // group name
    NULL, // arguments
    NULL, // options
    "Film loss",
    "Corrects for film loss segment-by-segment");

};

//////////////////////////////////////////////////////////////////////


namespace __bl {
typedef enum {
  AddPoint,
  RemovePoint,
  CycleSplineType,
  Add10Left,
  Add10Right,
  Add20Everywhere,
  QuitSavingPoints,
  LoadExact,
  LoadXValues,
  Abort
} BaselineActions;

static EventHandler baselineHandler = EventHandler("baseline").
  addClick(Qt::LeftButton, AddPoint, "place marker").
  addClick(Qt::RightButton, RemovePoint, "remove marker").
  baselineHandler().
  addPointPicker().
  addKey('1', Add10Left, "add 10 to the left").
  addKey('2', Add10Right, "add 10 to the right").
  addKey('0', Add20Everywhere, "add 20 regularly spaced").
  addKey('t', CycleSplineType, "cycle interpolation types").
  alsoKey('T').
  addKey('l', LoadXValues, "load X values from buffer").
  addKey('L', LoadExact, "load X/Y values from buffer").
  addKey('p', QuitSavingPoints, "quit saving interpolation points").
  alsoKey('P').
  addKey(Qt::Key_Escape, Abort, "abort");
  

static void baselineCommand(CurveEventLoop &loop, const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  // const GraphicsSettings & gs = soas().graphicsSettings();

  /// The area computed in the previous call
  /// @todo Make that more flexible ?
  static double oldArea = 0;
  double area = 0;
  static double oldCharge = 0;
  double charge = 0;
  // Make sure the area is updated whenever we go out of scope ?
  DelayedAssign<double> arr(oldArea, area);
  DelayedAssign<double> chrg(oldCharge, charge);

  bool hasSr = false;
  double sr = ds->metaScanRate(&hasSr);
  

  CurveView & view = soas().view();
  CurveMarker m;
  Spline s;

  BaselineHandler h(view, ds, "bl", BaselineHandler::None);
  PointPicker pick(&loop, ds);
  view.addItem(&m);

  m.size = 4;
  m.pen = QPen(Qt::NoPen);
  m.brush = QBrush(QColor(0,0,255,100)); /// @todo customize that too 

  Spline::Type type = Spline::CSpline;

  loop.setHelpString("Spline interpolation:\n"
                     + baselineHandler.buildHelpString());

  while(! loop.finished()) {
    bool needCompute = false;
    bool isLeft = false;     // Used for sharing code for the 1/2 actions.


    bool computeDerivative = false;
    bool shouldQuit = false;
    int action = baselineHandler.nextAction(loop);

    pick.processEvent(action);        // We don't filter actions out.
    if(h.nextAction(action, &shouldQuit, &computeDerivative,
                    &needCompute)) {
      if(shouldQuit)
        return;
    }
    else {
      switch(action) {
      case AddPoint: 
        s.insert(pick.point());
        needCompute = true;
        break;
      case RemovePoint:
        s.remove(loop.position().x());
        needCompute = true;
        break;
      case Abort:
        chrg.cancel = true;
        return;
      case QuitSavingPoints: {
        DataSet * nds = ds->derivedDataSet(m.points, "_bl_points.dat");
        soas().pushDataSet(nds);
        return;
      }
      case LoadExact:
      case LoadXValues: {
        bool ok;
        QString str = loop.promptForString("Load from buffer number: ", &ok);
        if(ok) {
          int nb = str.toInt(&ok);
          if(!ok)
            break;
          const DataSet * base = soas().stack().numberedDataSet(nb);
          if(! base) {
            Terminal::out << "Invalid buffer number: " << nb << endl;
            break;
          }
          s.clear();
          for(int i = 0; i < base->nbRows(); i++) {
            if(action == LoadExact)
              s.insert(base->pointAt(i));
            else {
              pick.pickAtX(base->x()[i]);
              s.insert(pick.point());
            }
          }
          needCompute = true;
        }
        break;
      }
      case CycleSplineType:
        type = Spline::nextType(type);
        Terminal::out << "Now using interpolation: " 
                      << Spline::typeName(type) << endl;
        needCompute = true;
        break;
      case Add10Left:
        isLeft = true;
      case Add10Right: {
        // Closest data point:
        int side = (isLeft ? ds->x().whereMin() :
                    ds->x().whereMax());
        QPair<double, int> dst = loop.distanceToDataSet(ds);

        int cur = dst.second;
        QList<QPointF> points = pick.pickBetween(side, cur, 10);
        for(int i = 0; i < points.size(); i++)
          s.insert(points[i]);
        needCompute = true;
      }
        break;
      case Add20Everywhere: {
        QList<QPointF> points = pick.pickBetween(0, ds->x().size()-1, 20);
        for(int i = 0; i < points.size(); i++)
          s.insert(points[i]);
        needCompute = true;
      
      }
      default:
        ;
      }
    }

    if(needCompute) {
      if(s.size() > 0)
        h.modified.yvalues = s.evaluate(h.diff.xvalues, type);
      else
        h.modified.yvalues.fill(0.0);
      if(computeDerivative)
        h.derivative.yvalues = s.derivative(h.diff.xvalues, type);

      h.updateBottom();
      area = Vector::integrate(ds->x(), h.diff.yvalues);
      if(hasSr > 0) {
        charge = area/sr;
        Terminal::out << "Area: " << area
                      << " (charge: " << charge << " with scan rate: " << sr 
                      << ") -- old area: " 
                      << oldArea
                      << " (old charge: " <<  oldCharge << ")" << endl;
      }
      else
        Terminal::out << "Area: " << area << " -- old area: " 
                      << oldArea << endl;

      
      m.points = s.pointList();

      // ?? no ?
      // h.bottom.setYRange(diff.yvalues.min(), diff.yvalues.max(), 
      //                    view.mainPanel());
    }
  }
  
}

static Command 
bsl("baseline", // command name
    optionLessEffector(baselineCommand), // action
    "buffer",  // group name
    NULL, // arguments
    NULL, // options
    "Baseline",
    "Interpolation-based baseline",
    "b");
}


//////////////////////////////////////////////////////////////////////

static void interpolateCommand(const QString &, 
                               DataSet * ds,
                               DataSet * nodes, const CommandOptions & opts)
{
  Spline s;
  for(int i = 0; i < nodes->nbRows(); i++)
    s.insert(nodes->pointAt(i));

  Spline::Type type = Spline::CSpline;
  updateFromOptions(opts, "type", type);
  
  Vector ny = s.evaluate(ds->x(), type);

  DataSet * nds = ds->derivedDataSet(ny, "_int.dat");
  soas().pushDataSet(nds);
}

static ArgumentList 
intArgs(QList<Argument *>() 
              << new DataSetArgument("xvalues", 
                                     "X values",
                                     "Dataset serving as base for X values")
              << new DataSetArgument("nodes", 
                                     "Nodes",
                                     "Dataset containing the nodes X/Y values"));

static ArgumentList 
intOpts(QList<Argument *>() 
        << new TemplateChoiceArgument<Spline::Type>
        (Spline::interpolationTypes(),"type", 
         "Type",
         "Interpolation type"));

static Command 
interpolate("interpolate", // command name
            effector(interpolateCommand), // action
            "buffer",  // group name
            &intArgs, // arguments
            &intOpts, // options
            "Interpolate",
            "Recompute interpolation with given nodes");




//////////////////////////////////////////////////////////////////////


namespace __cbl {
typedef enum {
  AddPoint,
  NextMarker,
  ToggleExponential,
  Pick1,
  Pick2 = Pick1+1,
  Pick3 = Pick1+2,
  Pick4 = Pick1+3,
  Abort,
} BaselineActions;

static EventHandler baselineHandler = EventHandler("catalytic-baseline").
  addClick(Qt::LeftButton, AddPoint, "place marker").
  addClick(Qt::RightButton, NextMarker, "next marker").
  baselineHandler(BaselineHandler::NoDerivative).
  addPointPicker().
  addKey('1', Pick1, "pick point 1").
  addKey('2', Pick2, "pick point 2").
  addKey('3', Pick3, "pick point 3").
  addKey('4', Pick4, "pick point 4").
  addKey('e', ToggleExponential, "toggle exponential").
  alsoKey('E').
  addKey(Qt::Key_Escape, Abort, "abort");
  

static void cBaselineCommand(CurveEventLoop &loop, const QString &)
{
  const DataSet * ds = soas().currentDataSet();

  CurveView & view = soas().view();
  CurveMarker m;

  int currentIndex = 0;

  BaselineHandler h(view, ds, "cbl", BaselineHandler::NoDerivative);
  PointPicker pick(&loop, ds);
  view.addItem(&m);

  m.size = 4;
  m.pen = QPen(Qt::NoPen);
  m.brush = QBrush(QColor(0,0,255,100)); /// @todo customize that too 

  loop.setHelpString("Catalytic baseline:\n"
                     + baselineHandler.buildHelpString());

  bool isExponential = false;

  while(! loop.finished()) {
    bool needCompute = false;


    bool computeDerivative = false;
    bool shouldQuit = false;
    int action = baselineHandler.nextAction(loop);
    pick.processEvent(action);        // We don't filter actions out.
    if(h.nextAction(action, &shouldQuit, &computeDerivative,
                    &needCompute)) {
      if(shouldQuit)
        return;
    }
    else {
      switch(action) {
      case AddPoint:
        if(m.points.size() <= currentIndex) {
          m.points << pick.point();
          m.labels << QString::number(m.points.size());
          currentIndex = m.points.size() - 1;
        }
        else 
          m.points[currentIndex] = pick.point();
        ++currentIndex;
        currentIndex %= 4;
        needCompute = true;
        break;
      case NextMarker:
        ++currentIndex;
        currentIndex %= 4;
        break;
      case ToggleExponential:
        isExponential = ! isExponential;
        needCompute = true;
        Terminal::out << "Using " 
                      << (isExponential ? "exponential" : "quadratic") 
                      << " baseline" << endl;
        break;
      case Pick1:
      case Pick2:
      case Pick3:
      case Pick4: {
        int w = action - Pick1;
        pick.pickPoint();
        if(m.points.size() <= w) {
          m.points << pick.point();
          m.labels << QString::number(m.points.size());
        }
        else 
          m.points[w] = pick.point();
        needCompute = true;
        break;
      }
      case Abort:
        return;
      default:
        ;
      }
    }

    if(needCompute) {
      if(m.points.size() == 4) {

        /// @todo Provide various strategies (for instance, one could
        /// use an exponential decay, too ?) Any three-parameters
        /// function would do (provided the equation can be inverted
        /// at compile time, else it would be too much of a pain)

        // We need to extract the parameters:
        double x_0 = 0.5 * (m.points[0].x() + m.points[1].x());
        double y_0 = (isExponential ? 
                      sqrt(m.points[0].y()*m.points[1].y()) * 
                      (m.points[0].y() >= 0 ? 1 : -1) : 
                      0.5 * (m.points[0].y() + m.points[1].y()));
        double dy_0 = (m.points[0].y() - m.points[1].y())/
          (m.points[0].x() - m.points[1].x());

        double x_lim = 0.5 * (m.points[2].x() + m.points[3].x());
        // y_lim isn't necessary
        double dy_lim = (m.points[2].y() - m.points[3].y())/
          (m.points[2].x() - m.points[3].x());

        const Vector & xv = ds->x();
        int sz = xv.size();

        if(isExponential) {
          double tau = (x_lim - x_0)/log(dy_0/dy_lim);
          double a = - dy_0 * tau * exp(x_0/tau);
          double b = y_0 - a * exp(-x_0/tau);
          
          // Here, we solve the real equation, ie exact values at
          // first and second points + ratio of the last two points
          // equal to the first
          double vals[3] = {tau, a, b};

          gsl_vector_view v = gsl_vector_view_array(vals, 3);
          
          LambdaMSolver slv(3, 
                            [&](const gsl_vector * x, 
                                gsl_vector * tg) -> void {
                              double mt = gsl_vector_get(x, 0);
                              double ma = gsl_vector_get(x, 1);
                              double mb = gsl_vector_get(x, 2);

                              gsl_vector_set(tg, 0, 
                                             mb + ma * exp(-m.points[0].x()/mt) - m.points[0].y());
                              gsl_vector_set(tg, 1, 
                                             mb + ma * exp(-m.points[1].x()/mt) - m.points[1].y());
                              double rat = mb + ma * exp(-m.points[3].x()/mt);
                              rat /= mb + ma * exp(-m.points[2].x()/mt);
                              gsl_vector_set(tg, 2, rat - m.points[3].y()/m.points[2].y());
                            });
          try {
            slv.solve(&v.vector);
            const gsl_vector * vect = slv.currentValue();
            tau = gsl_vector_get(vect, 0);
            a = gsl_vector_get(vect, 1);
            b = gsl_vector_get(vect, 2);
          }
          catch(RuntimeError & er) {
            Terminal::out << "Impossible to solve the equation: " 
                          << er.message() << endl;
          }

          for(int i = 0; i < sz; i++)
            h.modified.yvalues[i] = b + a*exp(-xv[i]/tau);
        }
        else {

          // Coming straight from the old soas documentation.
          double a = (dy_lim - dy_0)/(2 * (x_lim - x_0));
          double b = (x_lim * dy_0 - x_0 * dy_lim)/(x_lim - x_0);
          double c = (x_0 * x_0 * (dy_0 + dy_lim) + 
                      2 * y_0 * (x_lim - x_0) - 
                      2 * x_0 * x_lim * dy_0)/(2 * (x_lim - x_0));

          double y_lim_eff = x_lim*x_lim*a + x_lim*b + c;

          for(int i = 0; i < sz; i++) {
            double x = xv[i];
            if((x - x_0) * (x_lim - x) >= 0) {
              // Within the range
              h.modified.yvalues[i] = x*x*a + x*b + c;
            }
            else {
              // Outside the range
              if(fabs(x - x_0) > fabs(x - x_lim)) // Closer to the _lim range
                h.modified.yvalues[i] = (x - x_lim) * dy_lim + y_lim_eff;
              else
                h.modified.yvalues[i] = (x - x_0) * dy_0 + y_0;
            }
          }
        }

        h.updateBottom();
      }
    }
  }
  
}

static Command 
cbsl("catalytic-baseline", // command name
    optionLessEffector(cBaselineCommand), // action
    "buffer",  // group name
    NULL, // arguments
    NULL, // options
    "Catalytic baseline",
    "Cubic baseline",
    "B");
}

//////////////////////////////////////////////////////////////////////

namespace __bs {

  typedef enum {
    AddSegment,
    RemoveSegment,
    ToggleDerivative,
    Optimize,
    EquallySpaced,
    Resample,
    IncreaseSplinesOrder,
    DecreaseSplinesOrder,
    Replace,
    Abort
  } BSplinesActions;

  static EventHandler bsplinesHandler = EventHandler("filter-bsplines").
    addKey(Qt::Key_Escape, Abort, "abort").
    conventionalAccept(Replace, "replace with filtered data").
    addClick(Qt::LeftButton, AddSegment, "add segment").
    addClick(Qt::RightButton, RemoveSegment, "remove segment").
    addKey('d', ToggleDerivative, "toggle display of derivative").
    alsoKey('D').
    addKey('a', EquallySpaced, "equally spaced segments").
    alsoKey('A').
    addKey('r', Resample, "resample output").
    alsoKey('R').
    addKey('o', Optimize, "optimize positions").
    alsoKey('O').
    addKey('+', IncreaseSplinesOrder, "increase splines order").
    addKey('-', DecreaseSplinesOrder, "decrease splines order");
  
  


  static void bsplinesCommand(CurveEventLoop &loop, const QString &, const CommandOptions &opts)
  {
    const DataSet * ds = soas().currentDataSet();
    const GraphicsSettings & gs = soas().graphicsSettings();

    CurveView & view = soas().view();

    CEHideAll ha(view.mainPanel(), false);

    CurvePanel bottom;
    /// @todo This assumes that the currently displayed dataset is the
    /// first one.
    CurveItem * dsDisplay = view.mainPanel()->items().first();
    CurveData d;
    CurveData diff;
    bool derive = false;
    bottom.drawingXTicks = false;
    bottom.stretch = 30;        // 3/10ths of the main panel.
    int nbSegments = 10;
    int order = 4;

    view.addItem(&d);

    d.pen = gs.getPen(GraphicsSettings::BaselinePen);
    d.xvalues = ds->x();
    d.yvalues = QVector<double>(d.xvalues.size(), 0);
    d.countBB = true;
    diff.xvalues = d.xvalues;
    diff.yvalues = ds->y();
    diff.countBB = true;



    bottom.addItem(&diff);
    bottom.yLabel = Utils::deltaStr("Y");

    view.addPanel(&bottom);



    /// Position of the segments
    Vector x;
    bool autoXValues = false;
    bool needCompute = true;
    BSplines splines(ds, order);
    splines.autoBreakPoints(nbSegments-1);

    {
      ColumnSpecification cl;
      updateFromOptions(opts, "weight-column", cl);
      if(cl.isValid())
        splines.setWeights(cl.getColumn(ds));
    }
    x = splines.getBreakPoints();

    int resample = -1;            // If not negative, resample to that
    // number of samples.

    CurveVerticalLines lines;
    lines.xValues = &x;
    lines.pen = gs.getPen(GraphicsSettings::SeparationPen);
    view.addItem(&lines);

    loop.setHelpString("B-splines filtering:\n"
                       + bsplinesHandler.buildHelpString());
    do {
      switch(bsplinesHandler.nextAction(loop)) {
      case Replace:
        soas().pushDataSet(ds->derivedDataSet(d.yvalues, "_filtered.dat"));
        return;
      case RemoveSegment: { // Remove
        double xv = loop.position().x();
        if(x.size() <= 2)
          break;                // We don't remove.
        for(int i = 0; i < x.size() - 1; i++) {
          if(xv >= x[i] && xv <= x[i+1]) {
            if(i == x.size() - 2 || 
               ((xv - x[i]) < (x[i+1] - xv) && i > 0))
              x.remove(i);
            else
              x.remove(i+1);
          }
        }
        nbSegments = x.size() -1;
        needCompute = true;
        Terminal::out << "Using : " << nbSegments << " segments" << endl;
        break;
      }
      case AddSegment: {
        double xv = loop.position().x();
        for(int i = 0; i < x.size() - 1; i++)
          if(xv >= x[i] && xv <= x[i+1]) {
            x.insert(i+1, xv);
            break;
          }
        
        nbSegments += 1;
        needCompute = true;
        Terminal::out << "Using : " << nbSegments << " segments" << endl;
        break;
      }
      case Abort:
        return;
      case ToggleDerivative:
        derive = ! derive;
        dsDisplay->hidden = derive;
        needCompute = true;
        if(derive)
          soas().showMessage("Showing derivative");
        else
          soas().showMessage("Showing filtered data");
        break;
      case Optimize: {
        splines.optimize(15, false);
        x = splines.getBreakPoints();
        needCompute = true;
        break;
      }
      case EquallySpaced: 
        needCompute = true;
        autoXValues = true;
        break;
      case Resample: {
        bool ok = false;
        QString str = loop.promptForString("Resample to the given number of points:", &ok);
        if(ok) {
          int val = str.toInt(&ok);
          needCompute = true;
          if(ok && val > 2) {
            resample = val;
            Terminal::out << "Resampling to " << resample << " points" << endl;
          }
          else {
            val = -1;
            Terminal::out << "Not resampling anymore" << endl;
          }
        }
        else
          soas().showMessage("Not changing resampling");
        break;
      }
      case IncreaseSplinesOrder:
        ++order;
        Terminal::out << "Now using splines of order " << order << endl;
        needCompute = true;
        break;
      case DecreaseSplinesOrder:
        --order;
        if(order < 2)
          order = 2;            // must be at least linear !
        Terminal::out << "Now using splines of order " << order << endl;
        needCompute = true;
        break;
      default:
        ;
      }
      if(needCompute) {
        splines.setOrder(order);
        if(autoXValues) {
          splines.autoBreakPoints(nbSegments - 1);
          x = splines.getBreakPoints();
          autoXValues = false;
        }
        else
          splines.setBreakPoints(x);

        // Should move to yet another place
        double value = splines.computeCoefficients();
        if(derive) {
          diff.yvalues = ds->y() - splines.computeValues();

          if(resample < 2)
            d.yvalues = splines.computeValues(1);
          else {
            d.xvalues = ds->x().uniformlySpaced(resample);
            d.yvalues = splines.computeValues(d.xvalues, 1);
          }
        }
        else {
          if(resample < 2) {
            d.yvalues = splines.computeValues();
            diff.yvalues = ds->y() - d.yvalues;
          }
          else {
            d.xvalues = ds->x().uniformlySpaced(resample);
            d.yvalues = splines.computeValues(d.xvalues);
            diff.yvalues = ds->y() - splines.computeValues();
          }
        }
        Terminal::out << "Residuals: " << sqrt(value) << endl;
        bottom.setYRange(diff.yvalues.min(), diff.yvalues.max(), 
                         view.mainPanel());
        needCompute = false;
      }
    } while(! loop.finished());
  }

  static ArgumentList 
  bsOpts(QList<Argument *>() 
      << new ColumnArgument("weight-column", 
                            "Weights",
                            "Use the weights in the given column",
                            false, false, true)
        );

  static Command 
  bspl("filter-bsplines", // command name
       effector(bsplinesCommand), // action
       "filters",  // group name
       NULL, // arguments
       &bsOpts, // options
       "B-Splines filter",
       "Filter using bsplines");
};

//////////////////////////////////////////////////////////////////////

namespace __fft {

  typedef enum {
    IncreaseSetCutoff,
    DecreaseCutoff,
    ToggleDerivative,
    ToggleBaseline,
    TogglePowerSpectrum,
    QuitPushingTransform,
    ChangeAlpha,
    Replace,
    Abort
  } FFTActions;

  static EventHandler fftHandler = EventHandler("filter-fft").
    addKey(Qt::Key_Escape, Abort, "abort").
    conventionalAccept(Replace, "replace with filtered data").
    addClick(Qt::LeftButton, IncreaseSetCutoff, "increase/set cutoff").
    addClick(Qt::RightButton, DecreaseCutoff, "decrease cutoff").
    addKey('d', ToggleDerivative, "toggle display of derivative").
    alsoKey('D').
    addKey('b', ToggleBaseline, "toggle display of baseline").
    addKey('a', ChangeAlpha, "change alpha").
    alsoKey('A').
    addKey('T', QuitPushingTransform, "replace with transform").
    addKey('p', TogglePowerSpectrum, "display power spectrum").
    alsoKey('P');


  /// @todo Add windowing function change, bandcut filters, highpass filter ?
  static void fftCommand(CurveEventLoop &loop, const QString &, 
                         const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  const GraphicsSettings & gs = soas().graphicsSettings();

  CurveView & view = soas().view();
  CurvePanel bottom;
  CurvePanel spectrum;

  CurveItem * dsDisplay = view.mainPanel()->items().first();

  CEHideAll ha(view.mainPanel(), false);

  CurveData d;
  CurveData diff;
  CurveData baseline;
  CurveData spec1;
  CurveData spec2;
  CurveVerticalLine lim;


  double ap = 0.05;
  bool recomputeForward = true;
  FFT orig(ds->x(), ds->y());

  double dmin, dmax;
  ds->x().deltaStats(&dmin, &dmax);

  bool unevenDX = (fabs(dmin) * 1.05 > fabs(dmax) ? false : true);



  d.pen = gs.getPen(GraphicsSettings::ReglinPen);
  d.xvalues = ds->x();
  d.yvalues = QVector<double>(d.xvalues.size(), 0);
  d.countBB = true;
  view.addItem(&d);


  baseline.pen = gs.getPen(GraphicsSettings::BaselinePen);
  baseline.xvalues = ds->x();
  baseline.yvalues = baseline.xvalues;;
  baseline.countBB = false;

  // We don't show base line by default
  baseline.hidden = true;
  view.addItem(&baseline);

  // **************************************************
  // Setup of the "diff" panel


  bottom.stretch = 40;        // 4/10ths of the main panel.

  diff.xvalues = d.xvalues;
  diff.yvalues = ds->y();
  diff.countBB = true;
  bottom.addItem(&diff);
  view.addPanel(&bottom);

  bottom.yLabel = Utils::deltaStr("Y");
  bottom.drawingXTicks = false;

  
  int order = 0;
  updateFromOptions(opts, "derive", order);


  // **************************************************
  // Setup of the "power" panel


  spec1.pen = gs.dataSetPen(0);
  // We don't display the 0th frequency !
  spec1.xvalues = Vector(orig.frequencies() - 1,0);
  for(int i = 0; i < spec1.xvalues.size(); i++)
    spec1.xvalues[i] = log10((i+1)*0.5/(spec1.xvalues.size() * fabs(orig.deltaX)));
  spec1.yvalues = spec1.xvalues;
  spec1.countBB = true;
  spec1.histogram = true;
  spectrum.addItem(&spec1);

  spec2.pen = gs.getPen(GraphicsSettings::ReglinPen);
  spec2.xvalues = spec1.xvalues;
  spec2.yvalues = spec1.yvalues;
  spec2.histogram = true;
  spec2.countBB = false;        // Makes things complicated upon
                                // encountering NaNs...
  spectrum.addItem(&spec2);

  spectrum.yLabel = "Power (log)";
  spectrum.xLabel = "Frequency (log)";

  spectrum.stretch = 40;
  spectrum.anyZoom = true;

  lim.pen = gs.getPen(GraphicsSettings::SeparationPen);
  spectrum.addItem(&lim);




  /// Position of the segments
  Vector x;
  bool needUpdate = false;
  bool showSpectrum = false;
  bool needCompute = true;

  int cutoff = 20;
  dsDisplay->hidden = order > 0;


  {
    QList<int> facts = orig.factors();
    QStringList lst;
    for(int i = 0; i < facts.size(); i++)
      lst << QString::number(facts[i]);
    Terminal::out << "Mixed-radix: " << ds->x().size() << " factorized as "
                  << lst.join(" * ") << endl;
  }

  if(unevenDX)
    Terminal::out << "X values are not evenly spaced. This may be OK for filtering, but will give false results for deriving" << endl;

  // for(int i = 0; i < spec1.yvalues.size(); i++)
  //   spec1.yvalues[i] = log10(orig.magnitude(i+1));

  loop.setHelpString("FFT filtering:\n"
                       + fftHandler.buildHelpString());
  do {
    switch(fftHandler.nextAction(loop)) {
    case Replace:
      soas().pushDataSet(ds->derivedDataSet(d.yvalues, "_filtered.dat"));
      return;

    case IncreaseSetCutoff:
      if(loop.currentPanel() == &spectrum) {
        double x = loop.position(&spectrum).x();
        cutoff = 0.5/(pow(10.0, x) * orig.deltaX);
      }
      else {                                 // +/- decrease.
        cutoff++;
        if(cutoff > ds->x().size()/2 - 2)
          cutoff = ds->x().size()/2 - 2;
      }
      Terminal::out << "Now using a cutoff of " << cutoff << " points ("
                    << cutoff * orig.deltaX << " in X values)" << endl;
      needCompute = true;
      break;
    case DecreaseCutoff:
      cutoff--;
      if(cutoff < 2)
        cutoff = 2;
      Terminal::out << "Now using a cutoff of " << cutoff << " points ("
                    << cutoff * orig.deltaX << " in X values)" << endl;
      needCompute = true;
      break;
    case Abort:
      return;
    case QuitPushingTransform: {
      soas().pushDataSet(orig.transform(ds));
      return;
    }
    case ToggleBaseline:
      baseline.hidden = ! baseline.hidden;
      break;
    case ToggleDerivative:
      if(order > 0)
        order = 0;
      else
        order = 1;
      dsDisplay->hidden = (order > 0);
      needCompute = true;
      if(order > 0)
        soas().showMessage("Showing derivative");
      else
        soas().showMessage("Showing filtered data");
      break;
    case TogglePowerSpectrum:
      showSpectrum = ! showSpectrum;
      needUpdate = true;
      needCompute = true;
      break;
    case ChangeAlpha: {
      bool ok = false;
      QString na = loop.promptForString("new value of alpha (%):", 
                                        &ok, QString::number(100 * ap));
      if(ok) {
        double v = na.toDouble(&ok);
        if(ok) {
          ap = v*0.01;
          needCompute = true;
          recomputeForward = true;
        }
      }
    }
    default:
        ;
    }
    if(needUpdate) {
      // called whenever the status of the bottom panel changes
      needUpdate = false;
      if(showSpectrum)
        view.setPanel(0, &spectrum);
      else
        view.setPanel(0, &bottom);
    }
    if(needCompute) {
      if(recomputeForward) {
        orig.initialize(ds->x(), ds->y(), true, ap);
        orig.forward();
        recomputeForward = false;

        for(int i = 0; i < spec1.yvalues.size(); i++)
          spec1.yvalues[i] = 2*log10(orig.magnitude(i+1));
      }
      FFT trans = orig;
      lim.x = log10(0.5/(cutoff*orig.deltaX));
      trans.applyGaussianFilter(cutoff);

      if(order > 0)             /// @todo and the second derivative ?
        trans.differentiate();

      if(showSpectrum) {        // We need to update the values
        for(int i = 0; i < spec2.yvalues.size(); i++)
          spec2.yvalues[i] = 2*log10(trans.magnitude(i+1));
      }
        
      trans.reverse();  // Don't use baseline on derivatives (for now)
      d.yvalues = trans.data;
      if(order == 0) 
        diff.yvalues = ds->y() - d.yvalues;

      view.mainPanel()->setYRange(d.yvalues.min(), d.yvalues.max());

      // Compute the baseline
      trans.baseline(&baseline.yvalues);

      if(showSpectrum) {
        QRectF r = spec1.boundingRect();
        r = r.united(spec2.boundingRect());
        r.setTop(spec1.yvalues.finiteMin()-5);
        spectrum.zoomIn(r);
      }
      else
        bottom.setYRange(diff.yvalues.min(), diff.yvalues.max(), 
                         view.mainPanel());
      needCompute = false;
    }
    
  } while(! loop.finished());
}

static ArgumentList 
fftOps(QList<Argument *>() 
       << new IntegerArgument("derive", 
                              "Derivation order",
                              "The starting order of derivation")
      );


static Command 
fft("filter-fft", // command name
    effector(fftCommand), // action
    "filters",  // group name
    NULL, // arguments
    &fftOps, // options
    "FFT filter",
    "Filter using FFT");

}

//////////////////////////////////////////////////////////////////////


static void autoFilterBSCommand(const QString &, const CommandOptions & opts)
{
  int nb = 12;
  int order = 4;
  int derivatives = 0;
  int optimize = 15;



  updateFromOptions(opts, "number", nb);
  updateFromOptions(opts, "order", order);
  updateFromOptions(opts, "optimize", optimize);
  updateFromOptions(opts, "derivatives", derivatives);


  ColumnSpecification cl;
  updateFromOptions(opts, "weight-column", cl);

  DataSetList buffers(opts);
  DataStackHelper pusher(opts);

  for(const DataSet * ds : buffers) {
    Vector weights;
    if(cl.isValid())
      weights = cl.getColumn(ds);

  BSplines splines(ds, order, derivatives);
  splines.autoBreakPoints(nb);
  if(! weights.isEmpty())
    splines.setWeights(weights);

  if(optimize > 0)
    splines.optimize(optimize, false);
  double value = splines.computeCoefficients();
  Terminal::out << "Residuals: " << sqrt(value) << endl;
  for(int i = 0; i <= derivatives; i++)
    pusher << ds->derivedDataSet(splines.computeValues(i),
                                 (i ? QString("_afbs_diff_%1.dat").arg(i) :
                                  "_afbs.dat"));
  }
}

static ArgumentList 
afbsOps(QList<Argument *>() 
        << DataStackHelper::helperOptions()
        << DataSetList::listOptions("Datasets to filter")
        << new IntegerArgument("number", 
                               "Number",
                               "number of segments")
        << new IntegerArgument("order", 
                               "Order",
                               "order of the splines")
        << new IntegerArgument("optimize", 
                               "Optimize",
                               "number of iterations to optimize the position of the nodes (defaults to 15, set to 0 or less to disable)")
        << new ColumnArgument("weight-column", 
                              "Weights",
                              "uses the weights in the given column",
                              false, false, true)
        << new IntegerArgument("derivatives", 
                               "Derivative order",
                               "computes derivatives up to this number")
        );


static Command 
afbs("auto-filter-bs", // command name
     effector(autoFilterBSCommand), // action
     "filters",  // group name
     NULL, // arguments
     &afbsOps, // options
     "Auto B-splines",
     "Filter using bsplines",
     "afbs");

//////////////////////////////////////////////////////////////////////


/// @todo the auto-filter-... commands should be all merged into one
/// (?) or at least provide a consistent interface.
static void autoFilterFFTCommand(const QString &, const CommandOptions & opts)
{
  int cutoff = 20;
  int derivatives = 0;
  bool transform = false;

  updateFromOptions(opts, "cutoff", cutoff);
  updateFromOptions(opts, "derive", derivatives);
  updateFromOptions(opts, "transform", transform);


  DataSetList buffers(opts);
  DataStackHelper pusher(opts);

  for(const DataSet * ds : buffers) {

    double dmin, dmax;
    ds->x().deltaStats(&dmin, &dmax);

    bool unevenDX = (fabs(dmin) * 1.05 > fabs(dmax) ? false : true);
    if(unevenDX) {
      if(derivatives > 0)
        Terminal::out << "WARNING: dx are not even, the derivatives WILL BE WRONG !" << endl;
      else
        Terminal::out << "Warning: dx are not even, but that should be OK for filtering" << endl;
    }


    FFT orig(ds->x(), ds->y());
    orig.forward();
    if(transform) {
      soas().pushDataSet(orig.transform(ds));
      return;
    }
    orig.applyGaussianFilter(cutoff);
  
    for(int i = 0; i < derivatives; i++)
      orig.differentiate();
    orig.reverse();  // Don't use baseline on derivatives (for now)


    pusher << ds->derivedDataSet(orig.data,
                                 (derivatives ? 
                                  QString("_afft_diff_%1.dat").arg(derivatives) :
                                  "_afft.dat"));
  }
}

static ArgumentList 
afftOps(QList<Argument *>()
        << DataStackHelper::helperOptions()
        << DataSetList::listOptions("Datasets to filter")
        << new IntegerArgument("cutoff", 
                               "Cutoff",
                               "value of the cutoff")
        << new BoolArgument("transform", 
                            "Transform",
                            "if on, pushes the transform (off by default)")
        << new IntegerArgument("derive", 
                               "Derive",
                               "differentiate to the given order")
      );


static Command 
afft("auto-filter-fft", // command name
     effector(autoFilterFFTCommand), // action
     "filters",  // group name
     NULL, // arguments
     &afftOps, // options
     "Auto FFT",
     "Filter using FFT",
     "afft");

//////////////////////////////////////////////////////////////////////

static void findStepsCommand(const QString &, const CommandOptions & opts)
{
  double thresh = 0.1;
  int nb = 10;
  bool set = false;
  const GraphicsSettings & gs = soas().graphicsSettings();

  updateFromOptions(opts, "average", nb);
  updateFromOptions(opts, "threshold", thresh);
  updateFromOptions(opts, "set-segments", set);

  const DataSet * ds = soas().currentDataSet();
  QList<int> steps = ds->findSteps(nb, thresh);
  CurveView & view = soas().view();
  if(set) {
    DataSet * nds = new DataSet(*ds); // This is probably the
                                      // only legitimate use of the
                                      // copy constructor.
    nds->segments = steps;
    soas().pushDataSet(nds);
  }
  else {
    view.disableUpdates();
    for(int i = 0; i < steps.size(); i++) {
      Terminal::out << "Step #" << i << " @" << steps[i] 
                    << "\t X= " << ds->x()[steps[i]] <<endl;
      CurveVerticalLine * v = new CurveVerticalLine;
      v->x = 0.5* (ds->x()[steps[i]] + ds->x()[steps[i]-1]);
      v->pen =gs.getPen(GraphicsSettings::SeparationPen);

      view.addItem(v, true);
    }
    view.enableUpdates();
  }  
}

static ArgumentList 
fsOps(QList<Argument *>() 
      << new IntegerArgument("average", 
                            "Average over",
                            "Average over that many points")
      << new NumberArgument("threshold", 
                            "Threshold",
                            "Detection threshold")
      << new BoolArgument("set-segments", 
                          "Set segments",
                          "Whether or not to set the dataset segments")
      );
      
static Command 
fsc("find-steps", // command name
     effector(findStepsCommand), // action
     "segments",  // group name
     NULL, // arguments
     &fsOps, // options
     "Find steps",
     "Find steps in the data");

//////////////////////////////////////////////////////////////////////

static void clearSegmentsCommand(const QString &, const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  DataSet * nds = new DataSet(*ds); // This is probably the
                                      // only legitimate use of the
                                      // copy constructor.
  nds->segments.clear();
  soas().pushDataSet(nds);
}

      
static Command 
csc("clear-segments", // command name
     effector(clearSegmentsCommand), // action
     "segments",  // group name
     NULL, // arguments
     NULL, // options
     "Clear segments",
     "Clears all the segments");

//////////////////////////////////////////////////////////////////////


// Although this is not per se a data processing command, I guess it
// makes sense to leave it around here.

namespace __ss {

  typedef enum {
    AddSegment,
    RemoveSegment,
    DumpSegments,
    Accept,
    Abort
  } SetSegmentsActions;

  static EventHandler ssHandler = EventHandler("set-segments").
    addKey(Qt::Key_Escape, Abort, "abort").
    addKey('d', DumpSegments, "dump segments").
    alsoKey('D').
    conventionalAccept(Accept, "save segments").
    addClick(Qt::LeftButton, AddSegment, "add segment").
    addClick(Qt::RightButton, RemoveSegment, "remove segment").
    alsoKey('r').
    alsoKey('R');

static void setSegmentsCommand(CurveEventLoop &loop, const QString &,
                               const CommandOptions & /*opts*/)
{
  DataSet * ds = soas().currentDataSet();
  QList<int> savedSegments = ds->segments;

  loop.setHelpString("Set segments:\n"
                       + ssHandler.buildHelpString());

  do {
    int action = ssHandler.nextAction(loop);
    switch(action) {
    case Accept:
      if(ds->segments != savedSegments) {
        /// @hack This is ugly
        DataSet * nds = new DataSet(*ds);
        soas().pushDataSet(nds);
      }
      else
        Terminal::out << "Segments didn't change, not creating a new buffer" 
                      << endl;
      loop.terminate();
      break;
    case AddSegment:
    case RemoveSegment: {
      QPair<double, int> dst = loop.distanceToDataSet(ds);
      int idx = dst.second;
      const Vector & xv = ds->x();

      // Now, we refine the actual position so that the index is just
      // next to the x value.


      // Refine upwards/downwards
      double tg_x = loop.position().x();
      for(int j = 0; j < xv.size(); j++) {
        
        // First try up:
        double xl = xv.value(idx + j, xv.last());
        double xr = xv.value(idx + j + 1, xv.last());
        if(xl > xr)
          std::swap(xl,xr);
        if(xl <= tg_x && 
           tg_x <= xr) {
          idx = idx + j + 1;
          break;
        }

        // Same thing down
        xl = xv.value(idx - j - 1, xv.last());
        xr = xv.value(idx - j, xv.last());
        if(xl > xr)
          std::swap(xl,xr);
        if(xl <= tg_x && 
           tg_x <= xr) {
          idx = idx - j;
          break;
        }
      } // Should alway reach something fast.

      if(action == RemoveSegment) { // Remove 
        if(ds->segments.size() == 0);
        else if(idx <= ds->segments.first()) {
          ds->segments.takeFirst();
        }
        else if(idx >= ds->segments.last()) {
          ds->segments.takeLast();
        }
        else {
          double xv = loop.position().x();
          for(int i = 0; i < ds->segments.size() - 1; i++) {
            if(ds->segments[i] <= idx && ds->segments[i+1] >= idx) {
              // We're in !
              if(fabs(xv - ds->x()[ds->segments[i]]) <
                 fabs(ds->x()[ds->segments[i+1]] - xv))
                ds->segments.takeAt(i);
              else
                ds->segments.takeAt(i+1);
              break;
            }
          }
        }
      }
      else if(action == AddSegment) { // Add
        ds->segments.insert(idx);
      }
      
      Terminal::out << "Current segments: " << endl;
      for(int i = 0; i < ds->segments.size(); i++)
        Terminal::out << "Segment change #" << i << " @" << ds->segments[i] 
                      << "\t X= " << ds->x()[ds->segments[i]] <<endl;
      soas().view().invalidateCaches();
      break;
    }
    case DumpSegments:
      if(ds->segments.size() > 0) {
        QStringList cmd;
        cmd << "chop" << "/set-segments=true" << "/mode=indices";
        for(int i = 0; i < ds->segments.size(); i++)
          cmd << QString::number(ds->segments[i]);
        Terminal::out << "Segments can be set again using:\n" 
                      << cmd.join(" ") << endl;
      }
      break;
    case Abort:
        loop.terminate();
        break;
    default:
      ;
    }
  } while(! loop.finished());

  // Whatever happened, we restore the segments
  ds->segments = savedSegments;
}



static Command 
ssc("set-segments", // command name
    effector(setSegmentsCommand), // action
    "segments",  // group name
    NULL, // arguments
    NULL, // options
    "Set segments",
    "Set segments in the data (manually)");

};


//////////////////////////////////////////////////////////////////////

static void removeSpikesCommand(const QString &, const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  int nb = 2;                   // default to 2
  double extra = 3;
  bool force = false;

  updateFromOptions(opts, "number", nb);
  updateFromOptions(opts, "factor", extra);
  updateFromOptions(opts, "force-new", force);

  DataSet * newDs = ds->removeSpikes(nb, extra);
  if(newDs)
    soas().pushDataSet(newDs);
  else if(force)
    soas().pushDataSet(new DataSet(*ds));
}

static ArgumentList 
rsOps(QList<Argument *>() 
      << new IntegerArgument("number", 
                             "Number",
                             "looks at that many points")
      << new NumberArgument("factor", 
                            "Factor",
                            "threshold factor")
      << new BoolArgument("force-new", 
                          "Force new buffer",
                          "creates a new dataset even if no spikes were removed (default: false)")
      );


static Command 
rs("remove-spikes", // command name
   effector(removeSpikesCommand), // action
   "buffer",  // group name
   NULL, // arguments
   &rsOps, // options
   "Remove spikes",
   "Remove spikes",
   "R");

//////////////////////////////////////////////////////////////////////

static void intCommand(const QString &, const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  int idx = -1;
  updateFromOptions(opts, "index", idx);
  Vector in = Vector::integrateVector(ds->x(), ds->y(), idx > 0 ? idx : 0);
  soas().pushDataSet(ds->derivedDataSet(in, "_int.dat"));
}


static ArgumentList 
intOps(QList<Argument *>() 
      << new IntegerArgument("index", 
                             "Index",
                             "index of the point that should be used as y = 0")
      );

static Command 
integ("integrate", // command name
     effector(intCommand), // action
     "math",  // group name
     NULL, // arguments
     &intOps, // options
     "Integrate",
     "Integrate the buffer");

//////////////////////////////////////////////////////////////////////

static void diffCommand(const QString &, const CommandOptions &opts)
{
  const DataSet * ds = soas().currentDataSet();
  int order = -1;
  int deriv = 1;
  updateFromOptions(opts, "order", order); 
  updateFromOptions(opts, "derivative", deriv);
  if(order < 0)
    soas().pushDataSet(ds->firstDerivative());
  else
    soas().pushDataSet(ds->arbitraryDerivative(deriv, order));
}

static ArgumentList 
diffOps(QList<Argument *>() 
        << new IntegerArgument("order", 
                               "Order",
                               "total order of the computation")
        << new IntegerArgument("derivative", 
                               "Derivative order",
                               "the number of the derivative to take, only valid together with the order option")
        );


static Command 
diff("diff", // command name
     effector(diffCommand), // action
     "math",  // group name
     NULL, // arguments
     &diffOps, // options
     "Derive",
     "4th order accurate first derivative");

//////////////////////////////////////////////////////////////////////

static void diff2Command(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  soas().pushDataSet(ds->secondDerivative());
}

static Command 
dif2("diff2", // command name
     optionLessEffector(diff2Command), // action
     "math",  // group name
     NULL, // arguments
     NULL, // options
     "Derive twice",
     "4th order accurate second derivative");

//////////////////////////////////////////////////////////////////////

namespace __deldp {

  typedef enum {
                DeletePoint,
                TogglePoints,
                SelectRegion,
                RemoveRegion,
                Quit,
                Abort
  } DeldpActions;

  static EventHandler deldpHandler = EventHandler("deldp").
    addClick(Qt::LeftButton, DeletePoint, "delete point/select corner").
    addKey('p', TogglePoints, "toggle display of points/lines").
    addKey('r', SelectRegion, "start/stop selecting regions").
    addClick(Qt::MidButton, RemoveRegion, "delete points in selected region").
    alsoKey('R').
    addKey('q', Quit, "quit pushing new dataset").
    alsoKey('Q').
    addKey(Qt::Key_Escape, Abort, "abort");
  

  static void deldpCommand(CurveEventLoop &loop, const QString &)
  {
    const DataSet * ds = soas().currentDataSet();
    const GraphicsSettings & gs = soas().graphicsSettings();


    CurveView & view = soas().view();

    CEHideAll ha(view.mainPanel());

    std::unique_ptr<DataSet> newds(ds->derivedDataSet(ds->y(), "_deldp.dat"));

    CurveRectangle r;
    view.addItem(&r);
    r.pen = QPen(Qt::DotLine); /// @todo customize this
    r.brush = QBrush(QColor(0,0,255,50)); // A kind of transparent blue
    r.hidden = true;

    // Number of selected points. If -1, we're not selecting
    int selectedPoints = -1;

    CurveDataSet d(newds.get());
    if(ds->options.isJaggy(ds)) {
      d.tryPaintLines = false;
      d.paintMarkers = true;
    }
    else {
      d.tryPaintLines = true;
      d.paintMarkers = false;
    }
    PointTracker t(&loop, newds.get());
    view.addItem(&d);
    d.pen = gs.getPen(GraphicsSettings::ResultPen);
    view.addItem(&t);

    t.size = 4;
    t.pen = QPen(Qt::NoPen);
    t.brush = QBrush(QColor(0,0,255,100));

    loop.setHelpString(QString("Delete points: \n")
                       + deldpHandler.buildHelpString());
    while(! loop.finished()) {
      t.processEvent();
      if(loop.type() == QEvent::MouseMove && selectedPoints >= 0) {
        QPointF pos = loop.position(view.mainPanel());
        if(selectedPoints == 0)
          soas().
            showMessage(QString("Selecting first corner... %1,%2").
                        arg(pos.x()).arg(pos.y()));
        else {
          r.p2 = pos;
          soas().
            showMessage(QString("Selecting second corner... %1,%2").
                        arg(pos.x()).arg(pos.y()));
        }
      }
      switch(deldpHandler.nextAction(loop)) {
      case Quit:
        soas().pushDataSet(newds.release());
        return;
      case Qt::Key_Escape:
        return;
      case SelectRegion:
        r.hidden = true;
        if(selectedPoints < 0) {
          t.hidden = true;
          selectedPoints = 0;
        }
        else {
          selectedPoints = -1;
          t.hidden = false;
        }
        break;
      case TogglePoints:
        if(d.tryPaintLines) {
          d.tryPaintLines = false;
          d.paintMarkers = true;
        }
        else {
          d.tryPaintLines = true;
          d.paintMarkers = false;
        }
        break;
      case RemoveRegion: 
        if(selectedPoints >= 0) {
          int i = 0;
          QRectF reg(r.p1, r.p2);
          reg = reg.normalized();
          while(i < newds->nbRows()) {
            QPointF pt(newds->x()[i], newds->y()[i]);
            if(reg.contains(pt)) {
              newds->removeRow(i);
              --i;
            }
            ++i;
          }
          selectedPoints = 0;
          d.setDirty();
        }
        break;
      case DeletePoint:
        if(selectedPoints >= 0) {
          r.hidden = false;
          if(selectedPoints == 0) {
            r.p1 = loop.position(view.mainPanel());
            r.p2 = r.p1;
            selectedPoints = 1;
          }
          else
            selectedPoints = 0;
        }
        else {
          r.hidden = true;
          if(t.lastIndex >= 0 && t.lastIndex < newds->nbRows()) {
            newds->removeRow(t.lastIndex);
            d.setDirty();
          }

        }
        break;
      default:
        ;
      }
    }
  }

  static Command 
  ddp("deldp", // command name
      optionLessEffector(deldpCommand), // action
      "buffer",  // group name
      NULL, // arguments
      NULL, // options
      "Deldp",
      "Manually remove points");
}

//////////////////////////////////////////////////////////////////////

static void normCommand(const QString &, const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  Vector y = ds->y();
  bool positive = true;
  updateFromOptions(opts, "positive", positive);
  QList<double> mapTo;
  updateFromOptions(opts, "map-to", mapTo);
  if(mapTo.isEmpty()) {
    double fact = 1/(positive ? y.max() : -y.min());
    y *= fact;
  }
  else {
    double ym = y.min();
    double yM = y.max();

    double ymT = mapTo[0];
    double yMT = mapTo[1];
    y -= ym;
    y *= (yMT - ymT)/(yM - ym);
    y += ymT;
  }

  
  soas().pushDataSet(ds->derivedDataSet(y, "_norm.dat"));
}

static ArgumentList 
normOps(QList<Argument *>() 
        << new BoolArgument("positive", "Positive",
                            "whether to normalize on positive "
                            "or negative values (default true)")
        << new SeveralNumbersArgument("map-to", "Map to segment",
                                      "Normalizes by mapping to the given "
                                      "segment", true, true, ":")
        );

static Command 
norm("norm", // command name
     effector(normCommand), // action
     "norm",  // group name
     NULL, // arguments
     &normOps, // options
     "Normalize",
     "Divides by the maximum");

//////////////////////////////////////////////////////////////////////

static void zeroCommand(const QString &, double val, 
                        const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  bool isX = testOption<QString>(opts, "axis", "x");
  Vector nx = ds->x();
  Vector ny = ds->y();
  
  if(isX) {
    int idx = ny.closestPoint(val);
    double off = nx[idx];
    nx -= off;
    Terminal::out << "Offset X values by " << -off << endl;
  }
  else {
    int idx = nx.closestPoint(val);
    double off = ny[idx];
    ny -= off;
    Terminal::out << "Offset Y values by " << -off << endl;
  }
  soas().pushDataSet(ds->derivedDataSet(ny, "_off.dat", nx));
}

static ArgumentList 
zeroArgs(QList<Argument *>() 
         << new NumberArgument("value", 
                               "Value"
                               "value at which the other axis should be 0")
         );

static ArgumentList 
zeroOps(QList<Argument *>() 
        << new ChoiceArgument(QStringList() << "x" << "y",
                              "axis", "Axis", 
                              "which axis is zero-ed (default y)",
                              false, "axis")
        );

static Command 
zerp("zero", // command name
     effector(zeroCommand), // action
     "norm",  // group name
     &zeroArgs, // arguments
     &zeroOps, // options
     "Makes 0",
     "Ensure that a given point has X or Y value equal to 0");

//////////////////////////////////////////////////////////////////////

static void autoCorrelationCommand(const QString &,
                                   const CommandOptions & /*opts*/)
{
  const DataSet * ds = soas().currentDataSet();

  FFT f(ds->x(), ds->y(), false);

  f.forward(false);

  int nb = f.frequencies();
  /// @todo Probably this should join FFT ?
  for(int i = 0; i < nb; i++) {
    double &re = f.real(i);
    double &im = f.imag(i);
    re = (re*re) + (im * im);
    im = 0;
  }
  f.reverse(false);
  f.data /= f.data.size();       // because the transform is n times
                                 // too large, and it is squared above

  // Now, we splice the output value as we know the output function to
  // be periodic but we'd like to have 0 in the middle ;-)

  int offset = f.data.size() - nb;
  Vector ny = f.data.mid(nb) + f.data.mid(0,nb);
  if(ny.size() != f.data.size())
    throw InternalError("blunder !");

  Vector nx;
  for(int i = 0; i < ds->x().size(); i++)
    nx << f.deltaX * (i-offset);

  soas().pushDataSet(ds->derivedDataSet(ny, "_corr.dat", nx));
}

static Command 
ac("auto-correlation", // command name
   effector(autoCorrelationCommand), // action
   "buffer",  // group name
   NULL, // arguments
   NULL, // options
   "Auto-correlation",
   "Computes the auto-correlation function",
   "ac");

//////////////////////////////////////////////////////////////////////

static void binCommand(const QString &,
                       const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  int boxes = std::max(ds->nbRows()/15, 10);
  updateFromOptions(opts, "boxes", boxes);
  int col = 1;
  ColumnSpecification::updateFromOptions(opts, "column", col, ds);
  int weight = -1;
  ColumnSpecification::updateFromOptions(opts, "weight", weight, ds);
  
  bool lg = false;
  updateFromOptions(opts, "log", lg);
  bool nrm = false;
  updateFromOptions(opts, "norm", nrm);

  double min = std::nan("NaN");
  updateFromOptions(opts, "min", min);
  double max = std::nan("NaN");
  updateFromOptions(opts, "max", max);

  Vector w;
  if(weight >= 0)
    w = ds->column(weight);
  
  DataSet * nds = ds->derivedDataSet(ds->column(col).bin(boxes, lg, w, nrm,
                                                         min, max), 
                                     "_binned.dat");
  nds->options.histogram = true;
  soas().pushDataSet(nds);
}

static ArgumentList 
binOpts(QList<Argument *>()
        << new NumberArgument("min",
                              "Minimum",
                              "Minimum value of the histogram, overrides the minimum of the values in the data")
        << new NumberArgument("max",
                              "Maximum",
                              "Maximum value of the histogram, overrides the maximum of the values in the data")
        << new IntegerArgument("boxes", 
                              "Boxes"
                              "Number of bin boxes")
        << new ColumnArgument("column", 
                              "Column"
                              "Which column to bin")
        << new BoolArgument("log", 
                            "Log scale"
                            "Take the log10 before binning")
        << new BoolArgument("norm", 
                            "Normalize"
                            "Normalizes the bins so that the overall sum is 1")
        << new ColumnArgument("weight", 
                              "Weight column"
                              "Use a column as weight")
        );

static Command bn("bin", // command name
    effector(binCommand), // action
    "buffer",  // group name
    NULL, // arguments
    &binOpts, // options
    "Bin",
    "Make histograms");



//////////////////////////////////////////////////////////////////////

static void downsampleCommand(const QString &,
                              const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  int factor = 10;
  updateFromOptions(opts, "factor", factor);

  QList<Vector> cols;
  for(int i = 0; i < ds->nbColumns(); i++)
    cols << ds->column(i).downSample(factor);
  soas().pushDataSet(ds->derivedDataSet(cols, "_ds.dat"));
}


static ArgumentList 
dsOpts(QList<Argument *>() 
       << new IntegerArgument("factor", 
                              "Factor",
                              "Downsampling factor")
       );

static Command 
dsc("downsample", // command name
    effector(downsampleCommand), // action
    "buffer",  // group name
    NULL, // arguments
    &dsOpts, // options
    "Downsample",
    "Decrease the number of points in a dataset");

//////////////////////////////////////////////////////////////////////

static void dxCommand(const QString &,
                      const CommandOptions & /*opts*/)
{
  const DataSet * ds = soas().currentDataSet();
  const Vector & x = ds->x();
  Vector nx, dx;
  for(int i = 1; i < x.size(); i++) {
    nx << x[i-1];
    dx << x[i] - x[i-1];
  }
  QList<Vector> cols;
  cols << nx << dx;

  soas().pushDataSet(ds->derivedDataSet(cols, "_dx.dat"));
}


static Command 
dxC("dx", // command name
    effector(dxCommand), // action
    "math",  // group name
    NULL, // arguments
    NULL, // options
    "DX",
    "Replace Y by delta X");

//////////////////////////////////////////////////////////////////////

static void dyCommand(const QString &,
                      const CommandOptions & /*opts*/)
{
  const DataSet * ds = soas().currentDataSet();
  const Vector & x = ds->x();
  const Vector & y = ds->y();
  Vector nx, dy;
  for(int i = 1; i < x.size(); i++) {
    nx << x[i-1];
    dy << y[i] - y[i-1];
  }
  QList<Vector> cols;
  cols << nx << dy;

  soas().pushDataSet(ds->derivedDataSet(cols, "_dy.dat"));
}


static Command 
dyC("dy", // command name
    effector(dyCommand), // action
    "math",  // group name
    NULL, // arguments
    NULL, // options
    "DY",
    "Replace Y by delta Y");

//////////////////////////////////////////////////////////////////////


// These filters fail to build on vanilla 2.5 arches because of a bug
// in the GSL, fixed in:
//
// https://savannah.gnu.org/bugs/?54921
// 
// Black magic here to work around the bug
#define delete delete_dummy
#include <gsl/gsl_filter.h>
#undef delete

// Series of kernel based filters

enum Kernels {
              Gaussian,
              Median,
              RecursiveMedian,
              ImpulseMAD,
              ImpulseIQR,
              ImpulseSN,
              ImpulseQN
};

QHash<QString, Kernels> kernels =
  { {"gaussian", Gaussian},
    {"median", Median},
    {"rmedian", RecursiveMedian},
    {"impulse-mad", ImpulseMAD},
    {"impulse-iqr", ImpulseIQR},
    {"impulse-sn", ImpulseSN},
    {"impulse-qn", ImpulseQN}
  };

static void kernelFilterCommand(const QString &,
                                const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  const Vector & x = ds->x();
  const Vector & y = ds->y();
  Vector ny = y;
  Kernels ker = Gaussian;
  updateFromOptions(opts, "type", ker);

  int halfWidth = 5;
  updateFromOptions(opts, "size", halfWidth);

  double alpha = 2;
  updateFromOptions(opts, "alpha", alpha);

  int derivOrder = 0;
  // updateFromOptions(opts, "derive", derivOrder);

  double threshold = 0.5;
  updateFromOptions(opts, "threshold", threshold);

  gsl_filter_end_t endType = GSL_FILTER_END_PADVALUE;

  QString name = "???";
  for(const QString & n : ::kernels.keys()) {
    if(::kernels[n] == ker) {
      name = n;
      break;
    }
  }

  Terminal::out << "Using filter " << name << " with kernel half-width "
                << halfWidth << endl;

  switch(ker) {
  case Gaussian: {
    gsl_filter_gaussian_workspace *gauss_p =
      gsl_filter_gaussian_alloc(halfWidth);
    gsl_filter_gaussian(endType, alpha, derivOrder,
                        y, ny, gauss_p);

    gsl_filter_gaussian_free(gauss_p);
    break;
  }
  case Median: {
    gsl_filter_median_workspace * w = gsl_filter_median_alloc(halfWidth);
    gsl_filter_median(endType, y, ny, w);
    gsl_filter_median_free(w);
    break;
  }
  case RecursiveMedian: {
    gsl_filter_rmedian_workspace * w = gsl_filter_rmedian_alloc(halfWidth);
    gsl_filter_rmedian(endType, y, ny, w);
    gsl_filter_rmedian_free(w);
    break;
  }
  case ImpulseMAD:
  case ImpulseIQR: 
  case ImpulseSN: 
  case ImpulseQN: {
    gsl_filter_scale_t scl = GSL_FILTER_SCALE_MAD;
    switch(ker) {
    case ImpulseMAD:
      scl = GSL_FILTER_SCALE_MAD;
      break;
    case ImpulseIQR:
      scl = GSL_FILTER_SCALE_IQR;
      break;
    case ImpulseSN:
      scl = GSL_FILTER_SCALE_SN;
      break;
    case ImpulseQN:
      scl = GSL_FILTER_SCALE_QN;
      break;
    }
    gsl_filter_impulse_workspace * w = gsl_filter_impulse_alloc(halfWidth);
    size_t nb = 0;
    Vector ym = y, ys = y;

    gsl_filter_impulse(endType, scl, threshold, y, ny,
                       ym, ys, &nb, NULL, w);
    // gsl_vector *xmedian, gsl_vector *xsigma, size_t *noutlier, gsl_vector_int *ioutlier, gsl_filter_impulse_workspace *w)
    gsl_filter_impulse_free(w);
    break;
  }
  default:
    break;
  }
  
  soas().pushDataSet(ds->derivedDataSet(ny, "_" + name + ".dat"));
}

static ArgumentList 
kfOpts(QList<Argument *>() 
       << new IntegerArgument("size", 
                              "Size",
                              "Half window size")
       << new NumberArgument("alpha", 
                             "Alpha",
                             "Gaussian spread (only for gaussian)")
       << new NumberArgument("threshold", 
                             "Theshold",
                             "Threshold for impulse filters")
       << new TemplateChoiceArgument<Kernels>(::kernels,
                                             "type",
                                             "Type",
                                             "Kernel type")
       // << new IntegerArgument("derive", 
       //                        "Derivative over",
       //                        "Order of derivative (only for gaussian)")
       );


static Command 
kernFilter("kernel-filter", // command name
           effector(kernelFilterCommand), // action
           "math",  // group name
           NULL, // arguments
           &kfOpts, // options
           "Kernel filter",
           "Filters data using a kernel");


//////////////////////////////////////////////////////////////////////

/// @todo Convert to multibuffer later on
static void averageDupsCommand(const QString &,
                               const CommandOptions& opts)
{
  const DataSet * ds = soas().currentDataSet();

  ColumnListSpecification columns;
  updateFromOptions(opts, "columns", columns);
  double tolerance = 0;
  updateFromOptions(opts, "tolerance", tolerance);  

  QList<Vector> base, sums, errors;
  Vector numbers;

  base = ds->allColumns();         // to complement later
  QStringList oldNames = ds->mainColumnNames();
  if(columns.columns.size() > 0) {
    QList<int> nc = columns.getValues(ds);
    QStringList nn;
    QList<Vector> nb;
    for(int i : nc) {
      nb << base[i];
      nn << oldNames[i];
    }
    base = nb;
    oldNames = nn;
  }

  for(const Vector & v : base)
    sums << Vector();
  errors = sums;

  const Vector & x = base.first();
  for(int i = 0; i < x.size(); i++) {
    double xv = x[i];
    int idx = 0;
    for(; idx < numbers.size(); idx++) {
      double cv = sums[0][idx]/numbers[idx];
      if(fabs(cv - xv) <= tolerance * fabs(xv))
        break;
    }
    if(idx == numbers.size()) {
      for(Vector & v : sums)
        v << 0;
      for(Vector & v : errors)
        v << 0;
      numbers << 0;
    }
    numbers[idx] += 1;
    for(int j = 0; j < base.size(); j++) {
      double v = base[j][i];
      sums[j][idx] += v;
      errors[j][idx] += v*v;
    }
  }
  
  for(int j = 0; j < base.size(); j++) {
    sums[j] /= numbers;
    Vector s2 = sums[j];
    errors[j] /= numbers;
    s2 *= s2;
    errors[j] -= s2;
    for(double & v : errors[j])
      v = sqrt(v);
  }

  QList<Vector> cols;
  QStringList names;
  for(int j = 0; j < base.size(); j++) {
    cols << sums[j] << errors[j];
    names << oldNames[j] << oldNames[j] + "_err";
  }
  cols << numbers;
  names << "number";
  DataSet * nds = ds->derivedDataSet(cols, "_avgd.dat");
  nds->setColumnName(0, "dummy");
  nds->columnNames[0] = names;
  soas().pushDataSet(nds);
}

static ArgumentList 
avgdOps(QList<Argument *>()
        << new SeveralColumnsArgument("columns", "columns", "Column to be averaged (defaults to all)")
        << new NumberArgument("tolerance", "tolerance",
                              "Tolerance in comparing the X values (defaults to 0)")
        );


static Command 
avgd("average-duplicates", // command name
     effector(averageDupsCommand), // action
     "math",  // group name
     NULL, // arguments
     &avgdOps, // options
     "Average duplicates");


//////////////////////////////////////////////////////////////////////

static void convolveCommand(const QString &,
                            DataSet * ds1,
                            DataSet * kernel,
                            const CommandOptions& opts)
{
  Terminal::out << "Convolving dataset " << ds1->name
                << " with kernel " << kernel->name << endl;
  Vector ny = ds1->convolveWith(kernel);
  DataSet * nds = ds1->derivedDataSet(ny, "_conv.dat");
  soas().pushDataSet(nds);
}

static ArgumentList 
convArgs(QList<Argument *>()
         << new DataSetArgument("dataset", "dataset", "the dataset to convolve")
         << new DataSetArgument("kernel", "kernel", "the convolution kernel")
         );


static Command 
conv("convolve", // command name
     effector(convolveCommand), // action
     "math",  // group name
     &convArgs, // arguments
     NULL,
     "Convolve");
