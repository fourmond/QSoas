/**
   \file curvedisplaywidget.hh
   The widget handling all "terminal" interaction
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

#ifndef __CURVEDISPLAYWIDGET_HH
#define __CURVEDISPLAYWIDGET_HH

class DataSet;
class CurveView;

/// The display of 2D curves.
///
/// It displays the curves.
class CurveDisplayWidget : public QWidget {

  Q_OBJECT;

  /// The application-wide CurveDisplayWidget
  static CurveDisplayWidget * theCurveDisplayWidget;

  /// The underlying scene used for rendering.
  QGraphicsScene * scene;

  /// and the widget to view it...
  CurveView * view;

public:

  CurveDisplayWidget();
  virtual ~CurveDisplayWidget();

  /// Returns the application-wide CurveDisplayWidget
  static CurveDisplayWidget * displayWidget() {
    return theCurveDisplayWidget;
  };

  void addDataSet(const DataSet * ds);

};

#endif
