/*
  fitdialog.cc: Main window for QSoas
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
#include <fitdialog.hh>
#include <fit.hh>
#include <curveview.hh>
#include <dataset.hh>

#include <curvevector.hh>
#include <curvepanel.hh>
#include <soas.hh>
#include <terminal.hh>

#include <actioncombo.hh>
#include <vector.hh>

#include <settings-templates.hh>

#include <flowinggridlayout.hh>



FitParameterEditor::FitParameterEditor(const ParameterDefinition * d, 
                                       int idx) : index(idx), def(d)
{
  QHBoxLayout * layout = new QHBoxLayout(this);
  layout->addWidget(new QLabel(QString("<b>%1: </b>").arg(def->name)), 1);
  editor = new QLineEdit();
  connect(editor, SIGNAL(textChanged(const QString &)),
          SLOT(onValueChanged(const QString &)));
  layout->addWidget(editor);

  QSize sz = editor->minimumSizeHint();
  sz.setWidth(7*sz.width());
  editor->setMinimumSize(sz);

  global = new QCheckBox(tr("(global)"));
  connect(global, SIGNAL(clicked(bool)), SLOT(onGlobalClicked()));
  layout->addWidget(global);

  fixed = new QCheckBox(tr("(fixed)"));
  connect(fixed, SIGNAL(clicked(bool)), SLOT(onFixedClicked()));
  layout->addWidget(fixed);


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


//////////////////////////////////////////////////////////////////////

static SettingsValue<QSize> fitDialogSize("fitdialog/size", QSize(700,500));


FitDialog::FitDialog(FitData * d) : data(d),
                                    stackedViews(NULL), currentIndex(0),
                                    settingEditors(false)
{
  resize(fitDialogSize);
  unpackedParameters = new double[d->fullParameterNumber()];
  d->fit->initialGuess(d, unpackedParameters);

  int nbparams = d->parameterDefinitions.size();
  isGlobal = new bool[nbparams];
  for(int i = 0; i < nbparams; i++)
    isGlobal[i] = ! d->parameterDefinitions[i].canBeBufferSpecific;

  isFixed = new bool[d->fullParameterNumber()];
  for(int i = 0; i != d->fullParameterNumber(); i++)
    isFixed[i] = d->parameterDefinitions[i % nbparams].defaultsToFixed;

  compute();
  setupFrame();

  updateEditors();
}

FitDialog::~FitDialog()
{
  delete[] unpackedParameters;
  delete[] isGlobal;
  delete[] isFixed;

  fitDialogSize = size();

}

void FitDialog::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  stackedViews = new QStackedWidget;
  bufferSelection = new QComboBox;
  for(int i = 0; i < data->datasets.size(); i++) {
    const DataSet * ds = data->datasets[i];
    CurveView * view = new CurveView;
    gsl_vector_view v = data->viewForDataset(i, data->storage);
    view->showDataSet(ds);
    view->addItem(new CurveVector(ds, v));

    CurvePanel * bottomPanel = new CurvePanel();
    bottomPanel->stretch = 30;
    bottomPanel->drawingXTicks = false;
    bottomPanel->drawingLegend = false;
    bottomPanel->addItem(new CurveVector(ds, v, true));
    /// @todo add X tracking for the bottom panel.
    /// @todo Colors !
    view->addPanel(bottomPanel);
    
    stackedViews->addWidget(view);
    views << view;
    bufferSelection->addItem(ds->name);
  }
  layout->addWidget(stackedViews, 1);

  QHBoxLayout * hb = new QHBoxLayout;
  QPushButton * bt = new QPushButton(tr("<-"));
  connect(bt, SIGNAL(clicked()), SLOT(previousDataset()));
  hb->addWidget(bt);

  hb->addWidget(bufferSelection, 1);
  connect(bufferSelection, SIGNAL(currentIndexChanged(int)),
          SLOT(dataSetChanged(int)));
  
  bufferNumber = new QLabel(QString("%1/%2").
                            arg(1).arg(data->datasets.size()));
  hb->addWidget(bufferNumber);

  bt = new QPushButton(tr("->"));
  connect(bt, SIGNAL(clicked()), SLOT(nextDataset()));
  hb->addWidget(bt);

  layout->addLayout(hb);

  FlowingGridLayout * inner = new FlowingGridLayout();
  for(int i = 0; i < data->parameterDefinitions.size(); i++) {
    FitParameterEditor * ed = 
      new FitParameterEditor(&data->parameterDefinitions[i], i);
    inner->addWidget(ed);
    editors.append(ed);
    connect(ed, SIGNAL(fixedChanged(int, bool)), SLOT(onSetFixed(int)));
    connect(ed, SIGNAL(globalChanged(int, bool)), SLOT(onSetGlobal(int)));
    connect(ed, SIGNAL(valueChanged(int, double)), 
            SLOT(onSetValue(int, double)));
  }
  layout->addLayout(inner);

  hb = new QHBoxLayout;
  hb->addWidget(new QLabel(tr("Actions:")));
  ActionCombo * ac = new ActionCombo(tr("Data..."));
  ac->addAction("Push all to stack", this, SLOT(pushSimulatedCurves()));
  ac->addAction("Push current to stack", this, SLOT(pushCurrentCurve()));
  ac->addAction("Save all", this, SLOT(saveSimulatedCurves()));
  hb->addWidget(ac);
  
  ac = new ActionCombo(tr("Parameters..."));
  ac->addAction("Load from file", this, SLOT(loadParameters()));
  ac->addAction("Save to file", this, SLOT(saveParameters()));
  hb->addWidget(ac);
  hb->addStretch(1);

  layout->addLayout(hb);


  hb = new QHBoxLayout;
  bt = new QPushButton(tr("Update curves"));
  connect(bt, SIGNAL(clicked()), SLOT(compute()));
  hb->addWidget(bt);

  bt = new QPushButton(tr("Fit"));
  connect(bt, SIGNAL(clicked()), SLOT(startFit()));
  hb->addWidget(bt);


  bt = new QPushButton(tr("Close"));
  connect(bt, SIGNAL(clicked()), SLOT(reject()));
  hb->addWidget(bt);

  layout->addLayout(hb);
}


void FitDialog::dataSetChanged(int newds)
{
  stackedViews->setCurrentIndex(newds);
  currentIndex = newds;
  updateEditors();
  bufferNumber->setText(QString("%1/%2").
                        arg(newds + 1).arg(data->datasets.size()));
}


void FitDialog::compute()
{
  data->fit->function(unpackedParameters, 
                      data, data->storage);
  if(stackedViews && stackedViews->currentWidget())
    stackedViews->currentWidget()->repaint();
}


void FitDialog::updateEditors()
{
  settingEditors = true;
  int sz = editors.size();
  for(int i = 0; i < sz; i++) {
    double v;
    bool fixed;
    if(isGlobal[i]) {
      v = unpackedParameters[i];
      fixed = isFixed[i];
    }
    else {
      v = unpackedParameters[i + sz * currentIndex];
      fixed = isFixed[i + sz * currentIndex];
    }
    editors[i]->setValues(v, fixed, isGlobal[i]);
  }
  settingEditors = false;
}

void FitDialog::onSetGlobal(int index)
{
  isGlobal[index] = editors[index]->isGlobal();
}

void FitDialog::onSetFixed(int index)
{
  if(isGlobal[index])
    isFixed[index] = editors[index]->isFixed();
  else {
    int sz = editors.size();
    isFixed[index + currentIndex *sz] = 
      editors[index]->isFixed();
  }
}

void FitDialog::onSetValue(int index, double v)
{
  if(isGlobal[index])
    unpackedParameters[index] = v;
  else {
    int sz = editors.size();
    unpackedParameters[index + currentIndex *sz] = v;
  }
}

void FitDialog::setDataParameters()
{
  data->parameters.clear();
  data->fixedParameters.clear();

  int nb_bufs = data->datasets.size();
  int sz = editors.size();
  for(int i = 0; i < nb_bufs; i++) {
    for(int j = 0; j < sz; j++) {
      int ds = (isGlobal[j] ? -1 : i);
      if(isGlobal[j]) {
        if(i)
          continue;
        if(isFixed[j])
          data->fixedParameters 
            << FixedParameter(j, ds, unpackedParameters[j]);
        else 
          data->parameters
            << ActualParameter(j, ds);
      }
      else {
        if(isFixed[sz * i + j])
          data->fixedParameters 
            << FixedParameter(j, ds, unpackedParameters[sz* i + j]);
        else 
          data->parameters
            << ActualParameter(j, ds);
      }
    }
  }
}

void FitDialog::startFit()
{
  setDataParameters();
  data->initializeSolver(unpackedParameters);
  Terminal::out << "Starting fit '" << data->fit->fitName() << "' with "
                << data->parameters.size() << " free parameters"
                << endl;

  /// @todo customize the number of iterations
  while(data->iterate() == GSL_CONTINUE && data->nbIterations < 100) {
    soas().showMessage(tr("Fit iteration %1").arg(data->nbIterations));
    Terminal::out << "Iteration " << data->nbIterations 
                  << ", residuals :" << data->residuals() << endl;
  }
  data->unpackCurrentParameters(unpackedParameters);
  compute();
  updateEditors();
}

void FitDialog::nextDataset()
{
  if(currentIndex + 1 < data->datasets.size())
    bufferSelection->setCurrentIndex(currentIndex + 1);
}

void FitDialog::previousDataset()
{
  if(currentIndex > 0)
    bufferSelection->setCurrentIndex(currentIndex - 1);
}

DataSet *  FitDialog::simulatedData(int i)
{
  const DataSet * base = data->datasets[i];
  gsl_vector_view v =  data->viewForDataset(i, data->storage);    
  DataSet * ds = 
    new DataSet(QList<Vector>() << base->x() 
                << Vector::fromGSLVector(&v.vector));
  ds->name = base->cleanedName() + "_fit_" + data->fit->fitName() + ".dat";
  return ds;
}

                
void FitDialog::pushSimulatedCurves()
{
  for(int i = 0; i < views.size(); i++)
    soas().pushDataSet(simulatedData(i));
}

void FitDialog::pushCurrentCurve()
{
  if(currentIndex >= 0)
    soas().pushDataSet(simulatedData(currentIndex));
}

void FitDialog::saveSimulatedCurves()
{
}

void FitDialog::saveParameters()
{
}

void FitDialog::loadParameters()
{
}
