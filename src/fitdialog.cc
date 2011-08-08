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

FitDialog::FitDialog(FitData * d) : data(d), currentIndex(0)
{
  unpackedParameters = new double[d->fullParameterNumber()];
  d->fit->initialGuess(d, unpackedParameters);
  compute();
  setupFrame();
}

FitDialog::~FitDialog()
{
  delete[] unpackedParameters;
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
    view->addDataSet(ds);
    view->addItem(new CurveVector(ds, v));

    CurvePanel * bottomPanel = new CurvePanel();
    bottomPanel->stretch = 30;
    bottomPanel->drawingXTicks = false;
    bottomPanel->addItem(new CurveVector(ds, v, false));
    view->addPanel(bottomPanel);
    
    stackedViews->addWidget(view);
    views << view;
    bufferSelection->addItem(ds->name);
  }
  layout->addWidget(stackedViews);

  layout->addWidget(bufferSelection);
  connect(bufferSelection, SIGNAL(currentIndexChanged(int)),
          SLOT(dataSetChanged(int)));

  
  QHBoxLayout * hb = new QHBoxLayout;
  QPushButton * bt = new QPushButton(tr("Compute"));
  connect(bt, SIGNAL(clicked()), SLOT(compute()));
  hb->addWidget(bt);

  bt = new QPushButton(tr("Cancel"));
  connect(bt, SIGNAL(clicked()), SLOT(reject()));
  hb->addWidget(bt);
  layout->addLayout(hb);
}


void FitDialog::dataSetChanged(int newds)
{
  stackedViews->setCurrentIndex(newds);
  /// @todo Parameter changes !
}


void FitDialog::compute()
{
  data->fit->function(unpackedParameters, 
                      data, data->storage);
}
