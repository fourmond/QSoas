/*
  dataseteditor.cc: Implementation of the dataset editor
  Copyright 2013,2023,2024 by CNRS/AMU

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

  /// If true, we are editing the names of rows and columns,
  /// and the perpendicular coordinates
  bool editingNames;

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

  void setEditingNames(bool en) {
    if(en != editingNames) {
      beginResetModel();
      editingNames = en;
      endResetModel();
    }
  };

  /// Returns true if the dataset was modified since the last call to
  /// clearModified()
  bool isModified() const {
    return modified;
  }

  /// Clears the "modified" state.
  void clearModified() {
    modified = false;
  };

  static void columnName(const DataSet * ds, int column,
                         QString * standard,
                         QString * real) {
    if(column >= 0 && column < ds->nbColumns()) {
      *standard = DataSet::standardNameForColumn(column);
      if(ds->columnNames.size() > 0 && ds->columnNames[0].size() > column)
        *real = ds->columnNames[0][column];
      else
        *real = QString();
    }
    else {
      *standard = QString();
      *real = QString();
    }
      
  };

  static QString rowName(const DataSet * ds, int row) {
    if(ds->rowNames.size() > 0 && ds->rowNames[0].size() > row)
      return ds->rowNames[0][row];
    return QString();
  };

  explicit DataSetTableModel(const DataSet * ds = NULL) :
    source(ds), modifiedDataSet(NULL), modified(false),
    editingNames(false)
  {
    if(! source) {
      Vector v(1,1);
      modifiedDataSet = new DataSet(QList<Vector>() << v << v);
      modifiedDataSet->name = "edited.dat";
    }
  };

  ~DataSetTableModel() {
    delete modifiedDataSet;
  };

  /// @name Reimplemented interface
  ///
  /// @{
  virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override {
    if(editingNames)
      return currentDataSet()->nbRows() + 2;
    else
      return currentDataSet()->nbRows();
  }

  virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override {
    if(editingNames)
      return currentDataSet()->nbColumns() + 1;
    else
      return currentDataSet()->nbColumns();
  };

  virtual QVariant data(const QModelIndex & index, int role) const override {
    int col = index.column();
    int row = index.row();
    const DataSet * ds = currentDataSet();
    if(editingNames) {
      col -= 1;
      row -= 2;
      if(row == -2) {
        if(col == -1)
          return QVariant();
        if(role == Qt::DisplayRole || role == Qt::EditRole) {
          QString cn, rn;
          columnName(ds, col, &cn, &rn);
          if(! rn.isEmpty())
            return rn;
          if(role == Qt::EditRole)
            return cn;
          return QString("(%1)").arg(cn);
        }
      }
      if(row == -1) {
        if(col <= 0)
          return QVariant();
        if(role == Qt::DisplayRole || role == Qt::EditRole) {
          Vector pc = ds->perpendicularCoordinates();
          if(pc.size() > col-1)
            return QString::number(pc[col-1]);
        }
      }
      if(col == -1) {
        if(role == Qt::DisplayRole || role == Qt::EditRole) {
          QString rn = rowName(ds, row);
          if(! rn.isEmpty())
            return rn;
        }
        return QVariant();
      }
    }
    
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

  void setColumnName(int column, const QString & name) {
    modify();
    modifiedDataSet->setColumnName(column, name);
    if(editingNames)
      column += 1;
    emit(headerDataChanged(Qt::Horizontal, column, column));
  };

  void setRowName(int row, const QString & name) {
    modify();
    modifiedDataSet->setRowName(row, name);
    if(editingNames)
      row += 2;
    emit(headerDataChanged(Qt::Vertical, row, row));
  };

  void setPerpendicularCoordinate(int column, double value) {
    modify();
    if(column <= 0 || column >= modifiedDataSet->nbColumns())
      return;
    Vector pc = modifiedDataSet->perpendicularCoordinates();
    if(pc.size() != modifiedDataSet->nbColumns() - 1) {
      pc.clear();
      for(int i = 0; i < modifiedDataSet->nbColumns() - 1; i++)
        pc << i;
    }
    pc[column-1] = value;
    modifiedDataSet->setPerpendicularCoordinates(pc);
    if(editingNames)
      column += 1;
    emit(headerDataChanged(Qt::Horizontal, column, column));
  };
  
  virtual bool setData(const QModelIndex & index,
               const QVariant & value, int role) override {
    if(role != Qt::EditRole)
      return false;

    int col = index.column();
    int row = index.row();

    const DataSet * ds = currentDataSet();
    
    if(editingNames) {
      col -= 1;
      row -= 2;
      if(row == -2) {
        if(col == -1)
          return false;
        setColumnName(col, value.toString());
        emit(dataChanged(index, index));
        modified = true;
        return true;
      }
      if(row == -1) {
        if(col <= 0)
          return false;
        setPerpendicularCoordinate(col, value.toDouble());
        emit(dataChanged(index, index));
        modified = true;
        return true;
      }
      if(col == -1) {
        setRowName(row, value.toString());
        emit(dataChanged(index, index));
        modified = true;
        return true;
      }
    }

    
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
      if(editingNames)
        section -= 1;
      if(role == Qt::DisplayRole) {
        const DataSet * ds = currentDataSet();
        Vector pc = ds->perpendicularCoordinates();
        if(section == -1 && editingNames)
          return "Row name";
        if(section >= 0 && section < ds->nbColumns()) {
          QString cn, rn;
          columnName(ds, section, &cn, &rn);
          if(! rn.isEmpty())
            cn = QString("%1 (%2)").
              arg(rn).arg(cn);
          if(pc.size() >= section && section >= 1)
            cn += QString("\nZ = %1").arg(pc[section - 1]);
          return cn;
        }
      }
      return QVariant();
    }
    if(orientation == Qt::Vertical) {
      if(editingNames)
        section -= 2;
      if(role == Qt::DisplayRole) {
        const DataSet * ds = currentDataSet();
        if(section == -2 && editingNames)
          return "Column name";
        if(section == -1 && editingNames)
          return "Z";
        if(section >= 0 && section < ds->nbRows()) {
          QString n = QString("#%1").arg(section);
          QString rn = rowName(ds, section);
          if(! rn.isEmpty())
            n += ": " + Utils::shortenString(rn, 40);
          return n;
        }
      }
    }
    return QVariant();
  }
  /// @}

  /// @name Modification interface
  ///
  /// @{

  /// Insert @a nb columns to the left or to the right of the given
  /// column.

  void insertColumns(const QModelIndex & index, bool left, int nb = 1) {
    int col = index.column();
    const DataSet * ds = currentDataSet();
    if(editingNames)
      col -= 1;
    if(col < 0)
      col = 0;
    if(col >= ds->nbColumns())
      col = ds->nbColumns() - 1;
    modify();
    modified = true;
    beginInsertColumns(QModelIndex(), col, col + nb - 1);
    Vector v = modifiedDataSet->x();
    v *= 0;
    if(! left)
      col += 1;
    for(int i = 0; i < nb; i++)
      modifiedDataSet->insertColumn(col, v);
    endInsertColumns();
  };

  void insertRows(const QModelIndex & index, bool above, int nb = 1) {
    int row = index.row();
    const DataSet * ds = currentDataSet();
    if(editingNames)
      row -= 2;
    if(row < 0)
      row = 0;
    if(row >= ds->nbRows())
      row = ds->nbRows() - 1;
    modify();
    modified = true;
    beginInsertRows(QModelIndex(), row, row + nb - 1);
    if(! above)
      row += 1;
    for(int i = 0; i < nb; i++)
      modifiedDataSet->insertRow(row, 0);
    endInsertRows();
  };

  void removeRow(const QModelIndex & index) {
    int row = index.row();
    const DataSet * ds = currentDataSet();
    if(editingNames)
      row -= 2;
    if(row < 0 || row >= ds->nbRows())
      return;
    modify();
    modified = true;
    beginRemoveRows(QModelIndex(), row, row);
    modifiedDataSet->removeRow(row);
    endRemoveRows();
  };

  void removeColumn(const QModelIndex & index) {
    int column = index.column();
    const DataSet * ds = currentDataSet();
    if(editingNames)
      column -= 1;
    if(column < 0 || column >= ds->nbColumns())
      return;
    modify();
    modified = true;
    beginRemoveColumns(QModelIndex(), column, column);
    modifiedDataSet->takeColumn(column);
    endRemoveColumns();
  };

    

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

  table->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(table,
          SIGNAL(customContextMenuRequested(const QPoint&)),
          SLOT(contextMenuOnTable(const QPoint&)));

  QHBoxLayout * sub = new QHBoxLayout();

  layout->addLayout(sub);

  QCheckBox * cb = new QCheckBox("Edit names/Z");
  connect(cb, &QCheckBox::stateChanged, this,
          [this] (int state) {
            model->setEditingNames(state == Qt::Checked);
          });
  sub->addWidget(cb);
  
  sub->addStretch(1);

  QPushButton * bt = new QPushButton("Push new");
  connect(bt, SIGNAL(clicked()), SLOT(pushToStack()));
  sub->addWidget(bt);

  bt = new QPushButton("Close");
  connect(bt, &QPushButton::clicked, this, &DatasetEditor::reject);
  sub->addWidget(bt);
}

void DatasetEditor::reject()
{
  if(model->isModified()) {
    if(! Utils::askConfirmation("The dataset has unpushed "
                                "modifications, close anyway ?"))
      return;
  }
  QDialog::reject();
}

void DatasetEditor::pushToStack()
{
  soas().pushDataSet(model->currentDataSet()->
                     derivedDataSet(".dat"));
  model->clearModified();
}

void DatasetEditor::contextMenuOnTable(const QPoint& pos)
{
  QMenu menu;

  auto addAction = [this] (QMenu * menu,
                           const QString & str,
                           std::function<void ()> fn) {
    QAction * action = new QAction(str);
    QObject::connect(action, &QAction::triggered, this, fn);
    menu->addAction(action);
  };

  QMenu sub("Insert Column");
  addAction(&sub, "Left", [this] {
    model->insertColumns(table->currentIndex(), true);
  });
  addAction(&sub, "Right", [this] {
    model->insertColumns(table->currentIndex(), false);
  });

  menu.addMenu(&sub);

  QMenu sub2("Insert Row");
  addAction(&sub2, "Above", [this] {
    model->insertRows(table->currentIndex(), true);
  });
  addAction(&sub2, "Below", [this] {
    model->insertRows(table->currentIndex(), false);
  });

  menu.addMenu(&sub2);

  QMenu sub3("Remove");
  addAction(&sub3, "Current row", [this] {
    model->removeRow(table->currentIndex());
  });
  addAction(&sub3, "Current column", [this] {
    model->removeColumn(table->currentIndex());
  });

  menu.addMenu(&sub3);

  menu.exec(table->viewport()->mapToGlobal(pos));
}
