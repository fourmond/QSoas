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
#include <fitdata.hh>
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

#include <utils.hh>



FitParameterEditor::FitParameterEditor(const ParameterDefinition * d, 
                                       int idx, int totalDS, 
                                       int totalParams) : 
  index(idx), def(d)
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
  
  if(! d->canBeBufferSpecific) {
    global->setChecked(true);
    global->setEnabled(false);
  }

  if(totalDS <= 1)
    global->setVisible(false);

  if(totalParams >= 10) {
    global->setText("(G)");
    fixed->setText("(F)");
    sz.setWidth(5*sz.width()/6);
    editor->setMinimumSize(sz);
  }

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

    CurveVector * vec = new CurveVector(ds, v);
    vec->pen.setStyle(Qt::DashLine);
    vec->pen.setColor("#080");
    view->addItem(vec);

    CurvePanel * bottomPanel = new CurvePanel();
    bottomPanel->stretch = 30;
    bottomPanel->drawingXTicks = false;
    bottomPanel->drawingLegend = false;

    vec = new CurveVector(ds, v, true);
    vec->pen.setStyle(Qt::DashLine);
    vec->pen.setColor("#080");
    bottomPanel->addItem(vec);
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
  
  bufferNumber = new QLabel();
  hb->addWidget(bufferNumber);

  bt = new QPushButton(tr("->"));
  connect(bt, SIGNAL(clicked()), SLOT(nextDataset()));
  hb->addWidget(bt);

  layout->addLayout(hb);

  int nbParams = data->parameterDefinitions.size();
  FlowingGridLayout * inner = new FlowingGridLayout();
  for(int i = 0; i < nbParams; i++) {
    FitParameterEditor * ed = 
      new FitParameterEditor(&data->parameterDefinitions[i], i,
                             data->datasets.size(), nbParams);
    inner->addWidget(ed);
    editors.append(ed);
    connect(ed, SIGNAL(fixedChanged(int, bool)), SLOT(onSetFixed(int)));
    connect(ed, SIGNAL(globalChanged(int, bool)), SLOT(onSetGlobal(int)));
    connect(ed, SIGNAL(valueChanged(int, double)), 
            SLOT(onSetValue(int, double)));
  }
  layout->addLayout(inner);

  progressReport = new QLabel(" ");
  layout->addWidget(progressReport);


  hb = new QHBoxLayout;
  hb->addWidget(new QLabel(tr("<b>Actions:</b>")));
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
  ac->addAction("Reset this to initial guess", this, 
                SLOT(resetThisToInitialGuess()));
  ac->addAction("Reset all to initial guess !", this, 
                SLOT(resetAllToInitialGuess()));
  hb->addWidget(ac);

  ac = new ActionCombo(tr("Print..."));
  ac->addAction("Save all as PDF", this, SLOT(saveAllPDF()));

  hb->addWidget(ac);
  hb->addStretch(1);

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
  dataSetChanged(0);
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
  /// @todo annotation !
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
  
  QString params;
  if(data->independentDataSets())
    params = QString("%1 %2 %3").
      arg(data->parameters.size() / data->datasets.size()).
      arg(QChar(0xD7)).arg(data->datasets.size());
  else
    params = QString::number(data->parameters.size());
  
  Terminal::out << "Starting fit '" << data->fit->fitName() << "' with "
                << params << " free parameters"
                << endl;


  cancelButton->setVisible(true);
  startButton->setVisible(false);
  progressReport->setText(QString("Starting fit with %1 free parameters").
                          arg(params));

  

  QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  /// @todo customize the number of iterations
  int status;
  while((status = data->iterate(), status == GSL_CONTINUE) && 
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
  else {
    QString st;
    if(status != GSL_SUCCESS) {
      parameters.retrieveParameters();
      updateEditors();
      st = gsl_strerror(status);
    }
    else
      st = "done";
    progressReport->setText(progressReport->text() + 
                            QString(" (%1)").arg(st));
  }
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
  QList<DataSet *> newDs;
  QStringList fileNames;
  for(int i = 0; i < views.size(); i++) {
    DataSet * ds = simulatedData(i);
    newDs << ds;
    fileNames << ds->name;
  }

  QString msg = tr("Save all simulated curves as %1 ?").
    arg(fileNames.join(", "));
  /// @todo check for overwrite !
  if(Utils::askConfirmation(msg, tr("Save simulated curves")))
    for(int i = 0; i < newDs.size(); i++)
      newDs[i]->write();

  for(int i = 0; i < newDs.size(); i++)
    delete newDs[i];
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
  QString load = 
    QFileDialog::getOpenFileName(this, tr("Load parameters"));
  if(load.isEmpty())
    return;

  QFile f(load);
  if(! f.open(QIODevice::ReadOnly))
    return;                     /// @todo Signal !

  parameters.loadParameters(&f);
  updateEditors();
  compute();
  Terminal::out << "Loaded fit parameters from file " << load << endl;
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

void FitDialog::resetAllToInitialGuess()
{
  parameters.resetAllToInitialGuess();
  compute();
  updateEditors();
}

void FitDialog::resetThisToInitialGuess()
{
  parameters.resetToInitialGuess(currentIndex);
  compute();
  updateEditors();
}

void FitDialog::saveAllPDF()
{
  QPrinter p;
  p.setOrientation(QPrinter::Landscape);
  p.setOutputFileName("fits.pdf");
  CurveView::nupPrint(&p, views, 3, 2);
}
