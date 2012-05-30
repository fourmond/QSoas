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
#include <fitparameter.hh>
#include <fit.hh>
#include <dataset.hh>

#include <terminal.hh>


#include <settings-templates.hh>

#include <bijection.hh>
#include <utils.hh>



FitParameterEditor::FitParameterEditor(const ParameterDefinition * d, 
                                       int idx, FitParameters * p, 
                                       bool ext, bool checkTight, int ds) : 
  index(idx), dataset(ds), def(d), parameters(p), updatingEditor(false), 
  extended(ext)
{
  layout = new QHBoxLayout(this);
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

  if(parameters->datasets <= 1)
    global->setVisible(false);

  if(parameters->nbParameters >= 10 && checkTight) {
    global->setText("(G)");
    fixed->setText("(F)");
    sz.setWidth(5*sz.width()/6);
    editor->setMinimumSize(sz);
  }

  if(extended) {
    QLabel * bijectionLabel = new QLabel("setup transformation: ");
    layout->addWidget(bijectionLabel);

    bijectionCombo = new QComboBox;
    bijectionCombo->addItem("(none)", -1);
    connect(bijectionCombo, SIGNAL(activated(int)), 
            SLOT(onBijectionChanged(int)));
    layout->addWidget(bijectionCombo);
    
    availableBijections = Bijection::factoryItems();

    for(int i = 0; i < availableBijections.size(); i++) {
      const BijectionFactoryItem * it = availableBijections[i];
      bijectionCombo->addItem(it->publicName, i);
    }

    bijectionWidgets << bijectionLabel << bijectionCombo;

    updateBijectionEditors();
  }
}


/// It shouldn't be too difficult to keep the last bijection used in
/// memory in order to find it again
void FitParameterEditor::updateBijectionEditors()
{
  if(! extended)
    return;
  if(isFixed())
    for(int i = 0; i < bijectionWidgets.size(); i++)
      bijectionWidgets[i]->hide();
  else {
    for(int i = 0; i < bijectionWidgets.size(); i++)
      bijectionWidgets[i]->show();

    FreeParameter * param = dynamic_cast<FreeParameter*>(targetParameter());
    if(! param)
      return ;                  // But, really, that shouldn't happen !
    int idx = 0;
    for(int j = 0; j < availableBijections.size(); j++) {
      const BijectionFactoryItem * it = availableBijections[j];
      if(param->bijection && it->name == param->bijection->name())
        idx = j+1;
    }
    bool se = updatingEditor;   // Saving the value of updatingEditor.
    updatingEditor = true;
    bijectionCombo->setCurrentIndex(idx);
    updatingEditor = se;

  }
  updateBijectionParameters();
}


void FitParameterEditor::updateBijectionParameters()
{
  FreeParameter * param = dynamic_cast<FreeParameter*>(targetParameter());
  if(! param)
    return ;

  for(int i = 0; i < bijectionParameterEditors.size(); i++) {
    bijectionParameterEditors[i]->hide();
    bijectionParameterLabels[i]->hide();
  }

  if(param->bijection) {
    QStringList params = param->bijection->parameters();
    for(int i = 0; i < params.size(); i++) {
      QLabel * label = bijectionParameterLabels.value(i, NULL);
      if(! label) {
        label = new QLabel;
        layout->addWidget(label);
        bijectionParameterLabels << label;
        bijectionWidgets << label;
        QLineEdit * ed = new QLineEdit;
        layout->addWidget(ed);
        bijectionParameterEditors << ed;
        bijectionWidgets << ed;
        connect(ed, SIGNAL(textEdited(const QString &)),
                SLOT(onBijectionParameterChanged()));
      }
      label->setText(params[i]);
      bijectionParameterEditors[i]->
        setText(QString::number(param->bijection->parameterValue(i)));
      bijectionParameterEditors[i]->show();
      bijectionParameterLabels[i]->show();
    }
  }
}

void FitParameterEditor::onBijectionParameterChanged()
{
  FreeParameter * param = dynamic_cast<FreeParameter*>(targetParameter());
  if(! param)
    return ;
  if(param->bijection) {
    QStringList params = param->bijection->parameters();
    for(int i = 0; i < params.size(); i++)
      param->bijection->
        setParameterValue(i, bijectionParameterEditors[i]->text().toDouble());
  }
}

void FitParameterEditor::onBijectionChanged(int idx)
{
  FreeParameter * param = dynamic_cast<FreeParameter*>(targetParameter());
  if(! param)
    return ;
  delete param->bijection;
  if(idx == 0)
    param->bijection = NULL;
  else {
    idx--;
    QString name = availableBijections[idx]->name;
    param->bijection = Bijection::createNamedBijection(name);
  }
  updateBijectionParameters();
}
  
void FitParameterEditor::onFixedClicked()
{
  if(updatingEditor)
    return;
  FitParameter *& target = targetParameter();
  int ds = target->dsIndex;

  QString oldText = editor->text();
  delete target;
  if(fixed->isChecked()) {
    FixedParameter * param = new FixedParameter(index, ds,  0);
    target = param;
  }
  else {
    target = new FreeParameter(index, ds);
    if(oldText.size() > 0 && oldText[0] == '=') {
      oldText = QString::number(parameters->valueFor(index, dataset));
      editor->setText(oldText);
    }

  }
  updateBijectionEditors();
  onValueChanged(oldText);
}


void FitParameterEditor::onGlobalClicked()
{
  if(updatingEditor)
    return;
  FitParameter *& target = parameters->parameter(index, 0);
  if(global->isChecked()) {
    target->dsIndex = -1;
    emit(globalChanged(index,true));
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
    emit(globalChanged(index,false));
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

  if(isFixed() && str.size() > 0) {
    // We need to detect if we are changing the category (ie normal
    // fixed vs other) of the parameter.
    FitParameter *& target = targetParameter();
    int ds = target->dsIndex;
    if(dynamic_cast<FormulaParameter *>(target) != NULL && str[0] != '=') {
      // Replace
      FixedParameter * param = new FixedParameter(index, ds,  0);
      delete target;
      target = param;
    }
    else if(dynamic_cast<FormulaParameter *>(target) == NULL 
            && str[0] == '=') {
      FormulaParameter * param = new FormulaParameter(index, ds, "0");
      delete target;
      target = param;
    }
  }

  if(isGlobal()) {
    /// @todo This whole loop may not be necessary anymore now that we
    /// have a pack/unpack cycle in FitParameters::recompute()
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

  updateBijectionEditors();
  updatingEditor = false;
}
