/*
  parametersdialog.cc: implementation of the ParametersDialog box
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


#include <headers.hh>
#include <bijection.hh>
#include <fitdata.hh>
#include <parametersdialog.hh>

ParametersDialog::ParametersDialog(FreeParameter * p) :
  param(p)
{
  availableBijections = Bijection::factoryItems();
  setupFrame();
}

void ParametersDialog::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);

  QHBoxLayout * hb = new QHBoxLayout();
  hb->addWidget(new QLabel("Transformation"));
  
  QComboBox * cb = new QComboBox;
  cb->addItem("(none)", -1);
  
  for(int i = 0; i < availableBijections.size(); i++)
    cb->addItem(availableBijections[i]->publicName, i);

  connect(cb, SIGNAL(activated(int)), 
          SLOT(onBijectionChange(int)));
  

  hb->addWidget(cb);
  layout->addLayout(hb);
  

  QPushButton * bt = new QPushButton(tr("Close"));
  connect(bt, SIGNAL(clicked()), SLOT(close()));
  layout->addWidget(bt);
}

void ParametersDialog::onBijectionChange(int idx)
{
  delete param->bijection;
  if(idx == 0) {
    param->bijection = NULL;
    return;
  }
  idx--;
  QString name = availableBijections[idx]->name;
  param->bijection = Bijection::createNamedBijection(name);
}
