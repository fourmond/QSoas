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
  datasets.clear();
}

void DatasetBrowser::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);

  list = new QListWidget;
  list->setViewMode(QListView::IconMode);
  list->setMovement(QListView::Static);
  list->setResizeMode(QListView::Adjust);
  list->setIconSize(QSize(300,300));
  list->setSelectionMode(QAbstractItemView::ExtendedSelection);
  layout->addWidget(list);


  bottomLayout = new QHBoxLayout;


  sizeCombo = new QComboBox;
  sizeCombo->addItem("Tiny", 100);
  sizeCombo->addItem("Small", 200);
  sizeCombo->addItem("Medium", 350);
  sizeCombo->addItem("Large", 500);
  sizeCombo->addItem("Huge", 800);
  connect(sizeCombo, SIGNAL(activated(int)),
          SLOT(comboChangedSize(int)));
  sizeCombo->setCurrentIndex(2); // medium ?


  bottomLayout->addWidget(sizeCombo);

  QDialogButtonBox * buttons = new QDialogButtonBox(QDialogButtonBox::Close);
  bottomLayout->addSpacing(1);
  bottomLayout->addWidget(buttons);
  connect(buttons, SIGNAL(rejected()), SLOT(reject()));

  layout->addLayout(bottomLayout);

}

void DatasetBrowser::pageChanged(int newpage)
{
}

void DatasetBrowser::comboChangedSize(int idx)
{
  int sz = sizeCombo->itemData(idx).toInt();
  if(sz > 0)
    list->setIconSize(QSize(sz,sz));
}

void DatasetBrowser::displayDataSets(const QList<const DataSet *> &ds,
                                     bool es)
{
  extendedSelection = es;
  list->clear();
  cleanupViews();
  datasets = ds;
  for(int i = 0; i < datasets.size(); i++) {
    QListWidgetItem * item = new QListWidgetItem(datasets[i]->name);
    item->setData(Qt::UserRole, i);
    QIcon icn;
    icn.addPixmap(CurveView::renderDatasetAsPixmap(datasets[i], QSize(200,200)));
    icn.addPixmap(CurveView::renderDatasetAsPixmap(datasets[i], QSize(400,400)));
    icn.addPixmap(CurveView::renderDatasetAsPixmap(datasets[i], QSize(800,800)));
    
    item->setIcon(icn);
    list->addItem(item);
  }
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
  QList<QListWidgetItem *> sel = list->selectedItems();
  for(int i = 0; i < sel.size(); i++)
    ret << datasets[sel[i]->data(Qt::UserRole).toInt()];
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
  bottomLayout->insertWidget(0, button);
}
