/*
  dataseteditor.cc: Implementation of the dataset editor
  Copyright 2013 by CNRS/AMU

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
#include <dataseteditor.hh>
#include <dataset.hh>
#include <settings-templates.hh>

#include <valuehasheditor.hh>
#include <utils.hh>

#include <soas.hh>


class DataSetTableModel : public QAbstractTableModel {
protected:

  const DataSet * source;

  DataSet * modifiedDataSet;


  bool modified;

  void modify() {
    if(! modifiedDataSet)
      modifiedDataSet = source->derivedDataSet(".dat");
  };

public:

  /// Returns the currently displayed dataset.
  const DataSet * currentDataSet() const {
    if(modifiedDataSet)
      return modifiedDataSet;
    return source;
  };


  explicit DataSetTableModel(const DataSet * ds = NULL) :
    source(ds), modifiedDataSet(NULL), modified(false)
  {
    if(! source) {
      Vector v(1,1);
      modifiedDataSet = new DataSet(QList<Vector>() << v << v);
    }
  };

  ~DataSetTableModel() {
    delete modifiedDataSet;
  };

  /// @name Reimplemented interface
  ///
  /// @{
  virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override {
    return currentDataSet()->nbRows();
  }

  virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override {
    return currentDataSet()->nbColumns();
  };

  virtual QVariant data(const QModelIndex & index, int role) const override {
    int col = index.column();
    int row = index.row();
    const DataSet * ds = currentDataSet();
    if(col < 0 || col >= ds->nbColumns())
      return QVariant();
    if(row < 0 || row >= ds->nbRows())
      return QVariant();
    if(role == Qt::DisplayRole || role == Qt::EditRole) {
      return QString::number(ds->column(col)[row]);
    }
    return QVariant();
  };

  virtual Qt::ItemFlags flags(const QModelIndex & /*index*/) const override {
    return Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled;
  };

  virtual bool setData(const QModelIndex & index,
               const QVariant & value, int role) override {
    if(role != Qt::EditRole)
      return false;

    int col = index.column();
    int row = index.row();
    const DataSet * ds = currentDataSet();
    if(col < 0 || col >= ds->nbColumns())
      return false;
    if(row < 0 || row >= ds->nbRows())
      return false;

    modify();
    modifiedDataSet->column(col)[row] = value.toDouble();
    emit(dataChanged(index, index));
    modified = true;
    return true;
  };


  virtual QVariant headerData(int section, Qt::Orientation orientation,
                              int role) const override {
    if(orientation == Qt::Horizontal) {
      if(role == Qt::DisplayRole) {
        const DataSet * ds = currentDataSet();
        if(section >= 0 && section < ds->nbColumns()) {
          QString cn = DataSet::standardNameForColumn(section);
          if(ds->columnNames.size() > 0 && ds->columnNames[0].size() > section)
            cn = QString("%1 (%2)").
              arg(ds->columnNames[0][section]).arg(cn);
          return cn;
        }
      }
      return QVariant();
    }
    if(orientation == Qt::Vertical) {
      if(role == Qt::DisplayRole) {
        const DataSet * ds = currentDataSet();
        if(section >= 0 && section < ds->nbRows()) {
          QString n = QString("#%1").arg(section);
          if(ds->rowNames.size() > 0 && ds->rowNames[0].size() > section)
            n += ": " + Utils::shortenString(ds->rowNames[0][section], 40);
          return n;
        }
      }
    }
    return QVariant();
  }
  /// @}

};


//////////////////////////////////////////////////////////////////////

static SettingsValue<QSize> editorSize("editor/size", QSize(700,500));


DatasetEditor::DatasetEditor(const DataSet * ds) : 
  source(ds)
{
  resize(editorSize);
  setupFrame();
}

DatasetEditor::~DatasetEditor()
{
  editorSize = size();
}

void DatasetEditor::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);

  table = new QTableView;
  layout->addWidget(table, 1);
  model = new DataSetTableModel(source);
  table->setModel(model);
  
  QHBoxLayout * sub = new QHBoxLayout();

  layout->addLayout(sub);
  sub->addStretch(1);

  QPushButton * bt = new QPushButton("Push new");
  connect(bt, SIGNAL(clicked()), SLOT(pushToStack()));
  sub->addWidget(bt);

  bt = new QPushButton("Close");
  connect(bt, SIGNAL(clicked()), SLOT(close()));
  sub->addWidget(bt);
}

void DatasetEditor::pushToStack()
{
  soas().pushDataSet(model->currentDataSet()->
                     derivedDataSet(".dat"));
}
