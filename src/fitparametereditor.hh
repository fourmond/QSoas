/**
   \file fitparametereditor.hh
   Editor for a single fit parameter (one per dataset ?)
   Copyright 2011 by Vincent Fourmond

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

#ifndef __FITPARAMETEREDITOR_HH
#define __FITPARAMETEREDITOR_HH

#include <fitparameters.hh>

class FitData;
class CurveView;
class DataSet;
class ParameterDefinition;

/// A widget to edit the settings for a given parameter.
///
/// @todo Disable the global flag when there is only one dataset or
/// that the parameter can't be buffer-local.
class FitParameterEditor : public QWidget {
  Q_OBJECT;

  /// The index in the ParameterDefinition
  int index;

  /// The current dataset
  int dataset;

  const ParameterDefinition * def;

  /// The editor
  QLineEdit * editor;
  
  /// The global checkbox
  QCheckBox * global;

  /// The fixed checkbox
  QCheckBox * fixed;

  /// A pointer to the FitParameters object
  FitParameters * parameters;

  /// Set to true during updates to avoid infinite recursion
  bool updatingEditor;

  /// Whether the parameter is global or not
  bool isGlobal() const;

  /// Returns the target for the current conditions
  FitParameter *& targetParameter() {
    if(isGlobal())
      return parameters->parameter(index, 0);
    else
      return parameters->parameter(index, dataset);
  };

public:
  FitParameterEditor(const ParameterDefinition * d, int idx,
                     FitParameters * params);

public slots:
  void updateFromParameters();

  /// Changes the current dataset.
  void selectDataSet(int dsIndex);

protected slots:
  void onFixedClicked();
  void onGlobalClicked();
  void onValueChanged(const QString & str);

  void showEditor();
};


#endif
