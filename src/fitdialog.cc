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
                                    stackedViews(NULL), 
                                    parameters(d),
                                    currentIndex(0),
                                    settingEditors(false)
{
  resize(fitDialogSize);

  compute();
  setupFrame();

  updateEditors();
}

FitDialog::~FitDialog()
{
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
  ac->addAction("Export", this, SLOT(exportParameters()));
  ac->addAction("Export to output file", this, SLOT(exportToOutFile()));
  hb->addWidget(ac);
  hb->addStretch(1);

  layout->addLayout(hb);

  progressReport = new QLabel(" ");
  layout->addWidget(progressReport);


  hb = new QHBoxLayout;
  bt = new QPushButton(tr("Update curves"));
  connect(bt, SIGNAL(clicked()), SLOT(compute()));
  hb->addWidget(bt);

  startButton = new QPushButton(tr("Fit"));
  connect(startButton, SIGNAL(clicked()), SLOT(startFit()));
  hb->addWidget(startButton);

  cancelButton = new QPushButton(tr("Cancel fit"));
  connect(cancelButton, SIGNAL(clicked()), SLOT(cancelFit()));
  hb->addWidget(cancelButton);
  cancelButton->setVisible(false);


  bt = new QPushButton(tr("Close"));
  connect(bt, SIGNAL(clicked()), SLOT(close()));
  hb->addWidget(bt);

  layout->addLayout(hb);
}


void FitDialog::closeEvent(QCloseEvent * event)
{
  shouldCancelFit = true;
  QDialog::closeEvent(event);
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
  parameters.recompute();
  if(stackedViews && stackedViews->currentWidget())
    stackedViews->currentWidget()->repaint();
}


void FitDialog::updateEditors()
{
  settingEditors = true;
  int sz = editors.size();
  for(int i = 0; i < sz; i++)
    editors[i]->setValues(parameters.getValue(i, currentIndex),
                          parameters.isFixed(i, currentIndex),
                          parameters.isGlobal(i));
  settingEditors = false;
}

void FitDialog::onSetGlobal(int index)
{
  parameters.setGlobal(index, editors[index]->isGlobal());
}

void FitDialog::onSetFixed(int index)
{
  parameters.setFixed(index, currentIndex, editors[index]->isFixed());
}

void FitDialog::onSetValue(int index, double v)
{
  parameters.setValue(index, currentIndex, v);
}


void FitDialog::startFit()
{
  QTime timer;
  timer.start();
  parameters.prepareFit();
  shouldCancelFit = false;
  
  Terminal::out << "Starting fit '" << data->fit->fitName() << "' with "
                << data->parameters.size() << " free parameters"
                << endl;


  cancelButton->setVisible(true);
  startButton->setVisible(false);
  progressReport->setText(QString("Starting fit with %1 free parameters").
                          arg(data->parameters.size()));

  

  QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  /// @todo customize the number of iterations
  while(data->iterate() == GSL_CONTINUE && 
        data->nbIterations < 100 && 
        ! shouldCancelFit) {
    int it = data->nbIterations;
    double residuals = data->residuals();
    QString str = QString("Iteration #%1, residuals: %2").
      arg(it).arg(residuals);
    Terminal::out << str << endl;

    progressReport->setText(str);
    parameters.retrieveParameters();
    updateEditors();

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  }
  cancelButton->setVisible(false);
  startButton->setVisible(true);
  if(shouldCancelFit) {
    Terminal::out << "Fit cancelled" << endl;
    progressReport->setText(progressReport->text() + " (cancelled)");
  }
  else 
    progressReport->setText(progressReport->text() + " (done)");
  Terminal::out << "Fitting took an overall " << timer.elapsed() * 1e-3
                << " seconds" << endl;
  
  // parameters.retrieveParameters();
  compute();
  // updateEditors();
}



void FitDialog::cancelFit()
{
  shouldCancelFit = true;
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
  QString save = 
    QFileDialog::getSaveFileName(this, tr("Save parameters"));
  if(save.isEmpty())
    return;
           

  QFile f(save);
  if(! f.open(QIODevice::WriteOnly))
    return;                     /// @todo Signal !

  parameters.saveParameters(&f);
  Terminal::out << "Saved fit parameters to file " << save << endl;
}

void FitDialog::loadParameters()
{
}

void FitDialog::exportParameters()
{
  QString save = 
    QFileDialog::getSaveFileName(this, tr("Export parameters"));
  if(save.isEmpty())
    return;
           

  QFile f(save);
  if(! f.open(QIODevice::WriteOnly))
    return;                     /// @todo Signal !

  parameters.exportParameters(&f);
  Terminal::out << "Exported fit parameters to file " << save << endl;
}

void FitDialog::exportToOutFile()
{
  parameters.exportToOutFile();
  Terminal::out << "Exported fit parameters to output file"  << endl;
}
