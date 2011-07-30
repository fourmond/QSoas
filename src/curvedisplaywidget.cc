/*
  curvedisplaywidget.cc: displaying 2D curves...
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
#include <curvedisplaywidget.hh>

#include <curveitem.hh>
#include <terminal.hh>

CurveDisplayWidget * CurveDisplayWidget::theCurveDisplayWidget = NULL;

CurveDisplayWidget::CurveDisplayWidget() 
{
  theCurveDisplayWidget = this;
  scene = new QGraphicsScene;
  QHBoxLayout * layout = new QHBoxLayout(this);
  view = new QGraphicsView(scene);
  layout->addWidget(view);
  view->setFocusPolicy(Qt::NoFocus);
  view->show();
  setFocusPolicy(Qt::NoFocus);
}

CurveDisplayWidget::~CurveDisplayWidget()
{
  delete scene;
}

void CurveDisplayWidget::addDataSet(const DataSet * ds)
{
  CurveItem * it = new CurveItem(ds);
  scene->addItem(it);

  QSize s = view->size();
  QRectF r = scene->sceneRect();
  QPointF tl = r.topLeft();
  QPointF br = r.bottomRight();

  // Terminal::out << "View rect is: " << r.x() << "," << r.y()
  //               << " with width = " << r.width() << " and "
  //               << " height = " << r.height() << endl;
  
  QTransform t;
  t.translate(-r.x(), -r.y());
  t.scale(s.width()/r.width(), -s.height()/r.height());
  view->setTransform(t);
  
  // view->fitInView(r);
  // view->centerOn(it);
  // view->invalidateScene();

  QPointF p = r.topLeft();
  Terminal::out << "Old coordinates: " << p.x() 
                << "," <<  p.y() << endl;
  p = view->mapFromScene(p);
  Terminal::out << "New coordinates: " << p.x() 
                << "," <<  p.y() << endl;
  

  // QRectF r = view->sceneRect();

  // r = scene->sceneRect();
  // Terminal::out << "Scene rect is: " << r.x() << "," << r.y()
  //               << " with width = " << r.width() << " and "
  //               << " height = " << r.height() << endl;

  // r = it->boundingRect();
  // Terminal::out << "BB rect is: " << r.x() << "," << r.y()
  //               << " with width = " << r.width() << " and "
  //               << " height = " << r.height() << endl;
}
