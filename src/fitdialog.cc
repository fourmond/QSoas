/*
  fitdialog.cc: Main window for QSoas
  Copyright 2011, 2012, 2013 by Vincent Fourmond

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
#include <fitengine.hh>
#include <fitparametereditor.hh>
#include <curveview.hh>
#include <dataset.hh>

#include <curvevector.hh>
#include <curvepanel.hh>
#include <soas.hh>
#include <terminal.hh>

#include <curveitems.hh>

#include <actioncombo.hh>
#include <vector.hh>

#include <settings-templates.hh>
#include <graphicssettings.hh>

#include <flowinggridlayout.hh>
#include <parametersdialog.hh>

#include <utils.hh>
#include <fittrajectorydisplay.hh>

static SettingsValue<QSize> fitDialogSize("fitdialog/size", QSize(700,500));

FitDialog::FitDialog(FitData * d, bool displayWeights) : 
  data(d),
  stackedViews(NULL), 
  parameters(d),
  currentIndex(0),
  settingEditors(false), 
  displaySubFunctions(false),
  errorInconsistencyShown(false),
  progressReport(NULL),
  residualsDisplay(NULL),
  trajectoryDisplay(NULL),
  fitEngineFactory(NULL)
{
  setWindowModality(Qt::WindowModal);
  resize(fitDialogSize);

  compute();
  if(displayWeights && d->datasets.size() > 1)
    bufferWeightEditor = new QLineEdit;
  else
    bufferWeightEditor = NULL;
  setupFrame();
  setFitEngineFactory(FitEngine::defaultFactoryItem());

  
  if(data->fit->hasSubFunctions())
    displaySubFunctions = data->fit->displaySubFunctions();

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

    CurvePanel * bottomPanel = new CurvePanel(); // Leaks memory !
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
    bufferSelection->addItem(Utils::shortenString(ds->name));
  }
  layout->addWidget(stackedViews, 1);

  //////////////////////////////////////////////////////////////////////
  // First line
  QHBoxLayout * hb = new QHBoxLayout;
  QPushButton * bt = new QPushButton(tr("<-"));
  connect(bt, SIGNAL(clicked()), SLOT(previousDataset()));
  hb->addWidget(bt);

  bt = new QPushButton(tr("->"));
  connect(bt, SIGNAL(clicked()), SLOT(nextDataset()));
  hb->addWidget(bt);


  // Here, we try to be clever a little about the size of that...
  QLabel * label = new QLabel("<b>Fit:</b> " + data->fit->fitName());
  label->setWordWrap(true);
  hb->addWidget(label, 1);

  hb->addWidget(bufferSelection);
  connect(bufferSelection, SIGNAL(currentIndexChanged(int)),
          SLOT(dataSetChanged(int)));
  
  bufferNumber = new QLabel();
  hb->addWidget(bufferNumber);

  if(bufferWeightEditor) {
    hb->addWidget(bufferWeightEditor);
    connect(bufferWeightEditor, SIGNAL(textEdited(const QString &)),
            SLOT(weightEdited(const QString &)));

    bt = new QPushButton(tr("x 2"));
    connect(bt, SIGNAL(clicked()), SLOT(doubleWeight()));
    hb->addWidget(bt);

    bt = new QPushButton(tr("/ 2"));
    connect(bt, SIGNAL(clicked()), SLOT(halfWeight()));
    hb->addWidget(bt);
  }

  bt = new QPushButton(tr("<-"));
  connect(bt, SIGNAL(clicked()), SLOT(previousDataset()));
  hb->addWidget(bt);
  
  bt = new QPushButton(tr("->"));
  connect(bt, SIGNAL(clicked()), SLOT(nextDataset()));
  hb->addWidget(bt);

  layout->addLayout(hb);

  //////////////////////////////////////////////////////////////////////
  // Parameters


  int nbParams = data->parametersPerDataset();
  FlowingGridLayout * inner = new FlowingGridLayout();
  for(int i = 0; i < nbParams; i++) {
    FitParameterEditor * ed = 
      new FitParameterEditor(&data->parameterDefinitions[i], i, 
                             &parameters);
    inner->addWidget(ed);
    editors.append(ed);
    ed->connect(this, SIGNAL(currentDataSetChanged(int)), 
                SLOT(selectDataSet(int)));
  }
  layout->addLayout(inner);


  //////////////////////////////////////////////////////////////////////
  // Progress line (with fit engine choice)

  hb = new QHBoxLayout;
  
  progressReport = new QLabel(" ");
  progressReport->setWordWrap(true);
  progressReport->setTextFormat(Qt::RichText);
  hb->addWidget(progressReport, 1);

  residualsDisplay = new QLabel(" ");
  hb->addWidget(residualsDisplay);

  hb->addWidget(new QLabel("Fit engine:"));

  fitEngineSelection = new QComboBox;
  {
    QStringList fengines = FitEngine::availableEngines();
    for(int i = 0; i < fengines.size(); i++) {
      FitEngineFactoryItem * it = FitEngine::namedFactoryItem(fengines[i]);
      fitEngineSelection->addItem(it->publicName, it->name);
    }
    connect(fitEngineSelection, SIGNAL(activated(int)), 
            SLOT(engineSelected(int)));
  }
  hb->addWidget(fitEngineSelection);
  layout->addLayout(hb);

  // Bottom


  hb = new QHBoxLayout;
  hb->addWidget(new QLabel(tr("<b>Actions:</b>")));
  ActionCombo * ac = new ActionCombo(tr("Data..."));
  ac->addAction("Push all to stack", this, SLOT(pushSimulatedCurves()));
  ac->addAction("Push current to stack", this, SLOT(pushCurrentCurve()));
  ac->addAction("Save all", this, SLOT(saveSimulatedCurves()));
  if(data->fit->hasSubFunctions())
    ac->addAction("Toggle subfunctions display", this, 
                  SLOT(toggleSubFunctions()));
  hb->addWidget(ac);
  
  ac = new ActionCombo(tr("Parameters..."));
  ac->addAction("Load from file", this, 
                SLOT(loadParameters()),
                QKeySequence(tr("Ctrl+L")));
  ac->addAction("Load for this buffer only", this, 
                SLOT(loadParametersForCurrent()),
                QKeySequence(tr("Ctrl+Shift+L")));
  ac->addAction("Save to file (for reusing later)", 
                this, SLOT(saveParameters()),
                QKeySequence(tr("Ctrl+S")));
  ac->addAction("Export (for drawing/manipulating)", 
                this, SLOT(exportParameters()),
                QKeySequence(tr("Ctrl+X")));
  ac->addAction("Export to output file", this, 
                SLOT(exportToOutFile()),
                QKeySequence(tr("Ctrl+O")));
  ac->addAction("Export to output file (w/errors)", this, 
                SLOT(exportToOutFileWithErrors()),
                QKeySequence(tr("Ctrl+Shift+O")));
  ac->addAction("Reset this to initial guess", this, 
                SLOT(resetThisToInitialGuess()),
                QKeySequence(tr("Ctrl+T")));
  ac->addAction("Reset all to initial guess !", this, 
                SLOT(resetAllToInitialGuess()));
  ac->addAction("Reset to backup", this, 
                SLOT(resetParameters()),
                QKeySequence(tr("Ctrl+Shift+R")));
  ac->addAction("Show Trajectories", this, 
                SLOT(displayTrajectories()),
                QKeySequence(tr("Ctrl+Shift+T")));
  ac->addAction("Show covariance matrix", this, 
                SLOT(showCovarianceMatrix()),
                QKeySequence(tr("Ctrl+M")));

  if(bufferWeightEditor) {
    hb->addWidget(ac);
    
    ac = new ActionCombo(tr("Weights"));
    ac->addAction("Reset all weights to 1", this, 
                  SLOT(resetWeights()),
                  QKeySequence(tr("Ctrl+Shift+W")));
    ac->addAction("Give equal importance to all buffers", this, 
                  SLOT(equalWeightsPerBuffer()),
                  QKeySequence(tr("Ctrl+Shift+B")));
    ac->addAction("Give equal importance to all points", this, 
                  SLOT(equalWeightsPerPoint()),
                  QKeySequence(tr("Ctrl+Shift+P")));
  }
  hb->addWidget(ac);


  ac = new ActionCombo(tr("Print..."));
  ac->addAction("Save all as PDF", this, SLOT(saveAllPDF()));


  hb->addWidget(ac);
  hb->addStretch(1);

  bt = new QPushButton(tr("Update curves (Ctrl+U)"));
  connect(bt, SIGNAL(clicked()), SLOT(compute()));


  hb->addWidget(bt);

  bt = new QPushButton(tr("Edit parameters (Ctrl+E)"));
  connect(bt, SIGNAL(clicked()), SLOT(editParameters()));
  hb->addWidget(bt);

  startButton = new QPushButton(tr("Fit (Ctrl+F)"));
  connect(startButton, SIGNAL(clicked()), SLOT(startFit()));
  startButton->setDefault(true);
  hb->addWidget(startButton);

  cancelButton = new QPushButton(tr("Abort (Ctrl+B)"));
  connect(cancelButton, SIGNAL(clicked()), SLOT(cancelFit()));
  hb->addWidget(cancelButton);
  cancelButton->setVisible(false);


  bt = new QPushButton(tr("Close (Ctrl+W)"));
  connect(bt, SIGNAL(clicked()), SLOT(close()));
  hb->addWidget(bt);


  layout->addLayout(hb);

  // Registering shortcuts:
  Utils::registerShortCut(QKeySequence(tr("Ctrl+U")), 
                          this, SLOT(compute()));
  Utils::registerShortCut(QKeySequence(tr("Ctrl+W")), 
                          this, SLOT(close()));
  Utils::registerShortCut(QKeySequence(tr("Ctrl+F")), 
                          this, SLOT(startFit()));
  Utils::registerShortCut(QKeySequence(tr("Ctrl+B")), 
                          this, SLOT(cancelFit()));
  Utils::registerShortCut(QKeySequence(tr("Ctrl+E")), 
                          this, SLOT(editParameters()));

  // Ctr + PgUp/PgDown to navigate the buffers
  Utils::registerShortCut(QKeySequence(tr("Ctrl+PgUp")), 
                          this, SLOT(previousDataset()));
  Utils::registerShortCut(QKeySequence(tr("Ctrl+PgDown")), 
                          this, SLOT(nextDataset()));

  

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
  emit(currentDataSetChanged(currentIndex));
  updateEditors();
  /// @todo annotation !
  bufferNumber->setText(QString("%1/%2 %3 %4").
                        arg(newds + 1).arg(data->datasets.size()).
                        arg(data->fit->annotateDataSet(newds)).
                        arg(bufferWeightEditor ? " weight: " : ""));
  updateResidualsDisplay();
}

void FitDialog::setupSubFunctionCurves()
{
  
  // Cleanup the storage
  for(int i = 0; i < subFunctionCurves.size(); i++)
    delete subFunctionCurves[i];
  subFunctionCurves.clear();

  if(! displaySubFunctions)
    return;

  QList<Vector> subFunctions = parameters.computeSubFunctions();

  int base = 0;
  const GraphicsSettings & gs = soas().graphicsSettings();

  for(int i = 0; i < data->datasets.size(); i++) {
    const DataSet * ds = data->datasets[i];
    int sz = ds->x().size();
    for(int j = 0; j < subFunctions.size(); j++) {
      Vector subY = subFunctions[j].mid(base, sz);
      CurveData * dt = new CurveData(ds->x(), subY);
      dt->pen = gs.dataSetPen(j+2, true);

      views[i]->addItem(dt);
      subFunctionCurves << dt;
    }
    base += sz;
  }
    
}

void FitDialog::compute()
{
  try {
    parameters.recompute();
    setupSubFunctionCurves();
    updateResidualsDisplay();
  }
  catch (const RuntimeError & re) {
    QString s = QString("An error occurred while computing: ") +
      re.message();
    if(progressReport)
      progressReport->setText(s);
    Terminal::out << s << endl;
  }
  if(stackedViews && stackedViews->currentWidget())
    stackedViews->currentWidget()->repaint();
}


void FitDialog::updateEditors(bool updateErrors)
{
  settingEditors = true;
  int sz = editors.size();
  for(int i = 0; i < sz; i++)
    editors[i]->updateFromParameters(updateErrors);

  if(bufferWeightEditor)
    bufferWeightEditor->
      setText(QString::number(data->weightsPerBuffer[currentIndex]));

  settingEditors = false;
}


void FitDialog::startFit()
{
  QTime timer;
  timer.start();
  
  int iterationLimit = 150;

  int status = -1;
  double residuals = 0.0/0.0, relres = 0.0/0.0;

  if(! errorInconsistencyShown) {
    errorInconsistencyShown = true;
    if(! data->checkWeightsConsistency()) {
      QMessageBox::warning(this, "Error bar inconsistency" ,
                           "You are about to fit multiple buffers where some of the buffers have error bars and some others don't.\n\nErrors will NOT be taken into account !\nStart fit again to ignore");
      return;
    }
  }

  try {
    parameters.prepareFit(fitEngineFactory);
    parametersBackup = parameters.saveParameterValues();
    shouldCancelFit = false;
  
    QString params;
    int freeParams = data->freeParameters();
    if(data->independentDataSets())
      params = QString("%1 %2 %3").
        arg(freeParams / data->datasets.size()).
        arg(QChar(0xD7)).arg(data->datasets.size());
    else
      params = QString::number(freeParams);
  
    Terminal::out << "Starting fit '" << data->fit->fitName() << "' with "
                  << params << " free parameters"
                  << endl;

    cancelButton->setVisible(true);
    startButton->setVisible(false);
    progressReport->setText(QString("Starting fit with %1 free parameters").
                            arg(params));

  

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    /// @todo customize the number of iterations
    do {
      status = data->iterate();
      residuals = data->residuals();
      QString str = QString("Iteration #%1, current internal residuals: %2\n").
        arg(data->nbIterations).arg(residuals);
      Terminal::out << str << endl;

      progressReport->setText(str);
      parameters.retrieveParameters();
      updateEditors();

      QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
      if(shouldCancelFit || status != GSL_CONTINUE || 
         data->nbIterations >= iterationLimit)
        break;

    } while(true);
    cancelButton->setVisible(false);
    startButton->setVisible(true);

    QString mention;
    if(shouldCancelFit) {
      Terminal::out << "Fit cancelled" << endl;
      mention = " <font color=#d00>(cancelled)</font>";
    }
    else {
      QString st;
      if(status != GSL_SUCCESS) {
        parameters.retrieveParameters();
        updateEditors();
        mention = QString(" <font color=#d00>(%1)</font>").
          arg(gsl_strerror(status));
      }
      else {
        mention = " <font color=#080>(done)</font>";
        updateEditors(true);
      }
    }
    progressReport->setText(progressReport->text() + mention);
      
  }
  catch (const RuntimeError & re) {
    cancelButton->setVisible(false);
    startButton->setVisible(true);
    parameters.retrieveParameters();
    updateEditors();
    progressReport->setText(QString("An error occurred while fitting: ") +
                            re.message());
    QTextStream o(stdout);
    o << "Backtrace:\n\t" << re.exceptionBacktrace().join("\n\t") << endl;
  }

  trajectories << 
    FitTrajectory(parametersBackup, parameters.saveParameterValues(),
                  parameters.saveParameterErrors(),
                  residuals, relres, fitEngineFactory->name);
  if(shouldCancelFit)
    trajectories.last().ending = FitTrajectory::Cancelled;
  else if(data->nbIterations >= iterationLimit)
    trajectories.last().ending = FitTrajectory::TimeOut;
  else if(status != GSL_SUCCESS)
    trajectories.last().ending = FitTrajectory::Error;
    
  Terminal::out << "Fitting took an overall " << timer.elapsed() * 1e-3
                << " seconds, with " << data->evaluationNumber 
                << " evaluations" << endl;

  parameters.writeToTerminal();
  compute();
  emit(finishedFitting());
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
  return base->derivedDataSet(Vector::fromGSLVector(&v.vector), 
                              "_fit_" + data->fit->fitName(false) + ".dat");
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
  else 
    loadParametersFile(load);
}

void FitDialog::loadParametersForCurrent()
{
  QString load = 
    QFileDialog::getOpenFileName(this, 
                                 tr("Load parameters for dataset #%1").
                                 arg(currentIndex));
  if(load.isEmpty())
    return;
  else 
    loadParametersFile(load, currentIndex);
}

void FitDialog::loadParametersFile(const QString & file, int targetDS)
{
  QFile f(file);
  if(! f.open(QIODevice::ReadOnly))
    return;                     /// @todo Signal !
  QString msg;
  try {
    parameters.loadParameters(&f, targetDS);
    updateEditors();
    compute();
    msg = QString("Loaded fit parameters from file %1").arg(file);
  }
  catch (const Exception & e) {
    msg = QString("Could not load parameters from '%1':").
      arg(file) + e.message();
  }
  Terminal::out << msg << endl;
  progressReport->setText(msg);
}

void FitDialog::overrideParameter(const QString & name, double value)
{
  try {
    parameters.setValue(name, value);
    updateEditors();
    compute();
  }
  catch (const Exception & e) {
  }
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

void FitDialog::exportToOutFileWithErrors()
{
  parameters.exportToOutFile(true);
  Terminal::out << "Exported fit parameters and errors to output file"  << endl;
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
  QString fileName = 
    QFileDialog::getSaveFileName(this, tr("Save as"),
                                 QString(),
                                 tr("Documents (*.pdf *.ps)"));
  if(fileName.isEmpty())
    return;

  QPrinter p;
  p.setOrientation(QPrinter::Landscape);
  
  p.setOutputFileName(fileName);
  CurveView::nupPrint(&p, views, 3, 2);
}

void FitDialog::showCovarianceMatrix()
{
  CovarianceMatrixDisplay dlg(&parameters, this);
  dlg.exec();
}

void FitDialog::editParameters() 
{
  ParametersDialog dlg(&parameters);

  dlg.exec();
  updateEditors();
}

void FitDialog::weightEdited(const QString & str)
{
  bool ok = false;
  double nw = str.toDouble(&ok);
  if(ok)
    data->weightsPerBuffer[currentIndex] = nw;
}


void FitDialog::resetWeights()
{
  for(int i = 0; i < data->datasets.size(); i++)
    data->weightsPerBuffer[i] = 1;
  updateEditors();
}

void FitDialog::equalWeightsPerBuffer()
{
  // Now, we'll have to compute a weight for each buffer based on
  // their number of points/magnitude
  QVarLengthArray<double, 1024> weight(data->datasets.size());

  double max = 0;
  for(int i = 0; i < data->datasets.size(); i++) {
    data->weightsPerBuffer[i] = 1;
    double w = data->weightedSquareSumForDataset(i, NULL, true);

    weight[i] = 1/sqrt(w);
    if(weight[i] > max)
      max = weight[i];
  }
  for(int i = 0; i < data->datasets.size(); i++)
    data->weightsPerBuffer[i] = weight[i]/max;
  updateEditors();
}

void FitDialog::equalWeightsPerPoint()
{
  // Now, we'll have to compute a weight for each buffer based on
  // their number of points/magnitude
  QVarLengthArray<double, 1024> weight(data->datasets.size());

  double max = 0;
  for(int i = 0; i < data->datasets.size(); i++) {
    data->weightsPerBuffer[i] = 1;
    double w = data->weightedSquareSumForDataset(i, NULL, true);

    weight[i] = data->datasets[i]->nbRows()/sqrt(w);
    if(weight[i] > max)
      max = weight[i];
  }
  for(int i = 0; i < data->datasets.size(); i++)
    data->weightsPerBuffer[i] = weight[i]/max;
  updateEditors();
}

void FitDialog::doubleWeight()
{
  data->weightsPerBuffer[currentIndex] *= 2;
  updateEditors();
}

void FitDialog::halfWeight()
{
  data->weightsPerBuffer[currentIndex] *= 0.5;
  updateEditors();
}

void FitDialog::resetParameters()
{
  setParameterValues(parametersBackup);
  progressReport->setText(tr("Restored parameters"));
}

void FitDialog::displayTrajectories()
{
  // if(! trajectoryDisplay)
  //   trajectoryDisplay = new FitTrajectoryDisplay(this, data, &trajectories);
  FitTrajectoryDisplay dlg(this, data, &trajectories);
  dlg.update();
  dlg.exec();
}

void FitDialog::setFitEngineFactory(const QString & name)
{
  FitEngineFactoryItem * fact = FitEngine::namedFactoryItem(name);
  if(fact)
    setFitEngineFactory(fact);
}

void FitDialog::setFitEngineFactory(FitEngineFactoryItem * fact)
{
  fitEngineFactory = fact; 

  // Now update the combo box...
  int idx = fitEngineSelection->findData(fact->name);
  fitEngineSelection->setCurrentIndex(idx);
}

void FitDialog::engineSelected(int id)
{
  setFitEngineFactory(fitEngineSelection->itemData(id).toString());
}

void FitDialog::toggleSubFunctions()
{
  if(data->fit->hasSubFunctions()) {
    displaySubFunctions = ! displaySubFunctions;
    compute();                  // update display
  }
}


void FitDialog::setParameterValues(const Vector & values)
{
  parameters.restoreParameterValues(values);
  compute();
  updateEditors();
}

void FitDialog::updateResidualsDisplay()
{
  if(! residualsDisplay)
    return;
  QString s("Point: %1 (overall: %2)  Relative: %3 (overall: %4)");
  if(! parameters.pointResiduals.size())
    s = "(impossible to compute residuals)";
  else
    s = s.arg(parameters.pointResiduals[currentIndex], 0, 'g', 2).
      arg(parameters.overallPointResiduals, 0, 'g', 2).
      arg(parameters.relativeResiduals[currentIndex], 0, 'g', 2).
      arg(parameters.overallRelativeResiduals, 0, 'g', 2);
  residualsDisplay->setText(s);
}

void FitDialog::recomputeErrors()
{
  parameters.prepareFit(fitEngineFactory);
  parameters.recomputeJacobian();
}
