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

class DataSetListModel : public QStringListModel {
  QList<const DataSet*> datasets;

  mutable QCache<int, QIcon> cachedIcons;

  int curSize;
  
public:

  /// @todo Make cache size customizable 
  DataSetListModel() : cachedIcons(100 * 64) {
    
  }

  void setDataSetList(const QList<const DataSet*> & lst) {
    QStringList names;
    datasets = lst;
    for(int i = 0; i < datasets.size(); i++)
      names << datasets[i]->name;
    setStringList(names);
  };

  QVariant data(const QModelIndex & index, int role) const override {
    if(! index.isValid())
      return QVariant();
    if(role == Qt::DecorationRole) {
      int i = index.row();
      if(cachedIcons.contains(i))
        return *cachedIcons[i];
      QIcon * icn = new QIcon;
      icn->addPixmap(CurveView::renderDatasetAsPixmap(datasets[i], QSize(200,200)));
      if(curSize > 200)
        icn->addPixmap(CurveView::renderDatasetAsPixmap(datasets[i], QSize(400,400)));
      if(curSize > 400)
        icn->addPixmap(CurveView::renderDatasetAsPixmap(datasets[i], QSize(800,800)));
      int cst = curSize/100;
      cst *= cst;
      cachedIcons.insert(i, icn, cst);
      // QTextStream o(stdout);
      // o << "Making icon for : " << datasets[i]->name << endl;
      return *icn;
    }
    else
      return QStringListModel::data(index, role);
  };

  void setCurSize(int sz) {
    if(sz <= 200)
      sz = 200;
    else if(sz <= 400)
      sz = 400;
    else
      sz = 800;
    if(sz > curSize)
      cachedIcons.clear();
    curSize = sz;
  };

  QList<const DataSet * > indexedDatasets(const QModelIndexList & list) const {
    QList<const DataSet * > sel;
    for(auto i : list)
      sel << datasets[i.row()];
    return sel;
  }
};






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

  list = new QListView;
  list->setViewMode(QListView::IconMode);
  list->setMovement(QListView::Static);
  list->setResizeMode(QListView::Adjust);
  list->setIconSize(QSize(300,300));
  list->setSelectionMode(QAbstractItemView::ExtendedSelection);
  list->setUniformItemSizes(true);
  layout->addWidget(list);

  model = new DataSetListModel;
  list->setModel(model);


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
  if(sz > 0) {
    model->setCurSize(sz);
    list->setIconSize(QSize(sz,sz));
  }
}

void DatasetBrowser::displayDataSets(const QList<const DataSet *> &ds,
                                     bool es)
{
  extendedSelection = es;
  cleanupViews();
  datasets = ds;
  model->setDataSetList(ds);
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
  return model->indexedDatasets(list->selectionModel()->selectedIndexes());
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
