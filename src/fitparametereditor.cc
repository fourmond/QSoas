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

#include <parametersdialog.hh>

#include <terminal.hh>


#include <settings-templates.hh>

#include <flowinggridlayout.hh>

#include <utils.hh>



FitParameterEditor::FitParameterEditor(const ParameterDefinition * d, 
                                       int idx, FitParameters * p) : 
  index(idx), def(d), parameters(p), updatingEditor(false)
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

  QLabel * label = new QLabel(tr("<a href='biniou'>More</a>"));
  connect(label, SIGNAL(linkActivated(const QString &)), 
          SLOT(showEditor()));
  layout->addWidget(label);
  
  if(! d->canBeBufferSpecific) {
    global->setChecked(true);
    global->setEnabled(false);
  }

  if(parameters->datasets <= 1)
    global->setVisible(false);

  if(parameters->nbParameters >= 10) {
    global->setText("(G)");
    fixed->setText("(F)");
    sz.setWidth(5*sz.width()/6);
    editor->setMinimumSize(sz);

  }

}
  
void FitParameterEditor::onFixedClicked()
{
  if(updatingEditor)
    return;
  FitParameter *& target = targetParameter();
  int ds = target->dsIndex;
  
  delete target;
  if(fixed->isChecked()) {
    FixedParameter * param = new FixedParameter(index, ds,  0);
    target = param;
  }
  else {
    target = new FreeParameter(index, ds);
  }
  onValueChanged(editor->text());
}


void FitParameterEditor::onGlobalClicked()
{
  if(updatingEditor)
    return;
  FitParameter *& target = parameters->parameter(index, 0);
  if(global->isChecked()) {
    target->dsIndex = -1;
    for(int i = 1; i < parameters->datasets; i++) {
      delete parameters->parameter(index, i);
      parameters->parameter(index, i) = NULL;
    }
  }
  else {
    target->dsIndex = 0;
    for(int i = 1; i < parameters->datasets; i++) {
      FitParameter * p = target->dup();
      p->dsIndex = i;
      parameters->parameter(index, i) = p;
    }
  }
  onValueChanged(editor->text());
}

bool FitParameterEditor::isGlobal() const
{
  return parameters->parameter(index, 0)->global();
}

void FitParameterEditor::onValueChanged(const QString & str)
{
  if(updatingEditor)
    return;
  if(isGlobal()) {
    for(int i = 0; i < parameters->datasets; i++)
      parameters->parameter(index, 0)->
        setValue(&parameters->valueFor(index, i), str);
  }
  else
    parameters->parameter(index, dataset)->
      setValue(&parameters->valueFor(index, dataset), str);
  
  // not necessary... for now !
  // parameters->dump();
}

void FitParameterEditor::selectDataSet(int dsIndex)
{
  dataset = dsIndex;
  updateFromParameters();
}

void FitParameterEditor::updateFromParameters()
{
  updatingEditor = true;
  int dsIdx = dataset;
  FitParameter * param = targetParameter();
  if(param->global())
    dsIdx = 0;

  global->setChecked(param->global());
  fixed->setChecked(param->fixed());
  editor->
    setText(param->textValue(parameters->
                             values[index + dsIdx * parameters->nbParameters]));


  updatingEditor = false;
}

void FitParameterEditor::showEditor()
{
  FreeParameter * param = dynamic_cast<FreeParameter*>(targetParameter());
  if(param) {
    ParametersDialog dlg(param);
    dlg.exec();
  }
}
