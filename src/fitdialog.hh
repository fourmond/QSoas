/**
   \file fitdialog.hh
   Main window for QSoas
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

#ifndef __FITDIALOG_HH
#define __FITDIALOG_HH

class FitData;
class CurveView;

/// This class handles all the user interaction during fits.
class FitDialog : public QDialog {

  Q_OBJECT;

  void setupFrame();

  /// The FitData object we'll populate and run
  FitData * data;

  /// The stacked widgets holding the buffer view;
  QStackedWidget * stackedViews;

  /// The dataset views
  QList<CurveView *> views;

  /// The combo box for toogling between the various buffers
  QComboBox * bufferSelection;

  /// The button group for the "global or not global" checkboxes
  QButtonGroup * globalGroup;

  /// The button group for the global "fixed or free" checkboxes
  QButtonGroup * globalFixedGroup;

  /// The button group for the buffer-local "fixed or free" checkboxes
  QButtonGroup * localFixedGroup;

  /// The current parameters of the Fit, in unpacked form
  double * unpackedParameters;

  /// The current index
  int currentIndex;


public:
  FitDialog(FitData * data);
  ~FitDialog();

protected slots:

  void dataSetChanged(int newds);

  /// Compute the new curves based on initial guesses (or
  /// hand-modified versions)
  void compute();

};

#endif
