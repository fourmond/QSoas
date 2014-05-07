/**
   \file curvebrowser.hh
   Dialog box displaying several CurveView objects
   Copyright 2013,2014 by CNRS/AMU
   Written by Vincent Fourmond

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
#ifndef __CURVEBROWSER_HH
#define __CURVEBROWSER_HH

#include <curveview.hh>

/// A class for displaying several CurveView objects, and provide
/// facilities to switch easily between them.
class CurveBrowser : public QDialog {

  Q_OBJECT;

private:

  /// All the CurveView onto which one would display something.
  QList<CurveView *> views;

  /// Label corresponding to each of the views.
  QStringList labels;

  /// Stacked layout the views
  QStackedLayout * viewStackLayout;

  /// The label under the currently displayed CurveView
  QLabel * viewLabel;

  void setupFrame();

  /// The index of the current page
  int currentPage;


public:
  CurveBrowser(int sz);
  ~CurveBrowser();

  /// Choose the number of views displayed
  void setSize(int nb);

  /// The numbered view.
  CurveView * view(int idx);

  /// Sets the label for the given index
  void setLabel(int idx, const QString & str);

public:
public slots:

  void setPage(int newpage);
  void nextPage();
  void previousPage();

};

#endif
