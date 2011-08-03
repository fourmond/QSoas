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

namespace DataSetCommands {

  //////////////////////////////////////////////////////////////////////
  static void reglinCommand(const QString &)
  {
    const DataSet * ds = soas().currentDataSet();
    CurveEventLoop loop;
    CurveLine line;
    CurveVerticalLine left, right;
    CurveView & view = soas().view();
    CurvePanel bottom;
    CurveData d;
    bottom.drawingXTicks = false;
    bottom.stretch = 30;        // 3/10ths of the main panel.
    view.addItem(&left);
    view.addItem(&right);
    view.addItem(&line);
    bottom.addItem(&d);

    view.addPanel(&bottom);
    left.pen = QPen(QColor("blue"), 1, Qt::DotLine);
    left.pen.setCosmetic(true);
    right.pen = left.pen;
    line.pen = left.pen;        // Change color ? Use a utils function ?
    d.pen = QPen(QColor("red"));
    QPair<double, double> reg;


    loop.setHelpString(QObject::tr("Linear regression:\n"
                                   "left click: left boundary\n"
                                   "right click: right boundary\n"
                                   "u: subtract trend\n"
                                   "v: divide by trend\n"
                                   "e: divide by exp decay\n"
                                   "q, ESC: quit"));
    /// @todo selection mode ? (do we need that ?)
    while(! loop.finished()) {
      switch(loop.type()) {
      case QEvent::MouseButtonPress: 
        {
          CurveVerticalLine * l = NULL;
          switch(loop.button()) {
          case Qt::LeftButton:
            l = &left;
            break;
          case Qt::RightButton:
            l = &right;
            break;
          default:
            ;
          }
          if(l)
            l->x = loop.position().x();
          if(left.x > right.x)
            std::swap(left.x, right.x);
          // Now performing regression
          reg = ds->reglin(left.x, right.x);
          double y = reg.first * left.x + reg.second;
          line.p1 = QPointF(left.x, y);
          y = reg.first * right.x + reg.second;
          line.p2 = QPointF(right.x, y);
          Terminal::out << reg.first << "\t" << reg.second << endl;

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
            if(x >= left.x && x <= right.x) {
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
      QT_TR_NOOP("Linear regression"),
      QT_TR_NOOP("Performs linear regression"),
      QT_TR_NOOP("..."),
      "reg");

  //////////////////////////////////////////////////////////////////////

}
