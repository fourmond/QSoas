/**
   scrolledwidget.cc: implementation of ScrolledWidget
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
#include <scrolledwidget.hh>


ScrolledWidget::ScrolledWidget(QWidget * center,
                               QWidget * parent) :
  QScrollArea(parent), widget(center)
{
  setWidget(widget);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
}

QSize ScrolledWidget::sizeHint() const
{
  QSize sz2 = widget->sizeHint();
  QSize sz3 = verticalScrollBar()->sizeHint();
  return sz2 + sz3;
}

void ScrolledWidget::resizeEvent(QResizeEvent *event)
{
  QSize sz2 = widget->sizeHint();
  QSize sz3 = verticalScrollBar()->sizeHint();
  sz2.setWidth(event->size().width() - sz3.width());
  widget->resize(sz2);
  
  QScrollArea::resizeEvent(event);
}
