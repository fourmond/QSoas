/**
   \file parametersviewer.hh
   Display of fit parameters along the perpendicular coordinate
   Copyright 2014, 2017 by CNRS/AMU

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
class FitWorkspace;
class CurveView;
class TuneableDataDisplay;      // that's not a very nice name...
class CurvePoints;

/// This class handles all the user interaction during fits.
class ParametersViewer : public QDialog {

  Q_OBJECT;

private:

  /// The list of checkboxes for the parameters.
  QList<TuneableDataDisplay *> parametersDisplays;

  /// The view !
  CurveView * view;

  /// The perpendicular coordinates
  Vector perpendicularCoordinates;

  /// The fit parameters
  FitWorkspace * parameters;

  void makePerpendicularCoordinates();
  void setupFrame();

public:
  ParametersViewer(FitWorkspace * params);
  ~ParametersViewer();

protected slots:

  void pushVisible();

  void pushAll();

};

#endif
