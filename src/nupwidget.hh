/**
   \file nupwidget.hh
   Widget displaying nuped subwidgetse
   Copyright 2012, 2013, 2014 by CNRS/AMU

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
#ifndef __NUPWIDGET_HH
#define __NUPWIDGET_HH

#include <dataset.hh>
#include <curveview.hh>

class CheckableWidget;


/// Widget to display a series of subwidgets through a nup-ed view.
///
/// The widget won't display anything before a call to setupNup().
class NupWidget : public QWidget {

  Q_OBJECT;

private:

  /// Widgets embedding all datasets for a given page
  QList<QWidget *> pages;


  /// The list of sub-widgets this widget manages
  QList<QWidget *> subWidgets;

  /// Distribute all the datasets into pages...
  void setupPages();

  /// Stacked layout for the pages
  QStackedLayout * pageStackLayout;

  /// The current index
  int currentPage;

  /// Current number of columns
  int width;

  /// Current number of rows
  int height;

  /// Base index
  int index;

  /// Whether or not to use extended selection capacities
  bool extendedSelection;

public:
  NupWidget(QWidget * parent = 0);
  ~NupWidget();

public:
  /// Adds a widget to manage
  void addWidget(QWidget * widget);

  /// Clears the list of widgets (and deletes them)
  void clear();

  /// Returns true if there is more than one row and/or one column
  bool isNup() const;

  /// Returns the total number of pages
  int totalPages() const {
    return pages.size();
  };

  /// The width of the nup
  int nupWidth() const {
    return width;
  };

  /// The height of the nup
  int nupHeight() const {
    return height;
  };

  /// The index of the first visible
  int widgetIndex() const;

public slots:

  /// Changes the nup of the window.
  void setNup(int width, int height);

  /// Changes the nup based on some wxh specification.
  void setNup(const QString & str);

  /// Shows next page
  void nextPage();

  /// Shows previous page
  void previousPage();

  /// Shows the numbered page
  void showPage(int nb);

  /// Ensures the numbered widget is visible
  void showWidget(int idx);

signals:
  void pageChanged(int nb);

  void nupChanged(int w, int h);

};

#endif
