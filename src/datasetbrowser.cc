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

#include <checkablewidget.hh>


//////////////////////////////////////////////////////////////////////

static SettingsValue<QSize> browserSize("browser/size", QSize(700,500));


DatasetBrowser::DatasetBrowser() : width(4), height(4), 
                                   extendedSelection(false)
{
  resize(browserSize);
  setupFrame();
  actionsMapper = new QSignalMapper;
  connect(actionsMapper, SIGNAL(mapped(int)),
          SLOT(runHook(int)));
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


  bottomLayout = new QHBoxLayout;
  QPushButton * bt = new QPushButton(tr("<-"));
  connect(bt, SIGNAL(clicked()), SLOT(previousPage()));
  bottomLayout->addWidget(bt);

  bufferDisplay = new QLabel("");
  bottomLayout->addWidget(bufferDisplay);


  bt = new QPushButton(tr("Close"));
  connect(bt, SIGNAL(clicked()), SLOT(accept()));
  bottomLayout->addWidget(bt);

  bt = new QPushButton(tr("->"));
  connect(bt, SIGNAL(clicked()), SLOT(nextPage()));
  bottomLayout->addWidget(bt);

  layout->addLayout(bottomLayout);

}

void DatasetBrowser::dataSetChanged(int newds)
{
  currentIndex = newds;
  if(currentIndex >= datasets.size())
    currentIndex = datasets.size() - 1;
  if(currentIndex < 0)
    currentIndex = 0;
  currentIndex = currentIndex - (currentIndex % (width * height));
  setupGrid();
  bufferDisplay->setText(QString("%1-%2/%3").
                         arg(currentIndex+1).
                         arg(currentIndex + height*width).
                         arg(datasets.size()));
}

void DatasetBrowser::displayDataSets(const QList<const DataSet *> &ds,
                                     bool es)
{
  extendedSelection = es;
  cleanupViews();
  datasets = ds;
  for(int i = 0; i < datasets.size(); i++) {
    CheckableWidget * cw = 
      new CheckableWidget(new CurveView, this);
    cw->subWidget<CurveView>()->showDataSet(datasets[i]);
    views << cw;
  }
  dataSetChanged(0);
}

void DatasetBrowser::displayDataSets(const QList<DataSet *> &ds, 
                                     bool es)
{
  QList<const DataSet * > ds2;
  for(int i = 0; i < ds.size(); i++)
    ds2 << ds[i];
  displayDataSets(ds2, es);
}

void DatasetBrowser::setupGrid()
{
  // Empty the grid first:
  QTextStream o(stdout);
  if(datasets.size() == 0)
    return;
  
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
  }
}

void DatasetBrowser::nextPage()
{
  dataSetChanged(currentIndex + width*height);
}

void DatasetBrowser::previousPage()
{
  dataSetChanged(currentIndex - width*height);
}


QList<const DataSet*> DatasetBrowser::selectedDatasets() const
{
  QList<const DataSet*> ret;
  for(int i = 0; i < views.size(); i++)
    if(views[i]->isChecked())
      ret << datasets[i];
  return ret;
}

void DatasetBrowser::runHook(int id)
{
  QList<const DataSet*> datasets = selectedDatasets();
  ActOnSelected hook = actionHooks.value(id, NULL);
  if(hook)
    hook(datasets);
  
}

void DatasetBrowser::addButton(QString name, ActOnSelected hook)
{
  QPushButton * button = new QPushButton(name);
  actionsMapper->connect(button, SIGNAL(clicked()),
                        SLOT(map()));
  actionsMapper->setMapping(button, actionHooks.size());
  actionHooks << hook;
  bottomLayout->insertWidget(2, button);
}
