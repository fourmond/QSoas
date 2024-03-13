/**
   \file scrollwidget.hh
   A simple QScrollArea wrapper around a given widget
   Copyright 2024 by CNRS/AMU

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
#ifndef __SCROLLEDWIDGET_HH
#define __SCROLLEDWIDGET_HH

/// A small class helping me deal with embedding and scrolling widgets
/// I have a feeling this is essentially QScrollArea, but I never
/// manage to get this one running without a bit of help
class ScrolledWidget : public QScrollArea {
protected:

  QWidget * widget;
public:
  explicit ScrolledWidget(QWidget * center, QWidget * parent = NULL);

  virtual QSize sizeHint() const override;

protected:
  void resizeEvent(QResizeEvent *event) override;
};

#endif
