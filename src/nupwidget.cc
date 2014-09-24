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
  QWidget(parent), width(-1), height(-1), currentPage(0)
{
  pageStackLayout = new QStackedLayout(this);
}

NupWidget::~NupWidget()
{
}

void NupWidget::setupPages()
{
  if(width <= 0 || height <= 0)
    return;                     // Nothing to do for so long as there
                                // is no nup.

  int nbPerPage = width * height;
  int pgs = (subWidgets.size() - 1)/nbPerPage + 1;

  QList<QWidget * > oldpages = pages;
  pages.clear();

  
  for(int i = 0; i < pgs; i++) {
    int base = i * nbPerPage;
    QWidget * page = new QWidget(this);
    QGridLayout * layout = new QGridLayout(page);
    for(int j = 0; j < nbPerPage; j++) {
      QWidget * w = subWidgets.value(base + j, NULL);
      if(w)
        layout->addWidget(w, j/width, j % width);
      else 
        layout->addWidget(new QWidget, j/width, j % width);
    }
    /// @todo Customize the possibility not to have that ?
    for(int k = 0; k < width; k++)
      layout->setColumnStretch(k, 1);
    for(int k = 0; k < height; k++)
      layout->setRowStretch(k, 1);
    pageStackLayout->addWidget(page);
    pages << page;
  }

  // we delete later so that the child widgets have already changed parents.
  while(oldpages.size() > 0)
    delete oldpages.takeAt(0);
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


void NupWidget::showPage(int newpage)
{
  currentPage = newpage;
  if(currentPage >= pages.size())
    currentPage = pages.size() - 1;
  if(currentPage < 0)
    currentPage = 0;

  pageStackLayout->setCurrentIndex(currentPage);
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
  setupPages();
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
  subWidgets << wd;
  setupPages();
}

void NupWidget::clear()
{
  while(subWidgets.size() > 0)
    delete subWidgets.takeAt(0);
}
