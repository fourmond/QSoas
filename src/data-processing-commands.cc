/**
   \file data-processing-commands.cc
   Commands to extract information from datasets
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
#include <curveitems.hh>
#include <curvepanel.hh>
#include <math.h>

#include <utils.hh>
#include <outfile.hh>

#include <spline.hh>
#include <bsplines.hh>
#include <pointpicker.hh>

//////////////////////////////////////////////////////////////////////
static void reglinCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  CurveEventLoop loop;
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

  view.addPanel(&bottom);
  r.pen = QPen(QColor("blue"), 1, Qt::DotLine);
  r.pen.setCosmetic(true);
  line.pen = r.pen;        // Change color ? Use a utils function ?
  d.pen = QPen(QColor("red"));
  QPair<double, double> reg;
  double xleft = ds->x().min();
  double xright = ds->x().max();


  loop.setHelpString(QObject::tr("Linear regression:\n"
                                 "left click: left boundary\n"
                                 "right click: right boundary\n"
                                 "space: write to output file\n"
                                 "u: subtract trend\n"
                                 "v: divide by trend\n"
                                 "e: divide by exp decay\n"
                                 "q, ESC: quit"));
  /// @todo selection mode ? (do we need that ?)
  while(! loop.finished()) {
    switch(loop.type()) {
    case QEvent::MouseButtonPress: 
      {
        r.setX(loop.position().x(), loop.button());
        reg = ds->reglin(r.xleft, r.xright);
        double y = reg.first * xleft + reg.second;
        line.p1 = QPointF(xleft, y);
        y = reg.first * xright + reg.second;
        line.p2 = QPointF(xright, y);
        Terminal::out << reg.first << "\t" << reg.second << endl;
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
    case QEvent::KeyPress: 
      switch(loop.key()) {
      case 'q':
      case 'Q':
      case Qt::Key_Escape:
        return;
      case 'U':
      case 'u': {
        // Subtracting, the data is already computed in d
        DataSet * newds = new 
          DataSet(QList<Vector>() << d.xvalues << d.yvalues);
        newds->name = ds->cleanedName() + "_linsub.dat";
        soas().pushDataSet(newds);
        return;
      }
      case ' ': {
        OutFile::out.setHeader(QString("Dataset: %1\n"
                                       "a\tb\txleft\txright").
                               arg(ds->name));
        /// @todo add other fields ? 
        OutFile::out << reg.first << "\t" << reg.second << "\t"
                     << r.xleft << "\t" << r.xright << "\n" << flush;
        Terminal::out << "Writing to output file " << OutFile::out.fileName()
                      << endl;
        break;
      }
      case 'V':
      case 'v': {
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
      case 'E':
      case 'e': {
        // Dividing by exponential decay
        Vector newy = ds->y();
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
      break;
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

//////////////////////////////////////////////////////////////////////

static void baselineCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  CurveEventLoop loop;
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

  view.addItem(&m);
  view.addItem(&d);

  m.size = 4;
  m.pen = QPen(Qt::NoPen);
  m.brush = QBrush(QColor(0,0,255,100));

  Spline::Type type = Spline::CSpline;

  d.pen = QPen(QColor("black"));
  d.xvalues = ds->x();
  d.yvalues = QVector<double>(d.xvalues.size(), 0);
  d.countBB = true;
  diff.xvalues = d.xvalues;
  diff.yvalues = ds->y();

  bottom.addItem(&diff);

  view.addPanel(&bottom);

  loop.setHelpString(QObject::tr("Baseline interpolation:\n"
                                 "left click: left boundary\n"
                                 "right click: right boundary\n"
                                 "d: display derivative\n"
                                 "q: subtract baseline\n"
                                 "v: divide by baseline\n"
                                 "u: to replace by baseline\n"
                                 "ESC: abord"));
  while(! loop.finished()) {
    bool needCompute = false;
    if(pick.processEvent()) {
      if(pick.button() != Qt::NoButton) {
        s.insert(pick.point());
        needCompute = true;
      }
    }
    else {
      switch(loop.type()) {
      case QEvent::MouseButtonPress: 
        if(loop.button() == Qt::RightButton) { // Removing
          s.remove(loop.position().x());
          needCompute = true;
        }
        break;
      case QEvent::KeyPress: 
        switch(loop.key()) {
        case Qt::Key_Escape:
          return;
        case 'A':
        case 'a':
          type = Spline::Akima;
          needCompute = true;
          soas().showMessage("Using Akima spline interpolation");
          break;
        case '-':
          s.clear();
          needCompute = true;
          break;
        case '1': // left
        case '2': // right
          {
            // Closest data point:
            double xlim = (loop.key() == '1' ? 
                           ds->x().min() : ds->x().max());
            double x = loop.position().x();
            for(int i = 0; i < 10; i++) {
              double nx = xlim + (x - xlim)*i/9;
              double ny = ds->yValueAt(nx);
              s.insert(QPointF(nx, ny));
            }
            needCompute = true;
          }
          break;
        case 'D':
        case 'd':
          derive = ! derive;
          needCompute = true;
          if(derive)
            soas().showMessage("Showing derivative");
          else
            soas().showMessage("Showing baseline");
          break;
        case 'C':
        case 'c':
          type = Spline::CSpline;
          needCompute = true;
          soas().showMessage("Using C-spline interpolation");
          break;
        case 'P':
        case 'p':
          type = Spline::Polynomial;
          needCompute = true;
          soas().showMessage("Using polynomial interpolation");
          break;
        case 'q':
        case 'Q': {
          // Subtracting, the data is already computed in d
          DataSet * newds = new 
            DataSet(QList<Vector>() << d.xvalues << diff.yvalues);
          newds->name = ds->cleanedName() + "_bl_sub.dat";
          soas().pushDataSet(newds);
          return;
        }
        case 'U':
        case 'u': {
          // Replacing the data is already computed in d
          DataSet * newds = new 
            DataSet(QList<Vector>() << d.xvalues << d.yvalues);
          newds->name = ds->cleanedName() + (derive ? "_diff.dat" : 
                                             "_bl.dat");
          soas().pushDataSet(newds);
          return;
        }
        case 'V':
        case 'v': {
          // Dividing
          Vector newy = ds->y();
          newy /= d.yvalues;
          DataSet * newds = new 
            DataSet(QList<Vector>() << d.xvalues << newy);
          newds->name = ds->cleanedName() + "_bl_div.dat";
          soas().pushDataSet(newds);
          return;
        }
        default:
          ;
        }
        break;
      default:
        ;
      }
    }
    if(needCompute) {
      // In any case, the bottom panel shows the delta.
      if(derive) {
        d.yvalues = s.derivative(d.xvalues, type);
        diff.yvalues = ds->y() - s.evaluate(d.xvalues, type);
      } 
      else {
        d.yvalues = s.evaluate(d.xvalues, type);
        diff.yvalues = ds->y() - d.yvalues;
      }
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

//////////////////////////////////////////////////////////////////////

static void bsplinesCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  CurveEventLoop loop;
  CurveView & view = soas().view();
  CurvePanel bottom;
  CurveData d;
  CurveData diff;
  bool derive = false;
  bottom.drawingXTicks = false;
  bottom.stretch = 30;        // 3/10ths of the main panel.
  int nbPoints = 10;
  int order = 4;

  view.addItem(&d);

  d.pen = QPen(QColor("black"));
  d.xvalues = ds->x();
  d.yvalues = QVector<double>(d.xvalues.size(), 0);
  d.countBB = true;
  diff.xvalues = d.xvalues;
  diff.yvalues = ds->y();



  bottom.addItem(&diff);
  bottom.yLabel = Utils::deltaStr("Y");

  view.addPanel(&bottom);


  /// Position of the segments
  Vector x;
  bool autoXValues = true;
  bool needCompute = true;
  BSplines splines(ds, order);

  CurveVerticalLines lines;
  lines.xValues = &x;
  lines.pen = QPen(QColor("blue"), 1, Qt::DotLine);
  view.addItem(&lines);

  loop.setHelpString(QObject::tr("B-splines filtering:\n"
                                 "left click: place point\n"
                                 "right click: remove closest point\n"
                                 "d: display derivative\n"
                                 "o: optimize positions\n"
                                 "q: replace with filtered data\n"
                                 "ESC: abord"));
  do {
    switch(loop.type()) {
    case QEvent::MouseButtonPress: 
      /// @todo actual point placing
      if(loop.button() == Qt::RightButton) { // Remove
        nbPoints--;
        if(nbPoints < 2)
          nbPoints = 2;
        needCompute = true;
        autoXValues = true;
        Terminal::out << "Using : " << nbPoints << " points" << endl;
      }
      if(loop.button() == Qt::LeftButton) { // Remove
        nbPoints++;
        needCompute = true;
        autoXValues = true;
        Terminal::out << "Using : " << nbPoints << " points" << endl;
      }
      break;
    case QEvent::KeyPress: 
      switch(loop.key()) {
      case Qt::Key_Escape:
        return;
      case 'D':
      case 'd':
        derive = ! derive;
        needCompute = true;
        if(derive)
          soas().showMessage("Showing derivative");
        else
          soas().showMessage("Showing baseline");
        break;
      case 'O':
      case 'o':
        splines.optimize();
        x = splines.getBreakPoints();
        needCompute = true;
        break;
      case 'q':
      case 'Q': {
        // Quit replacing with data
        DataSet * newds = new 
          DataSet(QList<Vector>() << d.xvalues << d.yvalues);
        newds->name = ds->cleanedName() + "_filtered.dat";
        soas().pushDataSet(newds);
        return;
      }
      default:
        ;
      }
      break;
    default:
      ;
    }
    if(needCompute) {
      QRectF bb = ds->boundingBox();
      if(autoXValues) {
        double xmin = bb.left();
        double xmax = bb.right();
        x.clear();
        for(int i = 0; i < nbPoints; i++)
          x << xmin + (xmax - xmin)/(nbPoints + 1)*(i+1);
        autoXValues = false;
        splines.setBreakPoints(x);
      }

      // Should move to yet another place
      double value = splines.computeCoefficients();
      if(derive) {
        diff.yvalues = ds->y() - splines.computeValues();
        d.yvalues = splines.computeValues(1);
      }
      else {
        d.yvalues = splines.computeValues();
        diff.yvalues = ds->y() - d.yvalues;
      }
      Terminal::out << "Residuals: " << value << endl;
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

//////////////////////////////////////////////////////////////////////

static void findStepsCommand(const QString &, const CommandOptions & opts)
{
  double thresh = 0.1;
  int nb = 10;

  updateFromOptions(opts, "average", nb);
  updateFromOptions(opts, "threshold", thresh);

  const DataSet * ds = soas().currentDataSet();
  QList<int> steps = ds->findSteps(nb, thresh);
  CurveView & view = soas().view();
  view.disableUpdates();
  for(int i = 0; i < steps.size(); i++) {
    Terminal::out << "Step #" << i << " @" << steps[i] 
                  << "\t X= " << ds->x()[steps[i]] <<endl;
    CurveVerticalLine * v= new CurveVerticalLine;
    v->x = 0.5* (ds->x()[steps[i]] + ds->x()[steps[i]-1]);
    v->pen = QPen(QColor("blue"), 1, Qt::DotLine);
    view.addItem(v);
  }
  view.enableUpdates();
}

static ArgumentList 
fsOps(QList<Argument *>() 
      << new IntegerArgument("average", 
                            "Average over",
                            "Average over that many points")
      << new NumberArgument("threshold", 
                            "Threshold",
                            "Detection threshold")
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
     "Do not use this on noisy data");

