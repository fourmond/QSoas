/*
  nupwidget.cc: Implementation of the nup widget browser
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
#include <nupwidget.hh>

#include <exceptions.hh>

//////////////////////////////////////////////////////////////////////


NupWidget::NupWidget(QWidget * parent) : 
  QWidget(parent), currentPage(0), width(-1), height(-1),
  totalGeneratable(-1)
{
  layout = new QGridLayout(this);
}

NupWidget::~NupWidget()
{
}


int NupWidget::widgetsCount() const
{
  if(generator)
    return totalGeneratable;
  return widgets.size();
}


QWidget * NupWidget::widget(int index) const
{
  if(generator) {
    int sz = width*height;
    if(sz < 1)
      sz = 1;
    return generator(index, index%sz);
  }
  return widgets.value(index, NULL);
}

void NupWidget::setupGenerator(GeneratorFunc fn, int total)
{
  if(widgets.size() > 0)
    throw InternalError("Should not use both addWidget and setupGenerator");
  generator = fn;
  totalGeneratable = total;
}


int NupWidget::widgetIndex() const
{
  int w = width * height;
  if(w <= 0)
    w = 1;
  return currentPage * w;
}

bool NupWidget::isNup() const
{
  return (width != 1) || (height != 1);
}

void NupWidget::clearGrid()
{
  while(layout->count() > 0) {
    QLayoutItem * child = layout->takeAt(0);
    if(child->widget())
      child->widget()->hide();
    delete child;
  }
}

int NupWidget::totalPages() const
{
  int nb = width * height;
  int cnt = widgetsCount();
  if(nb > 0) {
    return cnt/nb + (cnt % nb > 0 ? 1 : 0);
  }
  return -1;
}

int NupWidget::nupWidth() const
{
  return width;
}

int NupWidget::nupHeight() const
{
  return height;
}

void NupWidget::showPage(int newpage)
{
  currentPage = newpage;
  if(currentPage >= totalPages())
    currentPage = totalPages() - 1;
  if(currentPage < 0)
    currentPage = 0;

  clearGrid();
  // Here, we assume that the grid layout is the correct size.
  int nb = width*height;
  int base = currentPage * nb;
  if(nb > 0) {
    for(int i = 0; i < nb; i++) {
      if(i + base >= widgetsCount())
        break;
      QWidget * w = widget(base + i);
      layout->addWidget(w, i/width, i % width);
      w->show();
    }
  }

  emit(pageChanged(currentPage));
}

void NupWidget::showWidget(int idx)
{
  if(width <= 0 || height <= 0)
    return;
  showPage(idx/(width*height));
}

void NupWidget::nextPage()
{
  showPage(currentPage + 1);
}

void NupWidget::previousPage()
{
  showPage(currentPage - 1);
}

void NupWidget::setNup(int w, int h)
{
  int cur = currentPage * width * height;
  width = w;
  height = h;
  clearGrid();
  delete layout;
  layout = new QGridLayout(this);
  if(width > 0 && height > 0)
    showPage(cur/(width * height));
  nupChanged(width, height);
}

void NupWidget::setNup(const QString & nup)
{
  QRegExp re("^\\s*(\\d+)\\s*x\\s*(\\d+)");
  if(re.indexIn(nup, 0) == 0) {
    int w = re.cap(1).toInt();
    int h = re.cap(2).toInt();
    setNup(w, h);
  }
}

void NupWidget::addWidget(QWidget * wd)
{
  wd->setParent(this);
  wd->hide();
  widgets << wd;
}

void NupWidget::clear()
{
  for(QWidget * w : widgets)
    delete w;
  widgets.clear();
  totalGeneratable = -1;
  generator = 0;
}
