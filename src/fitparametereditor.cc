/*
  fitparametereditor.cc: implementation of the FitParameter editors
   Copyright 2011 by Vincent Fourmond
             2012, 2013 by CNRS/AMU

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
#include <fitdata.hh>
#include <dataset.hh>

#include <terminal.hh>


#include <settings-templates.hh>

#include <bijection.hh>
#include <utils.hh>



FitParameterEditor::FitParameterEditor(const ParameterDefinition * d, 
                                       int idx, FitWorkspace * p, 
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


  QFontMetrics metrics(editor->font());
  int minWidth = metrics.width("1234567890");

  if(minWidth < 60)
    minWidth = 60;              // 60 = 10 * 6 =  6 digits ?

  sz.setWidth(minWidth);    
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
  else {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(contextMenu(const QPoint &)));
  }

  // update upon parameter change
  connect(parameters, SIGNAL(parameterChanged(int, int)),
          SLOT(parameterUpdated(int, int)));
  connect(parameters, SIGNAL(parametersChanged()),
          SLOT(updateFromParameters()));
}

void FitParameterEditor::contextMenu(const QPoint & pos)
{
  typedef enum {
    FixAll,
    FixAllButThis,
    UnfixAll,
    UnfixAllButThis,
    Noop
  } En;
  QMenu menu;
  QHash<QAction *, int> actions;
  actions[menu.addAction("Fix for all datasets")] = FixAll;
  actions[menu.addAction("Fix for all datasets but this one")] = FixAllButThis;
  actions[menu.addAction("Unfix for all datasets")] = UnfixAll;
  actions[menu.addAction("Unfix for all datasets but this one")] = UnfixAllButThis;

  int ac = actions.value(menu.exec(mapToGlobal(pos)), Noop);
  QString oldText = editor->text();
  switch(ac) {
  case FixAll:
  case UnfixAll:
    parameters->setFixed(index, -1, (ac == FixAll));
    updateBijectionEditors();
    onValueChanged(oldText);
    updatingEditor = true;
    fixed->setChecked(ac == FixAll);
    updatingEditor = false;
    break;
  case FixAllButThis:
  case UnfixAllButThis: {
    bool fxd = ac == FixAllButThis;
    for(int i = 0; i < parameters->data()->datasets.size(); i++) {
      parameters->setFixed(index, i, dataset == i ? !fxd : fxd);
    }
    updateBijectionEditors();
    onValueChanged(oldText);
    updatingEditor = true;
    fixed->setChecked(!fxd);
    updatingEditor = false;
  }
    break;
  case Noop:
  default:
    break;
  };
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
        QSize sz = ed->minimumSizeHint();
        int minWidth = 4*sz.width(); 
        if(minWidth < 40)
          minWidth = 40;             
        sz.setWidth(minWidth);    
        ed->setMinimumSize(sz);
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

  QString oldText = editor->text();
  parameters->setFixed(index, dataset, fixed->isChecked());
  // Replace with value...
  if(! fixed->isChecked() && oldText.size() > 0 && oldText[0] == '=') {
    oldText = QString::number(parameters->valueFor(index, dataset));
    editor->setText(oldText);   // Or not ?
  }
  updateBijectionEditors();
  onValueChanged(oldText);
}


void FitParameterEditor::onGlobalClicked()
{
  if(updatingEditor)
    return;
  bool nv = global->isChecked();
  parameters->setGlobal(index, nv);
  if(nv)
    onFixedClicked();           // so that we update the perception of
                                // the global parameter to that of the
                                // editor that toggled global
  emit(globalChanged(index, nv));
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

  setRelativeError(-1);         // Errors are meaningless if we edit !
  parameters->setValue(index, dataset, str);
}

void FitParameterEditor::selectDataSet(int dsIndex)
{
  dataset = dsIndex;
  updateFromParameters();
}

void FitParameterEditor::parameterUpdated(int idx, int ds)
{
  if(index == idx && (ds == -1 || ds == dataset))
    updateFromParameters(false);
}

void FitParameterEditor::updateFromParameters(bool setErrors)
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
  if(setErrors && (!parameters->isFixed(index, dsIdx)))
    setRelativeError(parameters->getParameterError(index, dsIdx));
  else
    setRelativeError(-1);
}

void FitParameterEditor::setRelativeError(double value)
{
  relativeError = value;        // Doesn't seem to be necessary !
  QColor background;
  if(relativeError < 0) {
    background = QColor(255,255,255); // White !
    editor->setToolTip("");           // Clear tool tip
  }
  else {
    double ra = relativeError > 0 ? log10(relativeError) + 0.5 : -2.5;
    if(ra < -2.5)
      ra = -2.5;
    if(ra > 2.5)
      ra = 2.5;
    ra /= -2.5;                    // Between -1 and 1
     
    background = QColor::fromHsvF((ra + 1)/6, 0.2, 1);

    editor->setToolTip(QString("Error: %1 %").arg(100 * value));
  }
  
  QPalette p;
  p.setColor(QPalette::Base, background);
  editor->setPalette(p);

}
