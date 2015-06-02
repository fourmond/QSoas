/*
  valuehasheditor.cc: implementation of the ValueHashEditor
  Copyright 2015 by CNRS/AMU

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
#include <valuehasheditor.hh>
#include <widgets.hh>
#include <flowinggridlayout.hh>

ValueHashEditor::ValueHashEditor(QWidget * parent) : QWidget(parent)
{
  QVBoxLayout * vLayout = new QVBoxLayout(this);
  layout = new FlowingGridLayout();
  vLayout->addLayout(layout);
}

LabeledEdit * ValueHashEditor::editorForKey(const QString & key)
{
  if(! editors.contains(key)) {
    LabeledEdit * edit = new LabeledEdit(key);
    layout->addWidget(edit);
    editors[key] = edit;
  }
  return editors[key];
}

void ValueHashEditor::setFromHash(const ValueHash & src)
{
  QStringList keys = src.keys();
  // To look nicer a bit
  qSort(keys);
  for(int i = 0; i < keys.size(); i++) {
    const QString & k = keys[i];
    // Primitive, but will work for what we want
    editorForKey(k)->setText(src[k].toString());
  }
}

void ValueHashEditor::updateHash(ValueHash * tgt) const
{
  for(auto i = editors.begin(); i != editors.end(); i++) {
    QVariant::Type t = QVariant::String;
    if(tgt->contains(i.key()))
      t = (*tgt)[i.key()].type();
    (*tgt)[i.key()] = QVariant(i.value()->text());
    (*tgt)[i.key()].convert(t);
  }
}
