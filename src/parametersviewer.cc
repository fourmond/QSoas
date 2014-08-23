/*
  parametersviewer.cc: Implementation of the parameters viewer
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
#include <parametersviewer.hh>
#include <settings-templates.hh>
#include <dataset.hh>
#include <curveview.hh>
#include <curvedataset.hh>
#include <curvepanel.hh>

#include <soas.hh>
#include <graphicssettings.hh>

#include <flowinggridlayout.hh>
#include <fitdata.hh>
#include <fitparameters.hh>
#include <fit.hh>

#include <terminal.hh>

//////////////////////////////////////////////////////////////////////

static SettingsValue<QSize> viewerSize("parametersviewer/size", QSize(700,500));

ParametersViewer::ParametersViewer(FitParameters * params) :
  parameters(params)
{
  resize(viewerSize);
  makeDatasets();
  setupFrame();
}

ParametersViewer::~ParametersViewer()
{
  viewerSize = size();
  for(int i = 0; i < datasets.size(); i++)
    delete datasets[i];
}

void ParametersViewer::makeDatasets()
{
  Vector coords = parameters->perpendicularCoordinates;
  int nbds = parameters->data()->datasets.size();
  if(coords.size() != nbds) {
    Terminal::out << "Wrong number of perpendicular coordinates, making them up" << endl;
    coords.clear();
    for(int i = 0; i < nbds; i++)
      coords << i;

    // Add it in a message label ?
  }

  int nbparams = parameters->data()->parametersPerDataset();
  for(int i = 0; i < nbparams; i++) {
    Vector y;
    Vector errs;
    for(int j = 0; j < nbds; j++) {
      double val = parameters->getValue(i, j);
      y << val;
      errs << parameters->getParameterError(i, j) * val;
    }
    DataSet * ds = new DataSet(coords, y, errs);
    ds->options.setYErrors(2);
    ds->name = parameters->data()->parameterDefinitions[i].name;
    datasets << ds;
  }
}

void ParametersViewer::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  view = new CurveView;
  layout->addWidget(view, 1);

  checkBoxes = new QButtonGroup(this);
  checkBoxes->setExclusive(false);
  FlowingGridLayout * ml = new FlowingGridLayout;
  layout->addLayout(ml);        // I prefer to add afterwards, but it
                                // sometimes means loads of unmanaged
                                // widgets piling up temporarily on
                                // the screen.
  int nbparams = datasets.size();
  for(int i = 0; i < nbparams; i++) {
    CurveDataSet * cds = new CurveDataSet(datasets[i]);
    curveDatasets << cds;
    cds->pen = soas().graphicsSettings().dataSetPen(i);
    QCheckBox * cb = new QCheckBox(datasets[i]->name);
    parametersBoxes << cb;
    checkBoxes->addButton(cb, i);
    ml->addWidget(cb);
    view->addItem(cds);
    bool on = (i == nbparams-1);
    cds->hidden = !on;
    cb->setChecked(on);
  }

  connect(checkBoxes, SIGNAL(buttonClicked(int)), SLOT(parameterChecked(int)));

  // Then, bottom line with buttons...
  QHBoxLayout * bl = new QHBoxLayout;
  layout->addLayout(bl);
  QPushButton * bt;

  bl->addStretch(1);

  bt = new QPushButton("Push visible");
  bl->addWidget(bt);
  connect(bt, SIGNAL(clicked()), SLOT(pushVisible()));

  bt = new QPushButton("Push all");
  bl->addWidget(bt);
  connect(bt, SIGNAL(clicked()), SLOT(pushAll()));

  bt = new QPushButton("Close");
  bl->addWidget(bt);
  connect(bt, SIGNAL(clicked()), SLOT(accept()));
}

void ParametersViewer::parameterChecked(int idx)
{
  curveDatasets[idx]->hidden = !curveDatasets[idx]->hidden;
  view->mainPanel()->zoomIn(QRectF());
  view->repaint();
}

void ParametersViewer::pushVisible()
{
  for(int i = 0; i < datasets.size(); i++)
    if(! curveDatasets[i]->hidden)
      soas().pushDataSet(new DataSet(*datasets[i]));
}

void ParametersViewer::pushAll()
{
  for(int i = 0; i < datasets.size(); i++)
    soas().pushDataSet(new DataSet(*datasets[i]));
}
