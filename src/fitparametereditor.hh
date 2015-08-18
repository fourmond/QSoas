/**
   \file fitparametereditor.hh
   Editor for a single fit parameter (one per dataset ?)
   Copyright 2011 by Vincent Fourmond
             2012 by CNRS/AMU

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
#ifndef __FITPARAMETEREDITOR_HH
#define __FITPARAMETEREDITOR_HH

#include <fitworkspace.hh>

class FitData;
class CurveView;
class DataSet;
class ParameterDefinition;
class BijectionFactoryItem;

/// A widget to edit the settings for a given parameter.
///
/// @todo There may be a way to force the Editor to add all its
/// elements to a GridLayout -- or to write a horizontal layout
/// connected to others (but HOW ?)
class FitParameterEditor : public QWidget {
  Q_OBJECT;

  /// The internal layout
  QHBoxLayout * layout;

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

  /// A pointer to the FitWorkspace object
  FitWorkspace * parameters;

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

  /// Whether the parameter is global or not
  bool isFixed() const {
    if(isGlobal())
      return parameters->isFixed(index, 0);
    else
      return parameters->isFixed(index, dataset);
  };

  /// Whether the editor also propose the edition of the Bijection
  bool extended;


  /// The combo box for edition of Bijection
  QComboBox * bijectionCombo;

  /// The labels for the bijection parameters
  QList<QLabel*> bijectionParameterLabels;

  /// The editors for the bijection parameters
  QList<QLineEdit*> bijectionParameterEditors;

  
  /// All the available bijections
  QList<const BijectionFactoryItem *> availableBijections;

  /// All the widgets that must be turned off 
  QList<QWidget*> bijectionWidgets;

  /// The relative error of the parameter. It is used for two things:
  /// @li first, set the background color for the line editor
  /// @li second, as a mouseover popup ?
  double relativeError;

public:
  /// Creates a widget to edit the given definition
  FitParameterEditor(const ParameterDefinition * d, int idx,
                     FitWorkspace * params, bool extended = false,
                     bool checkTight = true, 
                     int ds = 0);

public slots:
  /// If set error is false, all the errors are reset.
  void updateFromParameters(bool setErrors = false);

  /// Changes the current dataset.
  void selectDataSet(int dsIndex);

  /// Sets the relative error. A negative value cancels display
  void setRelativeError(double error);

protected slots:
  void onFixedClicked();
  void onGlobalClicked();
  void onValueChanged(const QString & str);

  /// Update the bijection-related editors
  void updateBijectionEditors();

  void updateBijectionParameters();

  void onBijectionParameterChanged();
  void onBijectionChanged(int idx);

  void contextMenu(const QPoint &pos);
signals:
  /// This signal is emitted whenever the global status is changing.
  void globalChanged(int index, bool global);
};


#endif
