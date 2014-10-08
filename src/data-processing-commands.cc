/**
   \file data-processing-commands.cc
   Commands to extract information from datasets
   Copyright 2011 by Vincent Fourmond
             2011,2013, 2014 by CNRS/AMU

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

#include <peaks.hh>
#include <idioms.hh>
#include <curve-effectors.hh>

#include <msolver.hh>

//////////////////////////////////////////////////////////////////////

namespace __reg {
  typedef enum {
    LeftPick,
    RightPick,
    Subtract,
    Divide,
    Write,
    Exponential,
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
    addKey('v', Divide, "divide by trend").
    alsoKey('V');

  static void reglinCommand(CurveEventLoop &loop, const QString &)
  {
    const DataSet * ds = soas().currentDataSet();
    const GraphicsSettings & gs = soas().graphicsSettings();

    CurveLine line;
    CurveHorizontalRegion r;
    CurveView & view = soas().view();
    CurvePanel bottom;
    CurveData d;
    bottom.drawingXTicks = false;
    bottom.stretch = 30;        // 3/10ths of the main panel.
    view.addItem(&line);
    view.addItem(&r);
    bottom.addItem(&d);

    bottom.yLabel = Utils::deltaStr("Y");
    d.countBB = true;


    view.addPanel(&bottom);
    line.pen = gs.getPen(GraphicsSettings::ReglinPen);
    d.pen = gs.getPen(GraphicsSettings::ResultPen);
    QPair<double, double> reg;
    double xleft = ds->x().min();
    double xright = ds->x().max();

    // Computed fields:
    double decay_rate = 0;


    loop.setHelpString(QString("Linear regression:\n")
                       + reglinHandler.buildHelpString());

    Terminal::out << "Linear regression:\na\tb\tkeff\txleft\txright" 
                  << endl;

    /// @todo selection mode ? (do we need that ?)
    while(! loop.finished()) {
      switch(reglinHandler.nextAction(loop)) {
      case LeftPick:
      case RightPick: {
        r.setX(loop.position().x(), loop.button());
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
        }
        double dy_min = 0;
        double dy_max = 0;
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
        e << "a" << reg.first << "b" << reg.second 
          << "keff" << decay_rate << "xleft" << r.xleft 
          << "xright" << r.xright;
        OutFile::out.writeValueHash(e, ds, QString("Dataset: %1").
                                    arg(ds->name));
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
        /// @bug Wrong here !
        double rate = reg.first/(reg.second - reg.first*d.xvalues[0]);
        for(int i = 1; i < newy.size(); i++)
          newy[i] /= exp(rate * (d.xvalues[i] - d.xvalues[0]));
        DataSet * newds = ds->derivedDataSet(newy, "_expdiv.dat");
        soas().pushDataSet(newds);
        return;
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
      "...",
      "reg");
};

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
    alsoKey('K');


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

  bottom.yLabel = "Remaining coverage";
  d.countBB = true;


  view.addPanel(&bottom);
  line.pen = gs.getPen(GraphicsSettings::ReglinPen);
  d.pen = gs.getPen(GraphicsSettings::ResultPen);
  QPair<double, double> reg;
  double xleft = ds->x().min();
  double xright = ds->x().max();

  loop.setHelpString(QString("Film loss:\n")
                       + filmLossHandler.buildHelpString());
  bool needUpdate = false;

  int currentSegment = 0;
  zoomToSegment(ds, currentSegment, view);

  QVarLengthArray<double, 200> rateConstants(ds->segments.size() + 1);
  for(int i = 0; i <= ds->segments.size(); i++)
    rateConstants[i] = 0;

  while(! loop.finished()) {
    int action = filmLossHandler.nextAction(loop);
    switch(action) {
    case PickLeft:
    case PickRight: {
      r.setX(loop.position().x(), loop.button());
      reg = ds->reglin(r.xmin(), r.xmax());
      double y = reg.first * xleft + reg.second;
      line.p1 = QPointF(xleft, y);
      y = reg.first * xright + reg.second;
      line.p2 = QPointF(xright, y);
      double decay = 1/(-reg.second/reg.first - r.xleft);
      rateConstants[currentSegment] = decay;
      Terminal::out << "xleft = " << r.xleft << "\t" 
                    << "xright = " << r.xright << "\t" 
                    << reg.first << "\t" << reg.second 
                    << "\t" << decay << endl;
      soas().showMessage(QObject::tr("Regression between X=%1 and X=%2 "
                                     "(segment #%3)").
                         arg(r.xleft).arg(r.xright).arg(currentSegment + 1));
      needUpdate = true;
      break;
    }
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
      soas().pushDataSet(ds->derivedDataSet(corr.yvalues, "_corr.dat"));
      return;
    case QuitReplacing:
      soas().pushDataSet(ds->derivedDataSet(d.yvalues, "_coverage.dat"));
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
    "Corrects for film loss segment-by-segment",
    "...");

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

  /// The charge computed in the previous call
  /// @todo Make that more flexible ?
  static double oldCharge = 0;
  double charge = 0;
  // Make sure the charge is updated whenever we go out of scope ?
  DelayedAssign<double> chrg(oldCharge, charge);
  

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
      h.modified.yvalues = s.evaluate(h.diff.xvalues, type);
      if(computeDerivative)
        h.derivative.yvalues = s.derivative(h.diff.xvalues, type);

      h.updateBottom();
      charge = Vector::integrate(ds->x(), h.diff.yvalues);
      Terminal::out << "Charge: " << charge << " (old: " 
                    << oldCharge << ")" << endl;

      
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
    "...",
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
                                     "Base for X values")
              << new DataSetArgument("nodes", 
                                     "Nodes",
                                     "Interpolation nodes X/Y values"));

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
            "Recompute interpolation with given nodes",
            "...");




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
    "...",
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
      int weights = -1;
      updateFromOptions(opts, "weight-column", weights);
      if(weights >= 0) 
        splines.setWeights(ds->column(weights));
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
                            "Use the weights in the given column")
        );

  static Command 
  bspl("filter-bsplines", // command name
       effector(bsplinesCommand), // action
       "buffer",  // group name
       NULL, // arguments
       &bsOpts, // options
       "Filter",
       "Filter using bsplines",
       "...");
};

//////////////////////////////////////////////////////////////////////

namespace __fft {

  typedef enum {
    IncreaseSetCutoff,
    DecreaseCutoff,
    ToggleDerivative,
    ToggleBaseline,
    TogglePowerSpectrum,
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



  d.pen = gs.getPen(GraphicsSettings::ResultPen);
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
    spec1.xvalues[i] = log((i+1)/(1.0*spec1.xvalues.size()));
  spec1.yvalues = spec1.xvalues;
  spec1.countBB = true;
  spec1.histogram = true;
  spectrum.addItem(&spec1);

  spec2.pen = gs.getPen(GraphicsSettings::ResultPen);
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

  for(int i = 0; i < spec1.yvalues.size(); i++)
    spec1.yvalues[i] = log(orig.magnitude(i+1));

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
        cutoff = exp(-x);
      }
      else {                                 // +/- decrease.
        cutoff++;
        if(cutoff > ds->x().size()/2 - 2)
          cutoff = ds->x().size()/2 - 2;
      }
      Terminal::out << "Now using a cutoff of " << cutoff << endl;
      needCompute = true;
      break;
    case DecreaseCutoff:
      cutoff--;
      if(cutoff < 2)
        cutoff = 2;
      Terminal::out << "Now using a cutoff of " << cutoff << endl;
      needCompute = true;
      break;
    case Abort:
      return;
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
      }
      FFT trans = orig;
      lim.x = -log(cutoff);
      trans.applyGaussianFilter(cutoff);

      if(order > 0)             /// @todo and the second derivative ?
        trans.differentiate();

      if(showSpectrum) {        // We need to update the values
        for(int i = 0; i < spec2.yvalues.size(); i++)
          spec2.yvalues[i] = log(trans.magnitude(i+1));
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
        r.setTop(spec1.yvalues.min()-5);
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
    "buffer",  // group name
    NULL, // arguments
    &fftOps, // options
    "Filter",
    "Filter using FFT",
    "...");

}

//////////////////////////////////////////////////////////////////////


static void autoFilterBSCommand(const QString &, const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  int nb = 12;
  int order = 4;
  int derivatives = 0;
  int weights = -1;


  updateFromOptions(opts, "number", nb);
  updateFromOptions(opts, "order", order);
  updateFromOptions(opts, "derivatives", derivatives);
  updateFromOptions(opts, "weight-column", weights);

  BSplines splines(ds, order, derivatives);
  splines.autoBreakPoints(nb);
  if(weights >= 0) {
    splines.setWeights(ds->column(weights));
  }
  splines.optimize(15, false);
  double value = splines.computeCoefficients();
  Terminal::out << "Residuals: " << sqrt(value) << endl;
  for(int i = 0; i <= derivatives; i++)
    soas().
      pushDataSet(ds->derivedDataSet(splines.computeValues(i),
                                     (i ? QString("_afbs_diff_%1.dat").arg(i) :
                                      "_afbs.dat")));
}

static ArgumentList 
afbsOps(QList<Argument *>() 
      << new IntegerArgument("number", 
                             "Number",
                             "Number of segments")
      << new IntegerArgument("order", 
                             "Order",
                             "Order of the splines")
      << new ColumnArgument("weight-column", 
                            "Weights",
                            "Use the weights in the given column")
      << new IntegerArgument("derivatives", 
                             "Derivative order",
                             "Compute derivatives up to the given ")
      );


static Command 
afbs("auto-filter-bs", // command name
     effector(autoFilterBSCommand), // action
     "buffer",  // group name
     NULL, // arguments
     &afbsOps, // options
     "Filter",
     "Filter using bsplines",
     "...",
     "afbs");

//////////////////////////////////////////////////////////////////////


/// @todo the auto-filter-... commands should be all merged into one
/// (?) or at least provide a consistent interface.
static void autoFilterFFTCommand(const QString &, const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  int cutoff = 20;
  int derivatives = 0;

  updateFromOptions(opts, "cutoff", cutoff);
  updateFromOptions(opts, "derive", derivatives);

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
  orig.applyGaussianFilter(cutoff);
  
  for(int i = 0; i < derivatives; i++)
    orig.differentiate();
  orig.reverse();  // Don't use baseline on derivatives (for now)


  soas().
    pushDataSet(ds->derivedDataSet(orig.data,
                                   (derivatives ? 
                                    QString("_afft_diff_%1.dat").arg(derivatives) :
                                    "_afft.dat")));
}

static ArgumentList 
afftOps(QList<Argument *>() 
        << new IntegerArgument("cutoff", 
                               "Number",
                               "Number of segments")
        << new IntegerArgument("derive", 
                               "Derive",
                               "Differentiate to the given order")
      );


static Command 
afft("auto-filter-fft", // command name
     effector(autoFilterFFTCommand), // action
     "buffer",  // group name
     NULL, // arguments
     &afftOps, // options
     "Filter",
     "Filter using bsplines",
     "...",
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
      CurveVerticalLine * v= new CurveVerticalLine;
      v->x = 0.5* (ds->x()[steps[i]] + ds->x()[steps[i]-1]);
      v->pen =gs.getPen(GraphicsSettings::SeparationPen);

      view.addItem(v);
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
     "buffer",  // group name
     NULL, // arguments
     &fsOps, // options
     "Find steps",
     "Find steps in the data",
     "...");

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
    "buffer",  // group name
    NULL, // arguments
    NULL, // options
    "Set segments",
    "Set segments in the data (manually)",
    "...");

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
                             "Number of points to look at")
      << new NumberArgument("factor", 
                            "Factor",
                            "...")
      << new BoolArgument("force-new", 
                          "Force new buffer",
                          "Adds a new buffer even if no spikes were removed")
      );


static Command 
rs("remove-spikes", // command name
   effector(removeSpikesCommand), // action
   "buffer",  // group name
   NULL, // arguments
   &rsOps, // options
   "Remove spikes",
   "Remove spikes",
   "Remove spikes using a simple heuristics",
   "R");

//////////////////////////////////////////////////////////////////////

static void diffCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  soas().pushDataSet(ds->firstDerivative());
}

static Command 
diff("diff", // command name
     optionLessEffector(diffCommand), // action
     "buffer",  // group name
     NULL, // arguments
     NULL, // options
     "Derive",
     "4th order accurate first derivative",
     "Computes the 4th order accurate derivative of the buffer\n"
     "Do not use this on noisy data");

//////////////////////////////////////////////////////////////////////

static void diff2Command(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  soas().pushDataSet(ds->secondDerivative());
}

static Command 
dif2("diff2", // command name
     optionLessEffector(diff2Command), // action
     "buffer",  // group name
     NULL, // arguments
     NULL, // options
     "Derive",
     "4th order accurate second derivative",
     "Computes the 4th order accurate second derivative of the buffer\n"
     "Do not use this on noisy data !");


//////////////////////////////////////////////////////////////////////

static void displayPeaks(QList<PeakInfo> peaks, const DataSet * ds, 
                         int maxnb = -1, bool write = true)
{
  const GraphicsSettings & gs = soas().graphicsSettings();
  CurveView & view = soas().view();
  view.disableUpdates();
  Terminal::out << "Found " << peaks.size() << " peaks" << endl;
  if(write) {
    Terminal::out << "Writing to output file " 
                  << OutFile::out.fileName() << endl;
  }
  if(maxnb < 0 || maxnb > peaks.size())
    maxnb = peaks.size();
  for(int i = 0; i < maxnb; i++) {
    ValueHash hsh;
    hsh << "buffer" << ds->name 
        << "what" << (peaks[i].isMin ? "min" : "max" )
        << "x" << peaks[i].x << "y" << peaks[i].y 
        << "index" << peaks[i].index;
    if(write)
      OutFile::out.writeValueHash(hsh, ds);
    Terminal::out << hsh.toString() << endl;
    CurveLine * v= new CurveLine;
    
    v->p1 = QPointF(peaks[i].x, 0);
    v->p2 = QPointF(peaks[i].x, peaks[i].y);
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
  updateFromOptions(opts, "output", write);

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
  displayPeaks(peaks, ds, nb, write);
}


static ArgumentList 
fpBaseOps(QList<Argument *>() 
          << new IntegerArgument("window", 
                                 "Peak window",
                                 "...")
          << new BoolArgument("include-borders",
                              "Whether or not to include borders",
                              "...")
          << new ChoiceArgument(QStringList() 
                                << "min" << "max" << "both",
                                "which",
                                "",
                                "...")
          << new BoolArgument("output", 
                              "Write to output file",
                              "Whether peak information should be written to output file by default")
       );

static ArgumentList 
fpOps(QList<Argument *>(fpBaseOps) 
      << new IntegerArgument("peaks", 
                            "Number of peaks",
                            "Display only that many peaks (by order of intensity)")
      );
      
static Command 
fp("find-peaks", // command name
   effector(findPeaksCommand), // action
   "buffer",  // group name
   NULL, // arguments
   &fpOps, // options
   "Find peaks",
   "Find all peaks",
   "...");

static Command 
fp1("1", // command name
   effector(findPeaksCommand), // action
   "buffer",  // group name
    NULL, // arguments
    &fpBaseOps, // options
    "Find peak",
    "Find the biggest peak",
    "...");

static Command 
fp2("2", // command name
   effector(findPeaksCommand), // action
   "buffer",  // group name
    NULL, // arguments
    &fpBaseOps, // options
    "Find two peaks",
    "Find the two biggest peaks",
    "...");

//////////////////////////////////////////////////////////////////////

static void echemPeaksCommand(const QString &, const CommandOptions &)
{
  int window = 8;

  const DataSet * ds = soas().currentDataSet();
  const GraphicsSettings & gs = soas().graphicsSettings();
  Peaks pk(ds, window);

  QList<EchemPeakPair> pairs = pk.findPeakPairs();
  {
    CurveView & view = soas().view();
    view.disableUpdates();
    Terminal::out << "Found " << pairs.size() << " peaks pairs" << endl;

    for(int i = 0; i < pairs.size(); i++) {
      QString str = QString("%1\t%2\t%3").
        arg(ds->name).arg(pairs[i].forward.x).arg(pairs[i].forward.y);
      CurveLine * v = new CurveLine;
    
      v->p1 = QPointF(pairs[i].forward.x, 0);
      v->p2 = QPointF(pairs[i].forward.x, pairs[i].forward.y);
      v->pen = gs.getPen(GraphicsSettings::PeaksPen);
      view.addItem(v);

      if(pairs[i].isReversible()) {
        str += QString("\t%1\t%2\t%3\t%4\t%5").
          arg(pairs[i].backward.x).arg(pairs[i].backward.y).
          arg(pairs[i].deltaX()).
          arg(pairs[i].deltaY()).
          arg(pairs[i].midX());

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
      }

      Terminal::out << str << endl;

    }
    view.enableUpdates();
  }
}

      
static Command 
ep("echem-peaks", // command name
   effector(echemPeaksCommand), // action
   "buffer",  // group name
   NULL, // arguments
   //&fpOps
   NULL, // options
   "Find peaks pairs",
   "Find all peaks pairs",
   "...");

//////////////////////////////////////////////////////////////////////

static void deldpCommand(CurveEventLoop &loop, const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  const GraphicsSettings & gs = soas().graphicsSettings();


  CurveView & view = soas().view();

  CEHideAll ha(view.mainPanel());

  DataSet * newds = ds->derivedDataSet(ds->y(), "_deldp.dat");

  CurveDataSet d(newds);
  PointTracker t(&loop, newds);
  view.addItem(&d);
  d.pen = gs.getPen(GraphicsSettings::ResultPen);
  view.addItem(&t);

  t.size = 4;
  t.pen = QPen(Qt::NoPen);
  t.brush = QBrush(QColor(0,0,255,100));

  loop.setHelpString(QObject::tr("..."));
  while(! loop.finished()) {
    t.processEvent();
    if(loop.isConventionalAccept()) {
      soas().pushDataSet(newds);
      return;
    }
    switch(loop.type()) {
    case QEvent::MouseButtonPress: 
      if(loop.button() == Qt::LeftButton) {
        if(t.lastIndex >= 0 && t.lastIndex < newds->nbRows())
          newds->removePoint(t.lastIndex);
      }
      
      break;
    case QEvent::KeyPress: 
      switch(loop.key()) {
      case Qt::Key_Escape:
        delete newds;
        return;
      }
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
    "...",
    "...");

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
                            "Whether we normalize on positive "
                            "or negative values")
        << new SeveralNumbersArgument("map-to", "Map to segment",
                                      "Normalizes by mapping to the given "
                                      "segment", true, true, ":")
        );

static Command 
norm("norm", // command name
     effector(normCommand), // action
     "buffer",  // group name
     NULL, // arguments
     &normOps, // options
     "Normalize",
     "Divides by the maximum",
     "Normalize the current buffer by its maximum value");

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
                               "Value at which the other axis should be 0")
         );

static ArgumentList 
zeroOps(QList<Argument *>() 
        << new ChoiceArgument(QStringList() << "x" << "y",
                              "axis", "Axis", 
                              "Which axis is zero-ed", false, "axis")
        );

static Command 
zerp("zero", // command name
     effector(zeroCommand), // action
     "buffer",  // group name
     &zeroArgs, // arguments
     &zeroOps, // options
     "Makes 0",
     "Ensure that a given point has X or Y value equal to 0",
     "...");

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
   "Computes the auto-correlation function", "", "ac");

//////////////////////////////////////////////////////////////////////

static void binCommand(const QString &,
                       const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  int boxes = std::max(ds->nbRows()/15, 10);
  updateFromOptions(opts, "boxes", boxes);
  int col = 1;
  updateFromOptions(opts, "column", col);
  bool lg = false;
  updateFromOptions(opts, "log", lg);
  DataSet * nds = ds->derivedDataSet(ds->column(col).bin(boxes, lg), 
                                     "_binned.dat");
  nds->options.histogram = true;
  soas().pushDataSet(nds);
}

static ArgumentList 
binOpts(QList<Argument *>() 
        << new IntegerArgument("boxes", 
                              "Boxes"
                              "Number of bin boxes")
        << new ColumnArgument("column", 
                              "Column"
                              "Which column to bin")
        << new BoolArgument("log", 
                            "Log scale"
                            "Take the log10 before binning")
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
ds("downsample", // command name
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
dx("dx", // command name
    effector(dxCommand), // action
    "buffer",  // group name
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
dy("dy", // command name
    effector(dyCommand), // action
    "buffer",  // group name
    NULL, // arguments
    NULL, // options
    "DY",
    "Replace Y by delta Y");
