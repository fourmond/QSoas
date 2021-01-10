/*
  checkablewidget.cc: implementation of CheckableWidget
  Copyright 2013 by CNRS/AMU

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
#include <checkablewidget.hh>

CheckableWidget::CheckableWidget(QWidget * sub,
                                 QWidget * parent) :
  QFrame(parent), widget(sub), present(NULL)
{
  QHBoxLayout * l = new QHBoxLayout(this);
  checkBox = new QCheckBox();
  l->addWidget(checkBox);
  l->addWidget(sub);
  setFrameStyle(QFrame::Panel);
  setFrameShadow(QFrame::Raised);
  setLineWidth(2);

  connect(checkBox, SIGNAL(stateChanged(int)),
          SLOT(cbStateChanged(int)));
}

bool CheckableWidget::isChecked() const
{
  return checkBox->isChecked();
}

void CheckableWidget::useSet(QSet<int> * tgt, int idx)
{
  present = NULL;
  index = idx;
  if(tgt)
    checkBox->setChecked(tgt->contains(index));
  present = tgt;
}

void CheckableWidget::cbStateChanged(int state)
{
  if(isChecked()) {
    setFrameShadow(QFrame::Sunken);
    if(present)
      present->insert(index);
  }
  else {
    setFrameShadow(QFrame::Raised);
    if(present)
      present->remove(index);
  }
}
