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

  freeIcon = Utils::standardIcon(Utils::FreeParameterIcon);
  fixedIcon = Utils::standardIcon(Utils::FixedParameterIcon);
}

bool ParametersItemModel::dataChanged() const
{
  return modified;
}

int ParametersItemModel::rowCount(const QModelIndex & /*parent*/) const
{
  return workspace->data()->datasets.size();
}

int ParametersItemModel::columnCount(const QModelIndex & /*parent*/) const
{
  return workspace->data()->parameterDefinitions.size() + 3;
}

int ParametersItemModel::parameterIndex(int idx) const
{
  int defs = workspace->data()->parameterDefinitions.size();
  idx--;
  if(idx >= defs)
    idx = defs - idx - 2;
  return idx;
}

int ParametersItemModel::parameterIndex(const QModelIndex & idx) const
{
  return parameterIndex(idx.column());
}

QVariant ParametersItemModel::data(const QModelIndex & index, int role) const
{
  int idx = parameterIndex(index);
  int row = index.row();
  // QTextStream o(stdout);
  // o << "Info: " << idx << ", " << row << " -- " << role << endl;;
  if(idx == -1) {
    const QList<const DataSet * > & ds =
      workspace->data()->datasets;
    switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      /// @todo Get the dataset index in the stack
      return ds[row]->name;
    }
    return QVariant();
  }
  if(idx == -2) {
    switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      /// @todo Get the dataset index in the stack
      if(workspace->perpendicularCoordinates.size() > row)
        return workspace->perpendicularCoordinates[row];
    }
    return QVariant();
  }
  if(idx == -3) {
    switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return workspace->getBufferWeight(row);
    }
    return QVariant();
  }
  // Fallover
  if(idx < 0)
    return QVariant();
  switch(role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    return workspace->getTextValue(idx, row);
  case Qt::DecorationRole:
    return (workspace->isFixed(idx, row) ?
            fixedIcon : freeIcon);
  case Qt::ForegroundRole:
    if(workspace->isGlobal(idx))
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
  int idx = parameterIndex(index.column());
  if(idx == -3) {
    bool ok = false;
    double flv = s.toDouble(&ok);
    if(ok) {
      workspace->setBufferWeight(index.row(), flv);
      emit(dataChanged(index, index));
      modified = true;
      return true;
    }
  }
  
  if(idx < 0)
    return false;
  workspace->setValue(idx, index.row(), s);
  if(workspace->isGlobal(idx))
    emit(dataChanged(index.sibling(0, index.column()),
                     index.sibling(rowCount() - 1, index.column())));
  else
    emit(dataChanged(index, index));
  modified = true;
  return true;
}

Qt::ItemFlags ParametersItemModel::flags(const QModelIndex & index) const
{
  int idx = parameterIndex(index);
  if(idx >= 0 || idx == -3)
    return Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled;
  return Qt::ItemIsEnabled;
}

QVariant ParametersItemModel::headerData(int section,
                                         Qt::Orientation orientation,
                                         int role) const
{
  if(orientation == Qt::Horizontal) {
    section = parameterIndex(section);
    if(section == -1)
      return QVariant("Buffer");
    if(section == -2)
      return QVariant("Z");
    if(section == -3)
      return QVariant("weight");
    const QList<ParameterDefinition> & defs =
      workspace->data()->parameterDefinitions;
    if(section >= defs.size())
      return QVariant();
    if(role == Qt::DisplayRole) {
      return defs[section].name;
    }
  }
  else {
    if(role == Qt::DisplayRole) {
      return QString("#%1").arg(section);
    }
  }
  return QVariant();
}

void ParametersItemModel::setFixed(const QList<QModelIndex> items, bool fixed)
{
  if(items.size() > 0)
    modified = true;

  for(int i = 0; i < items.size(); i++) {
    int idx = parameterIndex(items[i]);
    if(idx >= 0) {
      workspace->setFixed(idx, items[i].row(), fixed);
      emit(dataChanged(items[i], items[i]));
    }
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
    int idx = parameterIndex(items[i]);
    if(idx >= 0) {
      workspace->setValue(idx, ds, values[idx + nbParameters * ds]);
      emit(dataChanged(items[i], items[i]));
    }
  }
}
