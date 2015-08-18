/*
  parametersdialog.cc: implementation of the ParametersDialog box
  Copyright 2011 by Vincent Fourmond
            2012 by CNRS/AMU

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
#include <fit.hh>
#include <fitdata.hh>
#include <dataset.hh>
#include <fitworkspace.hh>
#include <fitparametereditor.hh>
#include <parametersdialog.hh>


#include <settings-templates.hh>

static SettingsValue<QSize> parametersDialogSize("parametersdialog/size", 
                                                 QSize(700,500));

ParametersDialog::ParametersDialog(FitWorkspace * params) :
  parameters(params)
{
  resize(parametersDialogSize);
  setupFrame();
}

ParametersDialog::~ParametersDialog()
{
  parametersDialogSize = size();
}


void ParametersDialog::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);

  QScrollArea * scrollArea = new QScrollArea;

  QWidget * w = new QWidget;
  QGridLayout * gd = new QGridLayout(w);

  const QList<ParameterDefinition> & defs = 
    parameters->data()->parameterDefinitions;

  nbParams = defs.size();
  QList<const DataSet *> datasets = parameters->data()->datasets;

  nbDatasets = datasets.size();

  int curRow = 0;
  for(int i = 0; i < datasets.size(); i++) {
    if(datasets.size() > 1)
      gd->addWidget(new QLabel(datasets[i]->name), curRow, 0);
      for(int j = 0; j < nbParams; ++j, ++curRow) {
        FitParameterEditor * edit = 
          new FitParameterEditor(&defs[j], j, parameters, true, false, i);
        edit->updateFromParameters();
        gd->addWidget(edit, curRow, 1);
        editors << edit;
        connect(edit, SIGNAL(globalChanged(int, bool)),
                SLOT(onGlobalChanged(int, bool)));
      }
  }

  // Setup global stuff:
  for(int i = 0; i < nbParams; i++)
    if(parameters->isGlobal(i))
      onGlobalChanged(i, true);

  scrollArea->setWidget(w);
  layout->addWidget(scrollArea);
  QPushButton * bt = new QPushButton(tr("Close"));
  connect(bt, SIGNAL(clicked()), SLOT(close()));
  layout->addWidget(bt);
}

void ParametersDialog::onGlobalChanged(int index, bool global)
{
  if(global) {
    editors[index]->updateFromParameters();
    for(int i = 1; i < nbDatasets; i++)
      editors[index + i * nbParams]->setEnabled(false);
  }
  else {
    for(int i = 0; i < nbDatasets; i++) {
      editors[index + i * nbParams]->setEnabled(true);
      editors[index + i * nbParams]->updateFromParameters();
    }
  }
}
