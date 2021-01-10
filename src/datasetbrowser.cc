/*
  datasetbrowser.cc: Implementation of the dataset browser
  Copyright 2012, 2013 by CNRS/AMU

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
#include <nupwidget.hh>
#include <utils.hh>

#include <actioncombo.hh>

//////////////////////////////////////////////////////////////////////

static SettingsValue<QSize> browserSize("browser/size", QSize(700,500));


DatasetBrowser::DatasetBrowser() : extendedSelection(false)
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

  nup = new NupWidget;
  layout->addWidget(nup);
  nup->setNup(4,4);

  connect(nup, SIGNAL(pageChanged(int)), SLOT(pageChanged(int)));


  bottomLayout = new QHBoxLayout;


  // Two arrows at the left
  QPushButton * bt =
    new QPushButton(Utils::standardIcon(QStyle::SP_ArrowLeft),"");
  nup->connect(bt, SIGNAL(clicked()), SLOT(previousPage()));
  bottomLayout->addWidget(bt);
  bt = new QPushButton(Utils::standardIcon(QStyle::SP_ArrowRight),"");
  nup->connect(bt, SIGNAL(clicked()), SLOT(nextPage()));
  bottomLayout->addWidget(bt);

  // Stretch
  bottomLayout->addStretch(1);

  bufferDisplay = new QLabel("");
  bottomLayout->addWidget(bufferDisplay);


  bt = new QPushButton(Utils::standardIcon(QStyle::SP_DialogCloseButton),
                       "Close");
  connect(bt, SIGNAL(clicked()), SLOT(accept()));
  bottomLayout->addWidget(bt);

  QComboBox * cb = new QComboBox;
  cb->setEditable(true);
  cb->addItem("4 x 4");
  cb->addItem("3 x 3");
  cb->addItem("2 x 2");
  cb->addItem("6 x 6");
  cb->addItem("3 x 2");
  nup->connect(cb, SIGNAL(activated(const QString&)), 
               SLOT(setNup(const QString &)));
  bottomLayout->addWidget(cb);


  // Two arrows at the right
  bt = new QPushButton(Utils::standardIcon(QStyle::SP_ArrowLeft),"");
  nup->connect(bt, SIGNAL(clicked()), SLOT(previousPage()));
  bottomLayout->addWidget(bt);
  bt = new QPushButton(Utils::standardIcon(QStyle::SP_ArrowRight),"");
  nup->connect(bt, SIGNAL(clicked()), SLOT(nextPage()));
  bottomLayout->addWidget(bt);

  layout->addLayout(bottomLayout);


  // Shortcuts
  addAction(ActionCombo::createAction("Next", nup,
                                      SLOT(nextPage()),
                                      QKeySequence("Ctrl+PgDown"),
                                      this));
  addAction(ActionCombo::createAction("Next", nup,
                                      SLOT(previousPage()),
                                      QKeySequence("Ctrl+PgUp"),
                                      this));

}

void DatasetBrowser::pageChanged(int newpage)
{
  bufferDisplay->setText(QString("%1/%2").
                         arg(newpage+1).
                         arg(nup->totalPages()));
}

void DatasetBrowser::displayDataSets(const QList<const DataSet *> &ds,
                                     bool es)
{
  extendedSelection = es;
  nup->clear();
  selected.clear();
  int wd = nup->nupWidth();
  int ht = nup->nupHeight();
  nup->setNup(0,0);
  cleanupViews();
  datasets = ds;
  nup->setupGenerator([this] (int idx, int inW) -> QWidget * {
                        return viewForDataset(idx, inW);
                      }, datasets.size());
  nup->setNup(wd, ht);
  nup->showPage(0);
}

CheckableWidget * DatasetBrowser::viewForDataset(int index, int inWindow)
{
  while(views.size() <= inWindow)
    views << new CheckableWidget(new CurveView(this), this);
  views[inWindow]->subWidget<CurveView>()->showDataSet(datasets[index]);
  views[inWindow]->useSet(&selected, index);
  return views[inWindow];
}


void DatasetBrowser::displayDataSets(const QList<DataSet *> &ds, 
                                     bool es)
{
  QList<const DataSet * > ds2;
  for(int i = 0; i < ds.size(); i++)
    ds2 << ds[i];
  displayDataSets(ds2, es);
}


QList<const DataSet*> DatasetBrowser::selectedDatasets() const
{
  QList<const DataSet*> ret;
  for(int sel : selected)
    ret << datasets[sel];
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
  // 4 = 2 buttons + 1 stretch + label
  bottomLayout->insertWidget(4, button);
}
