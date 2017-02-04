/*
  parametersviewer.cc: Implementation of the parameters viewer
  Copyright 2014, 2017 by CNRS/AMU

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
#include <curvepoints.hh>
#include <curvepanel.hh>

#include <xyiterable.hh>
#include <tuneabledatadisplay.hh>

#include <soas.hh>
#include <graphicssettings.hh>

#include <flowinggridlayout.hh>
#include <fitdata.hh>
#include <fitworkspace.hh>
#include <fit.hh>

#include <terminal.hh>

//////////////////////////////////////////////////////////////////////

static SettingsValue<QSize> viewerSize("parametersviewer/size", QSize(700,500));

ParametersViewer::ParametersViewer(FitWorkspace * params) :
  parameters(params)
{
  resize(viewerSize);
  makePerpendicularCoordinates();
  setupFrame();
}

ParametersViewer::~ParametersViewer()
{
  viewerSize = size();
}

void ParametersViewer::makePerpendicularCoordinates()
{
  perpendicularCoordinates = parameters->perpendicularCoordinates;
  int nbds = parameters->data()->datasets.size();
  if(perpendicularCoordinates.size() != nbds) {
    Terminal::out << "Wrong number of perpendicular coordinates, making them up" << endl;
    perpendicularCoordinates .clear();
    for(int i = 0; i < nbds; i++)
      perpendicularCoordinates  << i;
  }
}

void ParametersViewer::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  view = new CurveView;
  layout->addWidget(view, 1);

  FlowingGridLayout * ml = new FlowingGridLayout;
  layout->addLayout(ml);        // I prefer to add afterwards, but it
                                // sometimes means loads of unmanaged
                                // widgets piling up temporarily on
                                // the screen.

  int nbparams = parameters->data()->parametersPerDataset();
  for(int i = 0; i < nbparams; i++) {
    QColor color = soas().graphicsSettings().dataSetPen(i).color();
    TuneableDataDisplay * tdd =
      new TuneableDataDisplay(parameters->parameterName(i),
                              view, i != nbparams-1, color);
    
    CurvePoints * cds =
      tdd->addSource(new XYIGSLVectors(perpendicularCoordinates.toGSLVector(),
                                       parameters->parameterVector(i),
                                       parameters->errorVector(i)));
    cds->relativeErrorBar = true;
    cds->pen = color;
    cds->brush = color;
    cds->size = 5;              /// @todo make that customizable
    cds->countBB = true;

    ml->addWidget(tdd);
  }

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

void ParametersViewer::pushVisible()
{
  // for(int i = 0; i < datasets.size(); i++)
  //   if(! curveDatasets[i]->hidden)
  //     soas().pushDataSet(new DataSet(*datasets[i]));
}

void ParametersViewer::pushAll()
{
  // for(int i = 0; i < datasets.size(); i++)
  //   soas().pushDataSet(new DataSet(*datasets[i]));
}
