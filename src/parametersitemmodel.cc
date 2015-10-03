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

bool ParametersItemModel::dataChanged() const
{
  return modified;
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
  case Qt::EditRole:
    return workspace->getTextValue(index.column(), index.row());
  case Qt::DecorationRole:
    return (workspace->isFixed(index.column(), index.row()) ?
            fixedIcon : freeIcon);
  case Qt::ForegroundRole:
    if(workspace->isGlobal(index.column()))
      return QColor(128,128,128);
  }
  
  return QVariant();
}

bool ParametersItemModel::setData(const QModelIndex & index,
                                      const QVariant & value, int role)
{
  if(role != Qt::EditRole)
    return false;
  QString s = value.toString();
  int idx = index.column();
  workspace->setValue(idx, index.row(), s);
  if(workspace->isGlobal(idx))
    emit(dataChanged(index.sibling(0, idx), index.sibling(rowCount() - 1, idx)));
  else
    emit(dataChanged(index, index));
  modified = true;
  return true;
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

void ParametersItemModel::setFixed(const QList<QModelIndex> items, bool fixed)
{
  if(items.size() > 0)
    modified = true;

  for(int i = 0; i < items.size(); i++) {
    workspace->setFixed(items[i].column(), items[i].row(), fixed);
    emit(dataChanged(items[i], items[i]));
  }

}

void ParametersItemModel::resetValuesToInitialGuess(const QList<QModelIndex> items)
{
  const FitData * d = workspace->data();
  int nbParameters = d->parameterDefinitions.size();
  double values[nbParameters * d->datasets.size()];
  /// @todo That's an ugly const_cast.
  d->fit->initialGuess(const_cast<FitData*>(d), values);

  for(int i = 0; i < items.size(); i++) {
    int ds = items[i].row();
    int idx = items[i].column();
    workspace->setValue(idx, ds, values[idx + nbParameters * ds]);
    emit(dataChanged(items[i], items[i]));
  }
}
