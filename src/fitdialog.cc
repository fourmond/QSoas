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

FitDialog::FitDialog(FitData * d) : data(d),
                                    stackedViews(NULL), currentIndex(0),
                                    settingEditors(false)
{
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
    bottomPanel->addItem(new CurveVector(ds, v, false));
    /// @todo add X tracking for the bottom panel.
    /// @todo Colors !
    view->addPanel(bottomPanel);
    
    stackedViews->addWidget(view);
    views << view;
    bufferSelection->addItem(ds->name);
  }
  layout->addWidget(stackedViews);

  layout->addWidget(bufferSelection);
  connect(bufferSelection, SIGNAL(currentIndexChanged(int)),
          SLOT(dataSetChanged(int)));


  QGridLayout * grid = new QGridLayout;

  globalGroup = new QButtonGroup(this);
  globalGroup->setExclusive(false);
  connect(globalGroup, SIGNAL(buttonClicked(int)),
          SLOT(onSetGlobal(int)));

  globalFixedGroup = new QButtonGroup(this);
  globalFixedGroup->setExclusive(false);
  connect(globalFixedGroup, SIGNAL(buttonClicked(int)),
          SLOT(onSetFixed(int)));


  localFixedGroup = new QButtonGroup(this);
  localFixedGroup->setExclusive(false);
  connect(localFixedGroup, SIGNAL(buttonClicked(int)),
          SLOT(onSetFixed(int)));

  for(int i = 0; i < data->parameterDefinitions.size(); i++) {
    const ParameterDefinition & p = data->parameterDefinitions[i];
    grid->addWidget(new QLabel(p.name), i, 0);
    QLineEdit * le = new QLineEdit;
    grid->addWidget(le, i, 1);
    connect(le, SIGNAL(textChanged(const QString &)),
            SLOT(updateFromEditors()));
    globalEditors << le;
    QCheckBox * cb = new QCheckBox(tr("Global"));
    grid->addWidget(cb, i, 2);
    globalGroup->addButton(cb, i);
    globalCheckBoxes << cb;

    cb = new QCheckBox(tr("Fixed"));
    grid->addWidget(cb, i, 3);
    globalFixedGroup->addButton(cb, i);
    globalFixedCheckBoxes << cb;


    le = new QLineEdit;
    grid->addWidget(le, i, 4);
    localEditors << le;
    connect(le, SIGNAL(textChanged(const QString &)),
            SLOT(updateFromEditors()));

    cb = new QCheckBox(tr("Fixed"));
    grid->addWidget(cb, i, 5);
    localFixedGroup->addButton(cb, i);
    localFixedCheckBoxes << cb;

  }

  layout->addLayout(grid);

  QHBoxLayout * hb = new QHBoxLayout;
  QPushButton * bt = new QPushButton(tr("Compute"));
  connect(bt, SIGNAL(clicked()), SLOT(compute()));
  hb->addWidget(bt);

  bt = new QPushButton(tr("Fit"));
  connect(bt, SIGNAL(clicked()), SLOT(startFit()));
  hb->addWidget(bt);

  bt = new QPushButton(tr("Cancel"));
  connect(bt, SIGNAL(clicked()), SLOT(reject()));
  hb->addWidget(bt);

  layout->addLayout(hb);
}


void FitDialog::dataSetChanged(int newds)
{
  stackedViews->setCurrentIndex(newds);
  currentIndex = newds;
  updateEditors();
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
  int sz = globalEditors.size();
  for(int i = 0; i < sz; i++) {
    setGlobal(isGlobal[i], i);
    onSetGlobal(i);             // Because the latter isn't
                                // necessarily called if the value
                                // hasn't changed.f
    if(isGlobal[i]) {
      globalEditors[i]->setText(QString::number(unpackedParameters[i]));
      localEditors[i]->setText("");
      globalFixedCheckBoxes[i]->setChecked(isFixed[i]);
    }
    else {
      globalEditors[i]->setText("");
      localEditors[i]->setText(QString::number(unpackedParameters[i + sz * currentIndex]));
      localFixedCheckBoxes[i]->setChecked(isFixed[i + sz * currentIndex]);
    }
    
  }
  settingEditors = false;
}

void FitDialog::setGlobal(bool global, int index)
{
  globalCheckBoxes[index]->setChecked(global);
}

void FitDialog::onSetGlobal(int index)
{
  isGlobal[index] = globalCheckBoxes[index]->isChecked();

  globalEditors[index]->setEnabled(isGlobal[index]);
  globalFixedCheckBoxes[index]->setEnabled(isGlobal[index]);

  localEditors[index]->setEnabled(! isGlobal[index]);
  localFixedCheckBoxes[index]->setEnabled(! isGlobal[index]);
}

void FitDialog::onSetFixed(int index)
{
  if(isGlobal[index]) {
    isFixed[index] = globalFixedCheckBoxes[index]->isChecked();
  }
  else {
    int sz = globalEditors.size();
    isFixed[index + currentIndex *sz] = 
      localFixedCheckBoxes[index]->isChecked();
  }

}

void FitDialog::updateFromEditors()
{
  if(settingEditors)
    return;
  int sz = globalEditors.size();
  for(int i = 0; i < sz; i++) {
    if(isGlobal[i])
      unpackedParameters[i] = globalEditors[i]->text().toDouble();
    else 
      unpackedParameters[i + sz * currentIndex] = 
        localEditors[i]->text().toDouble();
  }
}

void FitDialog::setDataParameters()
{
  data->parameters.clear();
  data->fixedParameters.clear();

  int nb_bufs = data->datasets.size();
  int sz = globalEditors.size();
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
