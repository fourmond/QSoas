/**
   \file parametersdialog.hh
   Dialog box handling fine-tuning of parameters details for fits
   Copyright 2012 by Vincent Fourmond

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <headers.hh>
#ifndef __PARAMETERSDIALOG_HH
#define __PARAMETERSDIALOG_HH

class FitParameters;
class FitParameterEditor;

/// This class provides a dialog to fine-tune the exact nature of all
/// parameters
class ParametersDialog : public QDialog {

  Q_OBJECT;

  void setupFrame();

  /// The parameters we'll be editing
  FitParameters * parameters;

  /// The editors needed for that:
  QList<FitParameterEditor *> editors;

  /// Number of parameters per dataset
  int nbParams;

  /// Number of datasets
  int nbDatasets;

public:
  ParametersDialog(FitParameters * params);

  virtual ~ParametersDialog();

protected slots:
  void onGlobalChanged(int index, bool global);
};

#endif
