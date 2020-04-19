/*
  curvebrowser.cc: Implementation of the CurveView browser
  Copyright 2013, 2014 by CNRS/AMU
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
#include <curvebrowser.hh>
#include <settings-templates.hh>

#include <checkablewidget.hh>
#include <utils.hh>


//////////////////////////////////////////////////////////////////////

static SettingsValue<QSize> browserSize("curvebrowser/size", QSize(700,500));


CurveBrowser::CurveBrowser(int sz) : 
  currentPage(0)
{
  resize(browserSize);
  setupFrame();
  setSize(sz);
}

CurveBrowser::~CurveBrowser()
{
  browserSize = size();
}


void CurveBrowser::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);

  viewStackLayout = new QStackedLayout();
  layout->addLayout(viewStackLayout);


  QHBoxLayout * bottomLayout = new QHBoxLayout;
  layout->addLayout(bottomLayout);

  viewLabel = new QLabel("");
  bottomLayout->addWidget(viewLabel);

  QPushButton * bt =
    new QPushButton(Utils::standardIcon(QStyle::SP_ArrowLeft), "");
  connect(bt, SIGNAL(clicked()), SLOT(previousPage()));
  bottomLayout->addWidget(bt);

  /// @todo Make a Utils:: function for that idiom !
  bt = new QPushButton(Utils::standardIcon(QStyle::SP_ArrowRight), "");
  connect(bt, SIGNAL(clicked()), SLOT(nextPage()));
  bottomLayout->addWidget(bt);

  bottomLayout = new QHBoxLayout;
  bottomLayout->addStretch(1);

  bt = new QPushButton(tr("Close"));
  connect(bt, SIGNAL(clicked()), SLOT(accept()));
  bottomLayout->addWidget(bt);

  layout->addLayout(bottomLayout);
}

void CurveBrowser::setPage(int newpage)
{
  currentPage = newpage;
  if(currentPage >= views.size())
    currentPage = views.size() - 1;
  if(currentPage < 0)
    currentPage = 0;

  viewStackLayout->setCurrentIndex(currentPage);

  viewLabel->setText(QString("%3 %1/%2").
                     arg(currentPage+1).
                     arg(views.size()).
                     arg(labels.value(currentPage)));
}

void CurveBrowser::nextPage()
{
  setPage(currentPage + 1);
}

void CurveBrowser::previousPage()
{
  setPage(currentPage - 1);
}

CurveView * CurveBrowser::view(int idx)
{
  return views.value(idx, NULL);
}

void CurveBrowser::setSize(int sz)
{
  if(sz < views.size()) {
    while(sz < views.size()) {
      delete views.takeLast();
      labels.takeLast();
    }
  }
  else {
    while(sz > views.size()) {
      CurveView * v = new CurveView;
      viewStackLayout->insertWidget(views.size(), v);
      views << v;
      labels << "";
    }
  }
}

void CurveBrowser::setLabel(int idx, const QString & str)
{
  labels[idx] = str;
}
