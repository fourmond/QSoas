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

#include <soas.hh>


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

void DatasetEditor::setupTable(QTableWidget * table, const DataSet * ds)
{
  QStringList cn = ds->columnNames();
  table->setColumnCount(cn.size());

  int rows = ds->nbRows();
  table->setRowCount(rows);

  for(int i = 0; i < cn.size(); i++) {
    table->setHorizontalHeaderItem(i, new QTableWidgetItem(cn[i]));
    
    const Vector & c = ds->column(i);
    for(int j = 0; j < rows; j++) {
      if(! i)
        table->
          setVerticalHeaderItem(j, new QTableWidgetItem(QString("#%1").arg(j)));
      table->setItem(j, i, new QTableWidgetItem(QString::number(c[j])));
    }
  }
}

void DatasetEditor::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);

  metaEditor = new ValueHashEditor();
  layout->addWidget(metaEditor);
  
  metaEditor->setFromHash(source->getMetaData());

  table = new QTableWidget;
  layout->addWidget(table, 1);

  setupTable(table, source);
  connect(table, SIGNAL(itemChanged(QTableWidgetItem *)), 
          SLOT(itemEdited(QTableWidgetItem *)));
  
  QHBoxLayout * sub = new QHBoxLayout();

  layout->addLayout(sub);

  QPushButton * bt = new QPushButton("Push new");
  connect(bt, SIGNAL(clicked()), SLOT(pushToStack()));
  sub->addWidget(bt);

  bt = new QPushButton("Close");
  connect(bt, SIGNAL(clicked()), SLOT(close()));
  sub->addWidget(bt);
}

void DatasetEditor::itemEdited(QTableWidgetItem * it)
{
  it->setBackground(QBrush(QColor("#FDD"))); // light red
  it->setData(Qt::UserRole, QVariant(true));
}

void DatasetEditor::pushToStack()
{
  QList<Vector> vects;
  int cols = source->nbColumns();
  int rows = source->nbRows();

  for(int i = 0; i < cols; i++) {
    Vector v(source->column(i));
    for(int j = 0; j < rows; j++) {
      QTableWidgetItem * it = table->item(j, i);
      if(it->data(Qt::UserRole).toBool())
        v[j] = it->text().toDouble();
    }
    vects << v;
  }
  soas().pushDataSet(source->derivedDataSet(vects, "_edited.dat"));
}
