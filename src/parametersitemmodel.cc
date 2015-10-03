/*
  parametersitemmodel.cc: implementation of the item model
  Copyright 2015 by CNRS/AMU

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
#include <parametersitemmodel.hh>

#include <fitworkspace.hh>
#include <fit.hh>
#include <fitdata.hh>
#include <dataset.hh>
#include <utils.hh>

ParametersItemModel::ParametersItemModel(FitWorkspace * ws, QObject * parent) :
  QAbstractTableModel(parent), workspace(ws), modified(false)
{
  // Very crappy way to display fixed/free status...
  //
  // Todo: I should rather use the image of a lock for that, I think.
  QPixmap px(20,20);

  px.fill(QColor(0,128,0));
  freeIcon = QIcon(px);

  px.fill(QColor(255,0,0));
  fixedIcon = QIcon(px);

}

int ParametersItemModel::rowCount(const QModelIndex & parent) const
{
  return workspace->data()->datasets.size();
}

int ParametersItemModel::columnCount(const QModelIndex & parent) const
{
  return workspace->data()->parameterDefinitions.size();
}

QVariant ParametersItemModel::data(const QModelIndex & index, int role) const
{
  ///
  switch(role) {
  case Qt::DisplayRole:
    // Write formula...
    return workspace->getValue(index.column(), index.row());
  case Qt::DecorationRole:
    return (workspace->isFixed(index.column(), index.row()) ?
            fixedIcon : freeIcon);
  case Qt::ForegroundRole:
    if(workspace->isGlobal(index.column()))
      return QColor(128,128,128);
  }
  
  return QVariant();
}

Qt::ItemFlags ParametersItemModel::flags(const QModelIndex & index) const
{
  return Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled;

}

QVariant ParametersItemModel::headerData(int section,
                                         Qt::Orientation orientation,
                                         int role) const
{
  if(orientation == Qt::Horizontal) {
    const QList<ParameterDefinition> & defs =
      workspace->data()->parameterDefinitions;
    if(section >= defs.size())
      return QVariant();
    if(role == Qt::DisplayRole) {
      return defs[section].name;
    }
  }
  else {
    const QList<const DataSet * > & ds =
      workspace->data()->datasets;
    if(section >= ds.size())
      return QVariant();
    if(role == Qt::DisplayRole) {
      /// @todo Get the dataset index in the stack
      return Utils::abbreviateString(ds[section]->name, 30);
    }
    if(role == Qt::ToolTipRole)
      return ds[section]->name;
  }
  return QVariant();
}
