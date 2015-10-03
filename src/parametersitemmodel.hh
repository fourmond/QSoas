/**
   \file parametersitemmodel.hh
   MVC model for displaying parameters
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
#ifndef __PARAMETERSITEMMODEL_HH
#define __PARAMETERSITEMMODEL_HH

class FitWorkspace;


/// A table model for displaying
///
/// @todo Add Z coordinates, choice of meta...
class ParametersItemModel : public QAbstractTableModel {

  /// The workspace.
  FitWorkspace * workspace;

  /// Icons to distinguish between free and fixed parameters;
  QIcon freeIcon;
  QIcon fixedIcon;

  /// Whether some modifications have occurred
  bool modified;

public:

  /// Constructs the model
  ParametersItemModel(FitWorkspace * ws, QObject * parent = NULL);

  /// True if the model was used to change the underlying data.
  bool dataChanged() const; 

  /// Do not hide the base class...
  using QAbstractItemModel::dataChanged;



  /// @name Reimplemented interface
  ///
  /// @{
  virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;

  virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;

  virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;


  virtual bool setData(const QModelIndex & index,
                       const QVariant & value, int role);

  virtual QVariant headerData(int section, Qt::Orientation orientation,
                              int role) const;
  
  virtual Qt::ItemFlags flags(const QModelIndex & index) const;
  /// @}
};


#endif
