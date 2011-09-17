/**
   \file dataset-commands.cc commands for tweaking datasets
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
#include <outfile.hh>

#include <dataset.hh>
#include <soas.hh>
#include <curveview.hh>
#include <curveeventloop.hh>
#include <curvemarker.hh>
#include <curveitems.hh>
#include <curvepanel.hh>

#include <pointpicker.hh>

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

static void chopCommand(const QString &, QList<double> values, 
                        const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  QList<DataSet *> splitted;
  if(opts.contains("mode") && 
     opts["mode"]->value<QString>() == "index") {
    QList<int> split;
    for(int i = 0; i < values.size(); i++)
      split << values[i];
    splitted = ds->chop(split);
  }
  else
   splitted = ds->chop(values);
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
                            << "xvalues"
                            << "index",
                            "mode", 
                            "Mode",
                            "Whether to cut on index or x values (default)"));

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


static void cursorCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  CurveEventLoop loop;
  CurveMarker m;
  CurveView & view = soas().view();
  PointPicker pick(&loop, ds);

  view.addItem(&m);
  m.size = 4;
  m.pen = QPen(Qt::NoPen);
  m.brush = QBrush(QColor(0,0,255,100)); // A kind of transparent blue
  loop.setHelpString(QObject::tr("Cursor:\n"
                                 "click to see\n") 
                     + pick.helpText() +
                     QObject::tr("space: write to output\n"
                                 "q or ESC to quit"));
  while(! loop.finished()) {
    if(pick.processEvent()) {
      if(pick.button() != Qt::NoButton) {
        m.p = pick.point();
        Terminal::out << m.p.x() << "\t"
                      << m.p.y() << "\t" 
                      << pick.pointIndex() << endl;
      }
    }
    switch(loop.type()) {
    case QEvent::KeyPress: 
      if(loop.key() == 'q' || loop.key() == 'Q' ||
         loop.key() == Qt::Key_Escape)
        return;
      if(loop.key() == ' ') {
        // Write to output file
        OutFile::out.setHeader("Point positions:\n"
                               "X\tY\t\tidx\tbuffer");
        Terminal::out << "Writing position to output file: '" 
                      << OutFile::out.fileName() << "'" << endl;

        OutFile::out << m.p.x() << "\t"
                     << m.p.y() << "\t" 
                     << pick.pointIndex() << "\t"
                     << ds->name << "\n" << flush;
      }
      break;
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
//////////////////////////////////////////////////////////////////////


static void cutCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  CurveHorizontalRegion r;
  CurveView & view = soas().view();

  /// We remove the current display
  view.clear();
  CurveEventLoop loop;
  CurveData d;
  view.addItem(&r);
  view.addItem(&d);

  r.pen = QPen(QColor("blue"), 1, Qt::DotLine);
  d.pen = QPen(QColor("red"));

  view.mainPanel()->xLabel = "Index";

  loop.setHelpString(QObject::tr("Cut:\n"
                                 "left/right: bounds\n"
                                 "q: keep only the inside\n"
                                 "u: keep only the outside\n"
                                 "ESC: cancel"));

  d.countBB = true;
  d.yvalues = ds->y();
  d.xvalues = d.yvalues;
  for(int i = 0; i < d.xvalues.size(); i++)
    d.xvalues[i] = i;
  r.xleft = 0;
  r.xright = d.xvalues.size()-1;
    
  /// @todo selection mode ? (do we need that ?)
  while(! loop.finished()) {
    switch(loop.type()) {
    case QEvent::MouseButtonPress: {
      if(loop.button() == Qt::MiddleButton) {
        soas().pushDataSet(ds->subset(r.xleft, r.xright, true));
        return;
      }
      r.setX(loop.position().x(), loop.button());
      break;
    }
    case QEvent::KeyPress: 
      switch(loop.key()) {
      case Qt::Key_Escape:
        view.addDataSet(ds);  // To turn its display back on
        return;
      case 'q':
      case 'Q':
        soas().pushDataSet(ds->subset(r.xleft, r.xright, true));
        return;
          
      case 'U':
      case 'u':
        soas().pushDataSet(ds->subset(r.xleft, r.xright, false));
        return;
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
cut("cut", // command name
    optionLessEffector(cutCommand), // action
    "buffer",  // group name
    NULL, // arguments
    NULL, // options
    "Cut",
    "Cuts the current curve",
    "Cuts bits from the current curve",
    "c");

//////////////////////////////////////////////////////////////////////


static void zoomCommand(const QString &)
{
  soas().currentDataSet(); // to ensure datasets are loaded
  CurveEventLoop loop;
  CurveRectangle r;
  CurveView & view = soas().view();
  CurvePanel * panel = NULL;
  view.addItem(&r);
  r.pen = QPen(Qt::DotLine);
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


static void subCommand(const QString &, DataSet * a, 
                       DataSet * b, const CommandOptions & opts)
{
  bool naive = false;
  if(opts.contains("mode") && opts["mode"]->value<QString>() == "indices")
    naive = true;
  Terminal::out << QObject::tr("Subtracting buffer '%1' from buffer '%2'").
    arg(b->name).arg(a->name) 
                << (naive ? " (index mode)" : " (xvalues mode)" ) 
                << endl;
  soas().pushDataSet(a->subtract(b, naive));
}

static ArgumentList 
suba(QList<Argument *>() 
     << new DataSetArgument("first", 
                            "Buffer",
                            "First buffer")
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
                                    "values or indices"));


static Command 
sub("subtract", // command name
    effector(subCommand), // action
    "buffer",  // group name
    &suba, // arguments
    &operationOpts, // options
    "Subtract",
    "Subtract on buffer from another",
    "Subtract the second buffer from the first",
    "S");

//////////////////////////////////////////////////////////////////////


static void divCommand(const QString &, DataSet * a, 
                       DataSet * b, const CommandOptions & opts)
{
  bool naive = false;
  if(opts.contains("mode") && opts["mode"]->value<QString>() == "indices")
    naive = true;
  Terminal::out << QObject::tr("Dividing buffer '%2' by buffer '%1'").
    arg(b->name).arg(a->name) 
                << (naive ? " (index mode)" : " (xvalues mode)" ) 
                << endl;
  soas().pushDataSet(a->divide(b, naive));
}

static Command 
divc("div", // command name
     effector(divCommand), // action
     "buffer",  // group name
     &suba, // arguments
     &operationOpts, // options
     "Divide",
     "Divide one buffer by another",
     "Divide the first buffer by the second");

//////////////////////////////////////////////////////////////////////


static void shiftxCommand(const QString &)
{
  const DataSet * ds = soas().currentDataSet();
  DataSet * newDs = new DataSet(*ds);
  double x = newDs->x()[0];
  Terminal::out << QObject::tr("Shifting X axis by %1").
    arg(x) << endl;
  newDs->x() -= x;
  newDs->name = newDs->cleanedName() + "_shiftx.dat";
  soas().pushDataSet(newDs);
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
