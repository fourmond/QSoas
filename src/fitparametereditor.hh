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
  int index;
  const ParameterDefinition * def;

  /// The editor
  QLineEdit * editor;
  
  /// The global checkbox
  QCheckBox * global;

  /// The fixed checkbox
  QCheckBox * fixed;

public:
  FitParameterEditor(const ParameterDefinition * d, int idx, 
                     int totalDatasets, int totalParams);

  /// Set the editor values.
  void setValues(double value, bool fixed, bool global);

  /// Whether the parameter is global
  bool isGlobal() const {
    return global->checkState() == Qt::Checked;
  };

  /// Whether the parameter is fixed
  bool isFixed() const {
    return fixed->checkState() == Qt::Checked;
  };

signals:
  void fixedChanged(int index, bool fixed);
  void globalChanged(int index, bool global);
  void valueChanged(int index, double value);

protected slots:
  void onFixedClicked();
  void onGlobalClicked();
  void onValueChanged(const QString & str);
};


#endif
