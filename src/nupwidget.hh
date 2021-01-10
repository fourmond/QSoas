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

  /// The list of sub-widgets this widget manages
  QList<QWidget *> widgets;

  /// returns the number of subwidgets
  int widgetsCount() const;

  /// Returns the n-th widget
  QWidget * widget(int index) const;

  /// Distribute all the datasets into pages...
  void setupPages();

  /// The current index
  int currentPage;

  /// Current number of columns
  int width;

  /// Current number of rows
  int height;

  /// The grid layout. Remade every time the nup changes.
  QGridLayout * layout;

  /// Clears the grid layout, hiding all the widgets that are
  /// currently inside
  void clearGrid();


public:
  NupWidget(QWidget * parent = 0);
  ~NupWidget();

  /// A function to generate datasets on the fly.
  typedef std::function<QWidget * (int idx, int id) > GeneratorFunc;

protected:

  /// The generator function
  GeneratorFunc generator;

  /// The number of widgets that can be generated
  int totalGeneratable;

public:
  
  /// Adds a widget to manage. The NupWidget takes ownership of the
  /// added widget.
  void addWidget(QWidget * widget);

  /// Sets up on-the-fly widget generation. Mutually exclusive with
  /// the use of addWidget. Contrary to addWidget(), the widgets are
  /// not deleted. A widget can be reused, so long as it is not
  /// displayed several times in the page.
  void setupGenerator(GeneratorFunc function, int total);


  /// Clears the list of widgets (and deletes them)
  void clear();

  /// Returns true if there is more than one row and/or one column
  bool isNup() const;

  /// Returns the total number of pages
  int totalPages() const;

  /// The width of the nup
  int nupWidth() const;

  /// The height of the nup
  int nupHeight() const;

  /// The index of the first visible widget?
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
