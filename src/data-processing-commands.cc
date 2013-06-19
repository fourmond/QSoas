/**
   \file data-processing-commands.cc
   Commands to extract information from datasets
   Copyright 2011, 2013 by Vincent Fourmond

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

#include <dataset.hh>
#include <soas.hh>
#include <curveview.hh>
#include <curveeventloop.hh>
#include <curvemarker.hh>
#include <curvedataset.hh>
#include <curveitems.hh>
#include <curvepanel.hh>
#include <math.h>

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

  static EventHandler reglinHandler = EventHandler("reg").
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
        decay_rate = reg.first/(reg.second + reg.first * r.xleft);
        Terminal::out << reg.first << "\t" << reg.second 
                      << "\t" << decay_rate << endl;
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
        DataSet * newds = new 
          DataSet(QList<Vector>() << d.xvalues << d.yvalues);
        newds->name = ds->cleanedName() + "_linsub.dat";
        soas().pushDataSet(newds);
        return;
      }
      case Write: {
        OutFile::out.setHeader(QString("Dataset: %1\n"
                                       "a\tb\tkeff\txleft\txright").
                               arg(ds->name));
        /// @todo add other fields ? This should be done through an
        /// appropriate class, as far as I can tell.
        OutFile::out << reg.first << "\t" << reg.second 
                     << "\t" << decay_rate
                     << "\t" << r.xleft << "\t" << r.xright 
                     << "\n" << flush;
        Terminal::out << "Writing to output file " << OutFile::out.fileName()
                      << endl;
        break;
      }
      case Divide: {
        // Dividing
        Vector newy = ds->y();
        for(int i = 0; i < newy.size(); i++)
          newy[i] /= (d.xvalues[i] * reg.first + reg.second);
        DataSet * newds = new 
          DataSet(QList<Vector>() << d.xvalues << newy);
        newds->name = ds->cleanedName() + "_lindiv.dat";
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
        DataSet * newds = new 
          DataSet(QList<Vector>() << d.xvalues << newy);
        newds->name = ds->cleanedName() + "_expdiv.dat";
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


  loop.setHelpString(QObject::tr("Film loss:\n"
                                 "left click: left boundary\n"
                                 "right click: right boundary\n"
                                 "space, n: jump to next\n"
                                 "z: zoom on current segment\n"
                                 "k: enter rate constant manually\n"
                                 "b: back to previous segment\n"
                                 "r: replace buffer by coverage\n"
                                 "q: quit dividing by coverage"));

  int currentSegment = 0;
  zoomToSegment(ds, currentSegment, view);

  QVarLengthArray<double, 200> rateConstants(ds->segments.size() + 1);
  for(int i = 0; i <= ds->segments.size(); i++)
    rateConstants[i] = 0;
      

  while(! loop.finished()) {
    switch(loop.type()) {
    case QEvent::MouseButtonPress: {
      r.setX(loop.position().x(), loop.button());
      reg = ds->reglin(r.xleft, r.xright);
      double y = reg.first * xleft + reg.second;
      line.p1 = QPointF(xleft, y);
      y = reg.first * xright + reg.second;
      line.p2 = QPointF(xright, y);
      double decay = 1/(-reg.second/reg.first - r.xleft);
      rateConstants[currentSegment] = decay;
      Terminal::out << reg.first << "\t" << reg.second 
                    << "\t" << decay << endl;
      soas().showMessage(QObject::tr("Regression between X=%1 and X=%2 "
                                     "(segment #%3)").
                         arg(r.xleft).arg(r.xright).arg(currentSegment + 1));

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

      break;
    }
    case QEvent::KeyPress: 
      switch(loop.key()) {
      case 'z':
      case 'Z':
        zoomToSegment(ds, currentSegment, view);
        break;

      case 'q':
      case 'Q': {
        soas().pushDataSet(ds->derivedDataSet(corr.yvalues, "_corr.dat"));
        return;
      }
      case 'r':
      case 'R': {
        soas().pushDataSet(ds->derivedDataSet(d.yvalues, "_coverage.dat"));
        return;
      }
      case Qt::Key_Escape:
        return;
      case ' ':
      case 'n':
      case 'N': {
        currentSegment += 1;
        if(currentSegment > ds->segments.size())
          currentSegment = ds->segments.size();
        else
          zoomToSegment(ds, currentSegment, view);
        break;
      }
      case 'b':
      case 'B': {
        currentSegment -= 1;
        if(currentSegment < 0)
          currentSegment = 0;
        else
          zoomToSegment(ds, currentSegment, view);
        break;
      }
      default:
        ;
      }
      break;
    default:
      ;
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

//////////////////////////////////////////////////////////////////////


namespace __bl {
typedef enum {
  AddPoint,
  RemovePoint,
  CycleSplineType,
  Add10Left,
  Add10Right,
  Add20Everywhere,
  ClearAll,
  Derive,
  Replace,
  Divide,
  HideDataset,
  Subtract,
  Abort
} BaselineActions;

static EventHandler baselineHandler = EventHandler("baseline").
  addClick(Qt::LeftButton, AddPoint, "place marker").
  addClick(Qt::RightButton, RemovePoint, "remove marker").
  addKey('q', Subtract, "subtract baseline").
  alsoKey('Q').
  alsoClick(Qt::MiddleButton).
  addKey('v', Divide, "divide by baseline").
  alsoKey('V').
  addKey('u', Replace, "replace by baseline").
  alsoKey('U').
  addKey('d', Derive, "display derivative").
  alsoKey('D').
  addKey('h', HideDataset, "hide dataset").
  alsoKey('H').
  addKey('1', Add10Left, "add 10 to the left").
  addKey('2', Add10Right, "add 10 to the right").
  addKey('0', Add20Everywhere, "add 20 regularly spaced").
  addKey('t', CycleSplineType, "cycle interpolation types").
  alsoKey('T').
  addKey(Qt::Key_Escape, Abort, "abort");
  

static void baselineCommand(CurveEventLoop &loop, const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  const GraphicsSettings & gs = soas().graphicsSettings();

  /// The charge computed in the previous call
  /// @todo Make that more flexible ?
  static double oldCharge = 0;
  double charge = 0;
  // Make sure the charge is updated whenever we go out of scope ?
  DelayedAssign<double> chrg(oldCharge, charge);
  

  CurveView & view = soas().view();
  CurveMarker m;
  CurvePanel bottom;
  Spline s;
  CurveData d;
  CurveData diff;
  bool derive = false;
  bottom.drawingXTicks = false;
  bottom.stretch = 30;        // 3/10ths of the main panel.

  bottom.yLabel = Utils::deltaStr("Y");

  PointPicker pick(&loop, ds);

  CurveItem * cds = view.getCurrentDataSet();

  view.addItem(&m);
  view.addItem(&d);

  m.size = 4;
  m.pen = QPen(Qt::NoPen);
  m.brush = QBrush(QColor(0,0,255,100)); /// @todo customize that too 

  Spline::Type type = Spline::CSpline;

  d.pen = gs.getPen(GraphicsSettings::BaselinePen);
  d.xvalues = ds->x();
  d.yvalues = QVector<double>(d.xvalues.size(), 0);
  d.countBB = true;
  diff.xvalues = d.xvalues;
  diff.yvalues = ds->y();

  diff.countBB = true;

  bottom.addItem(&diff);

  view.addPanel(&bottom);

  loop.setHelpString("Spline interpolation:\n"
                     + baselineHandler.buildHelpString() + "\n" + 
                     pick.helpText());

  while(! loop.finished()) {
    bool needCompute = false;
    bool isLeft = false;     // Used for sharing code for the 1/2 actions.

    pick.processEvent();        // We don't filter actions out.

    switch(baselineHandler.nextAction(loop)) {
    case AddPoint: 
      s.insert(pick.point());
      needCompute = true;
      break;
    case RemovePoint:
      s.remove(loop.position().x());
      needCompute = true;
      break;
    case HideDataset:
      cds->hidden = ! cds->hidden;
      // m.hidden = cds->hidden; // (not needed)
      break;
    case Subtract:
      soas().pushDataSet(ds->derivedDataSet(diff.yvalues, "_bl_sub.dat"));
      return;
    case Divide: {
      // Dividing
      Vector newy = ds->y();
      newy /= d.yvalues;
      soas().pushDataSet(ds->derivedDataSet(newy, "_bl_div.dat"));
      return;
    }
    case Replace: {
      // Replacing the data is already computed in d
      soas().
        pushDataSet(ds->derivedDataSet(d.yvalues, 
                                       (derive ? "_diff.dat" : 
                                        "_bl.dat")));
      return;
    }
    case Abort:
      chrg.cancel = true;
      return;
    case Derive:
      derive = ! derive;
      needCompute = true;
      if(derive)
        soas().showMessage("Showing derivative");
      else
        soas().showMessage("Showing baseline");
      break;
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

    if(needCompute) {
      // In any case, the bottom panel shows the delta.
      if(derive) {
        d.yvalues = s.derivative(d.xvalues, type);
        diff.yvalues = (d.yvalues.size() == ds->y().size() ?
                        ds->y() - s.evaluate(d.xvalues, type):
                        ds->y());
      } 
      else {
        d.yvalues = s.evaluate(d.xvalues, type);
        diff.yvalues = (d.yvalues.size() == ds->y().size() ? 
                        ds->y() - d.yvalues :
                        ds->y());
      }

      charge = Vector::integrate(ds->x(), diff.yvalues);
      Terminal::out << "Charge: " << charge << " (old: " 
                    << oldCharge << ")" << endl;

      
      m.points = s.pointList();
      bottom.setYRange(diff.yvalues.min(), diff.yvalues.max(), 
                       view.mainPanel());
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
  
  


  static void bsplinesCommand(CurveEventLoop &loop, const QString &)
  {
    const DataSet * ds = soas().currentDataSet();
    const GraphicsSettings & gs = soas().graphicsSettings();

    CurveView & view = soas().view();
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
            d.xvalues = ds->x().resample(resample);
            d.yvalues = splines.computeValues(d.xvalues, 1);
          }
        }
        else {
          if(resample < 2) {
            d.yvalues = splines.computeValues();
            diff.yvalues = ds->y() - d.yvalues;
          }
          else {
            d.xvalues = ds->x().resample(resample);
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

  static Command 
  bspl("filter-bsplines", // command name
       optionLessEffector(bsplinesCommand), // action
       "buffer",  // group name
       NULL, // arguments
       NULL, // options
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
    TogglePowerSpectrum,
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
  CurveData d;
  CurveData diff;
  CurveData baseline;
  CurveData spec1;
  CurveData spec2;
  CurveVerticalLine lim;
  FFT orig(ds->x(), ds->y());


  d.pen = gs.getPen(GraphicsSettings::ResultPen);
  d.xvalues = ds->x();
  d.yvalues = QVector<double>(d.xvalues.size(), 0);
  d.countBB = true;
  view.addItem(&d);


  baseline.pen = gs.getPen(GraphicsSettings::BaselinePen);
  baseline.xvalues = ds->x();
  baseline.yvalues = baseline.xvalues;;
  baseline.countBB = false;
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

  // For some reason, we can't just set the order right now.
  bool derive = false;
  updateFromOptions(opts, "derive", derive);
  // The order of the derivative
  int order = (derive ? 1 : 0);


  // **************************************************
  // Setup of the "power" panel


  spec1.pen = gs.getPen(GraphicsSettings::ResultPen);
  // We don't display the 0th frequency !
  spec1.xvalues = Vector(orig.frequencies() - 1,0);
  for(int i = 0; i < spec1.xvalues.size(); i++)
    spec1.xvalues[i] = log((i+1)/(1.0*spec1.xvalues.size()));
  spec1.yvalues = spec1.xvalues;
  spec1.countBB = true;
  spectrum.addItem(&spec1);

  spec2.xvalues = spec1.xvalues;
  spec2.yvalues = spec1.yvalues;
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
  dsDisplay->hidden = derive;

  {
    QList<int> facts = orig.factors();
    QStringList lst;
    for(int i = 0; i < facts.size(); i++)
      lst << QString::number(facts[i]);
    Terminal::out << "Mixed-radix: " << ds->x().size() << " factorized as "
                  << lst.join(" * ") << endl;
  }
  orig.forward();
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
    case ToggleDerivative:
      if(order > 0)
        order = 0;
      else
        order = 1;
      dsDisplay->hidden = order;
      needCompute = true;
      if(order)
        soas().showMessage("Showing derivative");
      else
        soas().showMessage("Showing filtered data");
      break;
    case TogglePowerSpectrum:
        showSpectrum = ! showSpectrum;
        needUpdate = true;
        needCompute = true;
        break;
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
      FFT trans = orig;
      double cf = ds->x().size()/2 - cutoff;
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

      // Compute the baseline
      trans.baseline(&baseline.yvalues);

      if(showSpectrum) {
        QRectF r = spec1.boundingRect();
        r.setTop(spec1.yvalues.min()-20);
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
       << new BoolArgument("derive", 
                           "Derive",
                           "Whether to derive by default")
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


  updateFromOptions(opts, "number", nb);
  updateFromOptions(opts, "order", order);
  updateFromOptions(opts, "derivatives", derivatives);

  BSplines splines(ds, order, derivatives);
  splines.autoBreakPoints(nb);
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

static void setSegmentsCommand(CurveEventLoop &loop, const QString &)
{
  DataSet * ds = soas().currentDataSet();
  QList<int> savedSegments = ds->segments;
  CurveView & view = soas().view();

  loop.setHelpString("Set segments:\n"
                     "left click: place delimiter\n"
                     "right click: remove closest delimiter\n"
                     "q, mid: accept\n"
                     "ESC: abort\n");
  do {
    if(loop.isConventionalAccept()) {
      if(ds->segments != savedSegments) {
        /// @hack This is ugly
        DataSet * nds = new DataSet(*ds);
        soas().pushDataSet(nds);
      }
      else
        Terminal::out << "Segments didn't change, not creating a new buffer" 
                      << endl;
      break;
    }
    switch(loop.type()) {
    case QEvent::MouseButtonPress: {
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

      if(loop.button() == Qt::RightButton) { // Remove 
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
      else if(loop.button() == Qt::LeftButton) { // Add
        int target = 0;
        for(int i = 0; i <= ds->segments.size(); i++) {
          target = i;
          if(i < ds->segments.size() && ds->segments[i] > idx)
            break;
        }
        ds->segments.insert(target, idx);
      }
      
      Terminal::out << "Current segments: " << endl;
      for(int i = 0; i < ds->segments.size(); i++)
        Terminal::out << "Segment change #" << i << " @" << ds->segments[i] 
                      << "\t X= " << ds->x()[ds->segments[i]] <<endl;
      
      break;
    }
    case QEvent::KeyPress: 
      switch(loop.key()) {
      case Qt::Key_Escape:
        loop.terminate();
        break;
      default:
        ;
      }
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
     optionLessEffector(setSegmentsCommand), // action
     "buffer",  // group name
     NULL, // arguments
     NULL, // options
     "Set segments",
     "Set segments in the data (manually)",
     "...");



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
    OutFile::out.setHeader("buffer\twhat\tx\ty\tindex");
  }
  if(maxnb < 0)
    maxnb = peaks.size();
  for(int i = 0; i < maxnb; i++) {
    QString str = QString("%1\t%2\t%3\t%4\t%5").
      arg(ds->name).arg((peaks[i].isMin ? "min" : "max" )).
      arg(peaks[i].x).arg(peaks[i].y).arg(peaks[i].index);
    if(write)
      OutFile::out << str << "\n" <<  flush;
    Terminal::out << str << endl;
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

  updateFromOptions(opts, "window", window);
  updateFromOptions(opts, "peaks", nb);
  updateFromOptions(opts, "include-borders", includeBorders);
  updateFromOptions(opts, "output", write);

  const DataSet * ds = soas().currentDataSet();
  Peaks pk(ds, window);

  QList<PeakInfo> peaks = pk.findPeaks(includeBorders);
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
    "Find two peak",
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

  DataSet * newds = ds->derivedDataSet(ds->y(), "_deldp.dat");

  view.clear();

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
        if(t.lastIndex >= 0)
          newds->removePoint(t.lastIndex);
        d.invalidateCache();
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
                              "Which axis is zero-ed")
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
                                   const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  FFT f(ds->x(), ds->y(), false);

  f.forward(false);

  double dx = f.deltaX;

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


/// Decomposition into singular values.
/// 
/// This function decomposes a given dataset containing more than 1 Y
/// column into singular values, applying a threshold when applicable.
///
/// When fully-functional (ie not only displaying the singular
/// values), this command generates two datasets:
/// \li the "components" dataset, that has the same X values as the original
/// one
/// \li the "amplitudes" dataset, that has as many rows as there are Y
/// columns initially (would be the time amplitude if the original dataset
/// are spectra recorded at different times)
static void svCommand(const QString &,
                      const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  int nbrows = ds->nbRows();
  int nbcols = ds->nbColumns() - 1;
  if(nbcols < 2)
    throw RuntimeError("Need more than 1 Y columns !");
  gsl_matrix * data = gsl_matrix_alloc(nbrows, nbcols);

  // Now, populate the matrix
  for(int i = 0; i < nbcols; i++) {
    gsl_vector_view col = gsl_matrix_column(data, i);
    gsl_vector_memcpy(&col.vector, ds->column(i+1).toGSLVector());
  }

  Terminal::out << "Preparing matrices of " << nbcols << "x" << nbrows << endl;

  gsl_matrix * amplitudes = gsl_matrix_alloc(nbcols, nbcols);
  gsl_vector * values = gsl_vector_alloc(nbcols);
  gsl_vector * work = gsl_vector_alloc(nbcols);

  gsl_linalg_SV_decomp(data, amplitudes, values, work);

  // Now, we show the values:
  Terminal::out << "Singular values: ";
  for(int i = 0; i < nbcols; i++) 
    Terminal::out << gsl_vector_get(values, i) << " ";
  Terminal::out << "Condition number: " 
                << gsl_vector_get(values, 0)/
    gsl_vector_get(values, nbcols - 1) << endl;


  // And we must do something with that now !

  int components = -1;
  bool filterOnly = false;
  updateFromOptions(opts, "components", components);

  // If components are computed, either directly or though a
  // threshold, and in the case we don't filter, we create two buffers
  // with the same number of columns (number of selected components
  if(components > 0) {
    if(! filterOnly) {
      QList<Vector> ds1;
      QList<Vector> ds2;
      ds1 << ds->x();

      // the amplitude is held by the the main matrix
      for(int i = 0; i < components; i++) {
        double val = gsl_vector_get(values, i);
        gsl_vector_view v = gsl_matrix_column(data, i);
        gsl_vector_scale(&v.vector, val);
        ds1 << Vector::fromGSLVector(&v.vector);
      }

      Vector nx;
      for(int j = 0; j < nbcols; j++)
        nx << j;
      ds2 << nx;
      for(int i = 0; i < components; i++) {
        gsl_vector_view v = gsl_matrix_column(amplitudes, i);
        ds2 << Vector::fromGSLVector(&v.vector);
      }
      soas().pushDataSet(ds->derivedDataSet(ds2, "_amplitudes.dat"));
      soas().pushDataSet(ds->derivedDataSet(ds1, "_components.dat"));
    }
    else {
      /// @todo !
    }
  }

  

  // Cleanup !
  gsl_matrix_free(amplitudes);
  gsl_matrix_free(data);
  gsl_vector_free(values);
  gsl_vector_free(work);
}


static ArgumentList 
svOpts(QList<Argument *>() 
       << new IntegerArgument("components", 
                              "Number of components to keep",
                              "If specified, keeps only that many components")
       << new BoolArgument("filter", 
                           "Filter only",
                           "If on, does not decompose, only filter")
       );

static Command 
svc("sv-decomp", // command name
    effector(svCommand), // action
    "buffer",  // group name
    NULL, // arguments
    &svOpts, // options
    "SV decomposition",
    "Singular value decomposition");
