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

#include <fft.hh>

#include <peaks.hh>

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
  d.countBB = true;


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

  diff.countBB = true;

  bottom.addItem(&diff);

  view.addPanel(&bottom);

  loop.setHelpString(QObject::tr("Baseline interpolation:\n"
                                 "left click: place point\n"
                                 "right click: remove point\n"
                                 "d: display derivative\n"
                                 "q, middle click: subtract baseline\n"
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
      if(loop.isConventionalAccept()) {
        // Subtracting, the data is already computed in d
        /// @todo Probably
        DataSet * newds = new 
          DataSet(QList<Vector>() << d.xvalues << diff.yvalues);
        newds->name = ds->cleanedName() + "_bl_sub.dat";
        soas().pushDataSet(newds);
        return;
      }
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
          /// @todo Use a "toogle" to change the method used.
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
      QTextStream o(stdout);
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

  d.pen = QPen(QColor("black"));
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

  CurveVerticalLines lines;
  lines.xValues = &x;
  lines.pen = QPen(QColor("blue"), 1, Qt::DotLine);
  view.addItem(&lines);

  loop.setHelpString(QObject::tr("B-splines filtering:\n"
                                 "left click: place point\n"
                                 "right click: remove closest point\n"
                                 "d: display derivative\n"
                                 "a: equally spaced segments\n"
                                 "o: optimize positions\n"
                                 "+,-: change splines order\n"
                                 "q, middle click: replace with filtered data\n"
                                 "ESC: abort"));
  do {
    if(loop.isConventionalAccept()) {
      // Quit replacing with data
      DataSet * newds = new 
        DataSet(QList<Vector>() << d.xvalues << d.yvalues);
      newds->name = ds->cleanedName() + "_filtered.dat";
      soas().pushDataSet(newds);
      return;
    }
    switch(loop.type()) {
    case QEvent::MouseButtonPress: 
      if(loop.button() == Qt::RightButton) { // Remove
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
      }
      if(loop.button() == Qt::LeftButton) { // Remove
        double xv = loop.position().x();
        for(int i = 0; i < x.size() - 1; i++)
          if(xv >= x[i] && xv <= x[i+1]) {
            x.insert(i+1, xv);
            break;
          }

        nbSegments += 1;
        needCompute = true;
        Terminal::out << "Using : " << nbSegments << " segments" << endl;
      }
      break;
    case QEvent::KeyPress: 
      switch(loop.key()) {
      case Qt::Key_Escape:
        return;
      case 'D':
      case 'd':
        derive = ! derive;
        dsDisplay->hidden = derive;
        needCompute = true;
        if(derive)
          soas().showMessage("Showing derivative");
        else
          soas().showMessage("Showing filtered data");
        break;
      case 'O':
      case 'o':
        splines.optimize(15, false);
        x = splines.getBreakPoints();
        needCompute = true;
        break;
      case 'A':
      case 'a':
        needCompute = true;
        autoXValues = true;
        break;
      case '+':
        ++order;
        Terminal::out << "Now using splines of order " << order << endl;
        needCompute = true;
        break;
      case '-':
        --order;
        if(order < 2)
          order = 2;            // must be at least linear !
        Terminal::out << "Now using splines of order " << order << endl;
        needCompute = true;
        break;
      default:
        ;
      }
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
        d.yvalues = splines.computeValues(1);
      }
      else {
        d.yvalues = splines.computeValues();
        diff.yvalues = ds->y() - d.yvalues;
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

//////////////////////////////////////////////////////////////////////

static void fftCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  CurveEventLoop loop;
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


  d.pen = QPen(QColor("black"));
  d.xvalues = ds->x();
  d.yvalues = QVector<double>(d.xvalues.size(), 0);
  d.countBB = true;
  view.addItem(&d);


  baseline.pen = QPen(QColor("blue"), 1, Qt::DotLine);
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

  // The order of the derivative
  int order = 0;


  // **************************************************
  // Setup of the "power" panel


  spec1.pen = QPen(QColor("red"));
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

  lim.pen = QPen(QColor("blue"), 1, Qt::DotLine);
  spectrum.addItem(&lim);




  /// Position of the segments
  Vector x;
  bool needUpdate = false;
  bool showSpectrum = false;
  bool needCompute = true;

  int cutoff = 20;

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

  loop.setHelpString(QObject::tr("FFT filtering:\n"
                                 "left click: place point\n"
                                 "right click: remove closest point\n"
                                 "p: toogle power spectrum\n"
                                 "q, middle click: replace with filtered data\n"
                                 "ESC: abort"));
  do {
    if(loop.isConventionalAccept()) {
      // Quit replacing with data
      DataSet * newds = new 
        DataSet(QList<Vector>() << d.xvalues << d.yvalues);
      newds->name = ds->cleanedName() + "_filtered.dat";
      soas().pushDataSet(newds);
      return;
    }
    switch(loop.type()) {
    case QEvent::MouseButtonPress: 
      if(loop.currentPanel() == &spectrum) {
        if(loop.button() == Qt::LeftButton) { 
          double x = loop.position(&spectrum).x();
          cutoff = exp(-x);
        }
      }
      else {                                 // +/- decrease.
        if(loop.button() == Qt::RightButton) { // Decrease
          cutoff--;
          if(cutoff < 2)
            cutoff = 2;
        }
        if(loop.button() == Qt::LeftButton) { // Remove
          cutoff++;
          if(cutoff > ds->x().size()/2 - 2)
            cutoff = ds->x().size()/2 - 2;
        }
        /// @todo This should go on the panel ? 
        Terminal::out << "Now using a cutoff of " << cutoff << endl;
      }
      needCompute = true;
      break;
    case QEvent::KeyPress: 
      switch(loop.key()) {
      case Qt::Key_Escape:
        return;
      case 'd':
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
      case 'p':
        showSpectrum = ! showSpectrum;
        needUpdate = true;
        needCompute = true;
        break;
      default:
        ;
      }
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

static Command 
fft("filter-fft", // command name
    optionLessEffector(fftCommand), // action
    "buffer",  // group name
    NULL, // arguments
    NULL, // options
    "Filter",
    "Filter using FFT",
    "...");


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
  for(int i = 0; i <= derivatives; i++) {
    DataSet * newds = new 
      DataSet(QList<Vector>() << ds->x() << splines.computeValues(i));
    newds->name = ds->cleanedName() + "_afbs";
    if(i)
      newds->name += QString("_diff_%1").arg(i);
    newds->name += ".dat";
    soas().pushDataSet(newds);
  }
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

  DataSet * newds = new DataSet(ds->x(), orig.data);

  newds->name = ds->cleanedName() + "_fft";
  if(derivatives)
    newds->name += QString("_diff_%1").arg(derivatives);

  newds->name += ".dat";
  soas().pushDataSet(newds);
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

  updateFromOptions(opts, "average", nb);
  updateFromOptions(opts, "threshold", thresh);
  updateFromOptions(opts, "set-segments", set);

  const DataSet * ds = soas().currentDataSet();
  QList<int> steps = ds->findSteps(nb, thresh);
  CurveView & view = soas().view();
  if(set) {
    DataSet * nds = new DataSet(*ds);
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
      v->pen = QPen(QColor("blue"), 1, Qt::DotLine);
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

static void setSegmentsCommand(const QString &, const CommandOptions & )
{
  DataSet * ds = soas().currentDataSet();
  QList<int> savedSegments = ds->segments;
  CurveEventLoop loop;
  CurveView & view = soas().view();

  loop.setHelpString(QObject::tr("Set segments:\n"
                                 "left click: place delimiter\n"
                                 "right click: remove closest delimiter\n"
                                 "q, mid: accept\n"
                                 "ESC: abort (TODO!)"));
  do {
    if(loop.isConventionalAccept()) {
      if(ds->segments != savedSegments) {
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
      if(loop.button() == Qt::RightButton) { // Remove 
        if(ds->segments.size() == 0)
          break;
        if(idx < ds->segments.first()) {
          ds->segments.takeFirst();
          break;
        }
        if(idx > ds->segments.last()) {
          ds->segments.takeLast();
          break;
        }
        double xv = loop.position().x();
        for(int i = 1; i < ds->segments.size() - 1; i++) {
          if(ds->segments[i] <= idx && ds->segments[i+1] >= idx) {
            // We're in !
            if(((xv - ds->x()[ds->segments[i]]) < 
                (ds->x()[ds->segments[i+1]]) - xv))
              ds->segments.takeAt(i);
            else
              ds->segments.takeAt(i+1);
            break;
          }
        }
      }
      if(loop.button() == Qt::LeftButton) { // Add
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
     effector(setSegmentsCommand), // action
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
    v->pen = QPen(QColor("blue"), 1, Qt::DotLine);
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
      v->pen = QPen(QColor("blue"), 1, Qt::DotLine);
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
        v->pen = QPen(QColor("red"), 1, Qt::DotLine);
        view.addItem(v);

        v = new CurveLine;
        v->p1 = QPointF(pairs[i].backward.x, 0);
        v->p2 = QPointF(pairs[i].backward.x, pairs[i].backward.y);
        v->pen = QPen(QColor("green"), 1, Qt::DotLine);
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

static void deldpCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  CurveEventLoop loop;
  CurveView & view = soas().view();

  DataSet * newds = new DataSet(*ds);
  newds->name = ds->cleanedName() + "_deldp.dat";

  view.clear();

  CurveDataSet d(newds);
  PointTracker t(&loop, newds);
  view.addItem(&d);
  d.pen = QPen(QColor("red"));
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
  double fact = 1/(positive ? y.max() : -y.min());
  y *= fact;
  DataSet * nds = new DataSet(ds->x(), y);
  nds->name = ds->cleanedName() + "_norm.dat";
  soas().pushDataSet(nds);
}

static ArgumentList 
normOps(QList<Argument *>() 
        << new BoolArgument("positive", 
                            "Whether we normalize on positive "
                            "or negative values",
                            "...")
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
  DataSet * nds = new DataSet(nx, ny);
  nds->name = ds->cleanedName() + "_off.dat";
  soas().pushDataSet(nds);
}

static ArgumentList 
zeroArgs(QList<Argument *>() 
         << new NumberArgument("value", 
                               "Value at which the other axis should be 0", 
                               "...")
         );

static ArgumentList 
zeroOps(QList<Argument *>() 
        << new ChoiceArgument(QStringList() << "x" << "y",
                              "axis", 
                              "Which axis is 0ed", "...")
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
