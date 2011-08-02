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

#include <dataset.hh>
#include <soas.hh>
#include <curveview.hh>
#include <curveeventloop.hh>
#include <curvemarker.hh>
#include <curveitems.hh>
#include <curvepanel.hh>

namespace DataSetCommands {
  static Group grp("buffer", 2,
                   QT_TR_NOOP("Buffer"),
                   QT_TR_NOOP("Buffer manipulations"));

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
     CommandEffector::functionEffectorOptionLess(splitaCommand), // action
     "buffer",  // group name
     NULL, // arguments
     NULL, // options
     QT_TR_NOOP("Split first"),
     QT_TR_NOOP("Gets buffer until dx sign change"),
     QT_TR_NOOP("Returns the first part of the buffer, until "
                "the first change of sign of dx"));
    
  static void splitbCommand(const QString &)
  {
    const DataSet * ds = soas().currentDataSet();
    splitDataSet(ds, false);
  }
        

  static Command 
  sb("splitb", // command name
     CommandEffector::functionEffectorOptionLess(splitbCommand), // action
     "buffer",  // group name
     NULL, // arguments
     NULL, // options
     QT_TR_NOOP("Split second"),
     QT_TR_NOOP("Gets buffer after first dx sign change"),
     QT_TR_NOOP("Returns the part of the buffer after "
                "the first change of sign of dx"));

  //////////////////////////////////////////////////////////////////////


  static void cursorCommand(const QString &)
  {
    const DataSet * ds = soas().currentDataSet();
    CurveEventLoop loop;
    CurveMarker m;
    CurveView & view = soas().view();
    view.addItem(&m);
    m.size = 4;
    m.pen = QPen(Qt::NoPen);
    m.brush = QBrush(QColor(0,0,255,150)); // A kind of transparent blue
    loop.setHelpString(QObject::tr("Cursor:\n"
                                   "click to see\n"
                                   "q or ESC to quit"));
    /// @todo selection mode ? (do we need that ?)
    while(! loop.finished()) {
      switch(loop.type()) {
      case QEvent::MouseButtonPress: 
        {
          QPair<double, int> dst = loop.distanceToDataSet(ds);
          if(30 > dst.first &&  0 <= dst.second) { // operations in
                                                   // reverse to avoid
                                                   // confusing emacs
            m.p = ds->pointAt(dst.second);
            Terminal::out << dst.second << "\t"
                          << m.p.x() << "\t"
                          << m.p.y() << endl;
          }
          break;
        }
      case QEvent::KeyPress: 
        if(loop.key() == 'q' || loop.key() == 'Q' ||
           loop.key() == Qt::Key_Escape)
          return;
        break;
      default:
        ;
      }
    }
  }

  static Command 
  cu("cursor", // command name
     CommandEffector::functionEffectorOptionLess(cursorCommand), // action
     "buffer",  // group name
     NULL, // arguments
     NULL, // options
     QT_TR_NOOP("Cursor"),
     QT_TR_NOOP("Display cursors on the curve"),
     QT_TR_NOOP("Displays cursors on the curve"),
     "cu");

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
     CommandEffector::functionEffectorOptionLess(zoomCommand), // action
     "buffer",  // group name
     NULL, // arguments
     NULL, // options
     QT_TR_NOOP("Zoom"),
     QT_TR_NOOP("Zooms on the curve"),
     QT_TR_NOOP("Zooms on the current curve"),
     "z");
}
