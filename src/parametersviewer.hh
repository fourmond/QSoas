/**
   \file parametersviewer.hh
   Display of fit parameters along the perpendicular coordinate
   Copyright 2014 by Vincent Fourmond

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
#ifndef __PARAMETERSVIEWER_HH
#define __PARAMETERSVIEWER_HH

#include <dataset.hh>
#include <curveview.hh>

class DataSet;
class FitParameters;
class CurveView;
class CurveDataSet;

/// This class handles all the user interaction during fits.
class ParametersViewer : public QDialog {

  Q_OBJECT;

private:

  /// The list of checkboxes for the parameters.
  QList<QCheckBox *> parametersBoxes;

  /// Grouping all the parameter checkboxes
  ///
  /// Could also be done using QSignalMapper
  QButtonGroup * checkBoxes;

  /// A list of temporary datasets for each of the parameters (in the
  /// same order as the checkboxes)
  QList<DataSet * > datasets;

  /// The view !
  CurveView * view;

  /// The list of displayed datasets
  QList<CurveDataSet *> curveDatasets;

  /// The fit parameters
  FitParameters * parameters;

  void makeDatasets();
  void setupFrame();

public:
  ParametersViewer(FitParameters * params);
  ~ParametersViewer();

protected slots:

  void parameterChecked(int idx);

  void pushVisible();

  void pushAll();

};

#endif
