/*
  fitparametereditor.cc: implementation of the FitParameter editors
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
#include <fitparametereditor.hh>
#include <fit.hh>
#include <fitdata.hh>
#include <dataset.hh>

#include <terminal.hh>


#include <settings-templates.hh>

#include <flowinggridlayout.hh>

#include <utils.hh>



FitParameterEditor::FitParameterEditor(const ParameterDefinition * d, 
                                       int idx, int totalDS, 
                                       int totalParams) : 
  index(idx), def(d)
{
  QHBoxLayout * layout = new QHBoxLayout(this);
  layout->addWidget(new QLabel(QString("<b>%1: </b>").arg(def->name)), 1);
  editor = new QLineEdit();
  connect(editor, SIGNAL(textChanged(const QString &)),
          SLOT(onValueChanged(const QString &)));
  layout->addWidget(editor);

  QSize sz = editor->minimumSizeHint();
  sz.setWidth(6*sz.width());
  editor->setMinimumSize(sz);

  fixed = new QCheckBox(tr("(fixed)"));
  connect(fixed, SIGNAL(clicked(bool)), SLOT(onFixedClicked()));
  layout->addWidget(fixed);
  fixed->setToolTip(tr("If checked, the parameter is fixed"));


  global = new QCheckBox(tr("(global)"));
  connect(global, SIGNAL(clicked(bool)), SLOT(onGlobalClicked()));
  layout->addWidget(global);
  global->setToolTip(tr("If checked, the parameter is "
                        "common to all data sets"));
  
  if(! d->canBeBufferSpecific) {
    global->setChecked(true);
    global->setEnabled(false);
  }

  if(totalDS <= 1)
    global->setVisible(false);

  if(totalParams >= 10) {
    global->setText("(G)");
    fixed->setText("(F)");
    sz.setWidth(5*sz.width()/6);
    editor->setMinimumSize(sz);
  }

}
  
void FitParameterEditor::onFixedClicked()
{
  emit(fixedChanged(index, fixed->checkState() == Qt::Checked));
}


void FitParameterEditor::onGlobalClicked()
{
  emit(globalChanged(index, global->checkState() == Qt::Checked));
}

void FitParameterEditor::onValueChanged(const QString & str)
{
  bool ok;
  double value = str.toDouble(&ok);
  if(ok)
    emit(valueChanged(index, value));
}

void FitParameterEditor::setValues(double v, bool f, bool g)
{
  editor->setText(QString::number(v));
  fixed->setChecked(f);
  global->setChecked(g);
}
