/*
  datasetbrowser.cc: Implementation of the dataset browser
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
#include <datasetbrowser.hh>
#include <settings-templates.hh>

static SettingsValue<QSize> browserSize("browser/size", QSize(700,500));


DatasetBrowser::DatasetBrowser() : width(4), height(4)
{
  resize(browserSize);
  setupFrame();
}

DatasetBrowser::~DatasetBrowser()
{
  browserSize = size();
  cleanupViews();
}

void DatasetBrowser::cleanupViews()
{
  for(int i = 0; i < views.size(); i++)
    delete views[i];
  views.clear();
  datasets.clear();
}

void DatasetBrowser::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);

  grid = new QGridLayout();
  layout->addLayout(grid);


  QHBoxLayout * hb = new QHBoxLayout;
  QPushButton * bt = new QPushButton(tr("<-"));
  connect(bt, SIGNAL(clicked()), SLOT(previousPage()));
  hb->addWidget(bt);

  bufferDisplay = new QLabel("");
  hb->addWidget(bufferDisplay);

  bt = new QPushButton(tr("->"));
  connect(bt, SIGNAL(clicked()), SLOT(nextPage()));
  hb->addWidget(bt);

  layout->addLayout(hb);

  dataSetChanged(0);
}

void DatasetBrowser::dataSetChanged(int newds)
{
  currentIndex = newds;
  setupGrid();
  bufferDisplay->setText(QString("%1-%2/%3").
                         arg(currentIndex+1).
                         arg(currentIndex + height*width).
                         arg(datasets.size()));
}

void DatasetBrowser::displayDataSets(const QList<const DataSet *> &ds)
{
  cleanupViews();
  datasets = ds;
  for(int i = 0; i < datasets.size(); i++) {
    CurveView * view = new CurveView;
    view->showDataSet(datasets[i]);
    views << view;
  }

  currentIndex = 0;
  // Now, setup the grid
  setupGrid();
}

void DatasetBrowser::displayDataSets(const QList<DataSet *> &ds)
{
  QList<const DataSet * > ds2;
  for(int i = 0; i < ds.size(); i++)
    ds2 << ds[i];
  displayDataSets(ds2);
}

void DatasetBrowser::setupGrid()
{
  // Empty the grid first:
  QTextStream o(stdout);
  
  while(grid->takeAt(0))
    ;                           // We don't delete, as widgets are in
                                // memory
  
  for(int i = 0; i < views.size(); i++)
    views[i]->hide();
  int up = std::min(currentIndex + width*height, views.size());
  for(int i = currentIndex; i < up; i++) {
    int base = i - currentIndex;
    views[i]->show();
    grid->addWidget(views[i], base / width, base % width);
    o << "..." << base << " - " << i << ":" 
      << base / width << "x" <<  base % width << endl;
  }
}

void DatasetBrowser::nextPage()
{
  currentIndex += width*height;
  int s = std::max(views.size() - width*height, 0);
  currentIndex = std::min(s, currentIndex);
  dataSetChanged(currentIndex);
}

void DatasetBrowser::previousPage()
{
  currentIndex -= width*height;
  currentIndex = std::max(0, currentIndex);
  dataSetChanged(currentIndex);
}
