/*
  synchronizedtables.cc: implementation of SynchronizedTables
  Copyright 2023 by CNRS/AMU

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
#include <synchronizedtables.hh>

SynchronizedTables::SynchronizedTables(QWidget * parent) :
  QSplitter(Qt::Vertical, parent)
{
}

int SynchronizedTables::addWidget(QWidget * widget, int region,
                                  int stretch)
{
  if(region < 0)
    region = splitterWidgets.size();
  while(splitterWidgets.size() <= region) {
    QWidget * w = new QWidget;
    QSplitter::addWidget(w);
    splitterWidgets << w;
    layouts << new QVBoxLayout(w);
  }
  layouts[region]->addWidget(widget, stretch);
  allWidgets << widget;
  return region;
}

int SynchronizedTables::addTable(QTableView * table, int region,
                                 int stretch)
{
  QTableView * ref = NULL;
  if(tables.size() > 0)
    ref = tables.first();

  int rv = addWidget(table, region, stretch);
  if(ref) {
    for(int i = 0; i < ref->horizontalHeader()->count(); i++) {
      if(ref->isColumnHidden(i))
        table->hideColumn(i);
      else
        table->showColumn(i);
      table->setColumnWidth(i, ref->columnWidth(i));
    }
  }
  connect(table->horizontalHeader(),
          SIGNAL(sectionResized(int, int, int)),
          SLOT(columnResized(int, int, int)));

  // table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  // table->horizontalScrollBar()->setEnabled(false);
  connect(table->horizontalScrollBar(),
          SIGNAL(valueChanged(int)),
          SLOT(tableSliderChanged(int)));
  tables << table;

  return rv;
}

void SynchronizedTables::hideColumn(int col)
{
  for(QTableView * table : tables)
    table->hideColumn(col);
}

void SynchronizedTables::showColumn(int col)
{
  for(QTableView * table : tables)
    table->showColumn(col);
}

void SynchronizedTables::tableSliderChanged(int value)
{
  for(QTableView * v : tables)
    v->horizontalScrollBar()->setValue(value);
}


void SynchronizedTables::columnResized(int column, int oldSize, int newSize)
{
  if(oldSize == newSize)
    return;
  for(QTableView * v : tables)
    v->setColumnWidth(column, newSize);
}
