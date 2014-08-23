/*
  widgets.cc: implementation of general purpose widgets
  Copyright 2014 by CNRS/AMU

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
#include <widgets.hh>

LabeledEdit::LabeledEdit(const QString & l, const QString & c, 
                         QWidget * parent) :
  QWidget(parent) {
  QHBoxLayout * layout = new QHBoxLayout(this);
  _label = new QLabel(l);
  layout->addWidget(_label);

  _edit = new QLineEdit(c);
  layout->addWidget(_edit);
}

void LabeledEdit::setLabelText(const QString & str)
{
  _label->setText(str);
}

void LabeledEdit::setText(const QString & txt)
{
  _edit->setText(txt);
}

QString LabeledEdit::text() const
{
  return _edit->text();
}

QLabel * LabeledEdit::label()
{
  return _label;
}

QLineEdit * LabeledEdit::lineEdit()
{
  return _edit;
}
