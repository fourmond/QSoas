/*
  fitdialog.cc: Main window for QSoas
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014 by CNRS/AMU

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
#include <parametersviewer.hh>

#include <nupwidget.hh>

#include <curvebrowser.hh>

static SettingsValue<QSize> fitDialogSize("fitdialog/size", QSize(700,500));

FitDialog::FitDialog(FitData * d, bool displayWeights, const QString & pm) : 
  data(d),
  nup(NULL),
  parameters(d),
  currentIndex(0),
  settingEditors(false), 
  displaySubFunctions(false),
  errorInconsistencyShown(false),
  perpendicularMeta(pm),
  progressReport(NULL),
  residualsDisplay(NULL),
  fitEngineFactory(NULL)
{
  setWindowModality(Qt::WindowModal);
  resize(fitDialogSize);


  if(displayWeights && d->datasets.size() > 1)
    bufferWeightEditor = new QLineEdit;
  else
    bufferWeightEditor = NULL;

  softOptions = data->fit->fitSoftOptions();
  if(softOptions && softOptions->size() == 0) {
    delete softOptions;
    softOptions = NULL;
  }

  // Here, setup the perpendicular coordinates
  if(data->datasets.size() > 1) {
    if(!perpendicularMeta.isEmpty()) {
      for(int i = 0; i < data->datasets.size(); i++) {
        const DataSet * ds = data->datasets[i];
        if(ds->getMetaData().contains(perpendicularMeta)) {
          parameters.perpendicularCoordinates << ds->getMetaData(perpendicularMeta).toDouble();
        }
      }
    }
    else {
      // Gather from datasets
      for(int i = 0; i < data->datasets.size(); i++) {
        const DataSet * ds = data->datasets[i];
        if(ds->perpendicularCoordinates().size() > 0)
          parameters.perpendicularCoordinates << ds->perpendicularCoordinates()[0];
      }
    }
    if(parameters.perpendicularCoordinates.size() > 0 && 
       parameters.perpendicularCoordinates.size() != data->datasets.size()) {
      Terminal::out << "Did get only " << parameters.perpendicularCoordinates.size() 
                    << " perpendicular coordinates, but I need " 
                    <<  data->datasets.size() << ", ignoring them" << endl;
      parameters.perpendicularCoordinates.clear();
    }
  }



  setupFrame();
  setFitEngineFactory(FitEngine::defaultFactoryItem(data->datasets.size()));

  if(data->fit->hasSubFunctions())
    displaySubFunctions = data->fit->displaySubFunctions();
  compute();

  

  updateEditors();
}

FitDialog::~FitDialog()
{
  fitDialogSize = size();
}

void FitDialog::message(const QString & str)
{
  if(progressReport) {
    progressReport->setText(str);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  }
}

void FitDialog::appendToMessage(const QString & str, bool format)
{
  if(progressReport) {
    QString txt = progressReport->text();
    if(format)
      txt = str.arg(txt);
    else
      txt += str;
    message(txt);
  }
}

void FitDialog::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  nup = new NupWidget;
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
    
    nup->addWidget(view);
    views << view;
    bufferSelection->addItem(Utils::shortenString(ds->name));
  }
  nup->setNup(1,1);
  connect(nup, SIGNAL(pageChanged(int)), SLOT(dataSetChanged(int)));
  connect(nup, SIGNAL(nupChanged(int,int)), SLOT(nupChanged()));

  layout->addWidget(nup, 1);

  //////////////////////////////////////////////////////////////////////
  // First line
  QHBoxLayout * hb = new QHBoxLayout;
  QPushButton * bt = new QPushButton(tr("<-"));
  nup->connect(bt, SIGNAL(clicked()), SLOT(previousPage()));
  hb->addWidget(bt);

  bt = new QPushButton(tr("->"));
  nup->connect(bt, SIGNAL(clicked()), SLOT(nextPage()));
  hb->addWidget(bt);


  // Here, we try to be clever a little about the size of that...
  QLabel * label = new QLabel("<b>Fit:</b> " + data->fit->fitName());
  label->setWordWrap(true);
  hb->addWidget(label, 1);

  hb->addWidget(bufferSelection);
  nup->connect(bufferSelection, SIGNAL(currentIndexChanged(int)),
               SLOT(showWidget(int)));


  
  bufferNumber = new QLabel();
  hb->addWidget(bufferNumber);

  QComboBox * cb = new QComboBox;
  cb->setEditable(true);
  cb->addItem("1 x 1");
  cb->addItem("4 x 4");
  cb->addItem("3 x 3");
  cb->addItem("2 x 2");
  cb->addItem("6 x 6");
  cb->addItem("3 x 2");
  nup->connect(cb, SIGNAL(activated(const QString&)), 
               SLOT(setNup(const QString &)));
  hb->addWidget(cb);


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
  nup->connect(bt, SIGNAL(clicked()), SLOT(previousPage()));
  hb->addWidget(bt);
  
  bt = new QPushButton(tr("->"));
  nup->connect(bt, SIGNAL(clicked()), SLOT(nextPage()));
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
      fitEngineSelection->addItem(it->description, it->name);
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
  ac->addAction("Push all residuals to stack", this, SLOT(pushResiduals()));

  if(parameters.perpendicularCoordinates.size() > 1)
    ac->addAction("Show transposed data", this, SLOT(showTransposed()));


  if(data->fit->hasSubFunctions()) {
    ac->addAction("Toggle subfunctions display", this, 
                  SLOT(toggleSubFunctions()));
    ac->addAction("Push all subfunctions", this, 
                  SLOT(pushSubFunctions()));
  }
  hb->addWidget(ac);
  
  ac = new ActionCombo(tr("Parameters..."));
  ac->addAction("Edit", this, 
                SLOT(editParameters()),
                QKeySequence(tr("Ctrl+E")));
  ac->addAction("Load from file", this, 
                SLOT(loadParameters()),
                QKeySequence(tr("Ctrl+L")));
  ac->addAction("Load for this buffer only", this, 
                SLOT(loadParametersForCurrent()),
                QKeySequence(tr("Ctrl+Shift+L")));
  ac->addAction("Save to file (for reusing later)", 
                this, SLOT(saveParameters()),
                QKeySequence(tr("Ctrl+S")));
  if(data->datasets.size() > 1)
    ac->addAction("Show parameters", 
                  this, SLOT(showParameters()));
  ac->addAction("Export (for drawing/manipulating)", 
                this, SLOT(exportParameters()),
                QKeySequence(tr("Ctrl+X")));
  ac->addAction("Export with errors", 
                this, SLOT(exportParametersWithErrors()),
                QKeySequence(tr("Ctrl+Shift+X")));
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

  if(softOptions) {
    bt = new QPushButton(tr("Fit options"));
    connect(bt, SIGNAL(clicked()), SLOT(setSoftOptions()));
    hb->addWidget(bt);
  }

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

  // Ctr + PgUp/PgDown to navigate the buffers
  Utils::registerShortCut(QKeySequence(tr("Ctrl+PgUp")), 
                          nup, SLOT(previousPage()));
  Utils::registerShortCut(QKeySequence(tr("Ctrl+PgDown")), 
                          nup, SLOT(nextPage()));

  

  nup->showWidget(0);
}


void FitDialog::closeEvent(QCloseEvent * event)
{
  shouldCancelFit = true;
  QDialog::closeEvent(event);
}


void FitDialog::dataSetChanged(int newds)
{
  // stackedViews->setCurrentIndex(newds);
  currentIndex = nup->widgetIndex();
  
  emit(currentDataSetChanged(currentIndex));
  if(! nup->isNup())
    updateEditors();
  QString txt = QString("%1/%2 %3 %4").
    arg(newds + 1).arg(data->datasets.size()).
    arg(data->fit->annotateDataSet(currentIndex)).
    arg(bufferWeightEditor ? " weight: " : "");
  if(parameters.perpendicularCoordinates.size() > 0) {
    QString str = QString(" %1 = %2").arg(perpendicularMeta.isEmpty() ? 
                                          "Z" : perpendicularMeta).
      arg(parameters.perpendicularCoordinates[currentIndex]);
    txt += str;
  }
  bufferNumber->setText(txt);

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

  subFunctions = parameters.computeSubFunctions();

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


void FitDialog::internalCompute()
{
  parameters.recompute();
  setupSubFunctionCurves();
  updateResidualsDisplay();

  // Update here ?
  // if(stackedViews && stackedViews->currentWidget())
  //   stackedViews->currentWidget()->repaint();
}

void FitDialog::compute()
{
  try {
    message("Computing...");
    internalCompute();
    appendToMessage(" done");
  }
  catch (const RuntimeError & re) {
    QString s = QString("An error occurred while computing: ") +
      re.message();
    message(s);
    Terminal::out << s << endl;
  }
}

void FitDialog::showEditors(bool show)
{
  int sz = editors.size();
  for(int i = 0; i < sz; i++)
    editors[i]->setVisible(show);
}

void FitDialog::nupChanged()
{
  showEditors(! nup->isNup());
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
    message(QString("Starting fit with %1 free parameters").
            arg(params));

  

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    /// @todo customize the number of iterations
    do {
      status = data->iterate();
      residuals = data->residuals();
      QString str = QString("Iteration #%1, current internal residuals: %2\n").
        arg(data->nbIterations).arg(residuals);
      Terminal::out << str << endl;

      message(str);
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
    appendToMessage(mention);
      
  }
  catch (const RuntimeError & re) {
    cancelButton->setVisible(false);
    startButton->setVisible(true);
    // We only take back the parameters if the fit really started !
    if(data->hasEngine()) {
      parameters.retrieveParameters();
      updateEditors();
    }
    message(QString("An error occurred while fitting: ") +
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
  try {
    internalCompute();
  }
  catch (const Exception & e) {
    appendToMessage(QString("Error while computing: ") + e.message());
  }
  emit(finishedFitting());
}



void FitDialog::cancelFit()
{
  shouldCancelFit = true;
}

DataSet *  FitDialog::simulatedData(int i, bool residuals)
{
  const DataSet * base = data->datasets[i];
  gsl_vector_view v =  data->viewForDataset(i, data->storage);
  Vector ny = Vector::fromGSLVector(&v.vector);
  if(residuals)
    ny = base->y() - ny;
  DataSet * ds = base->derivedDataSet(ny, (residuals ? "_delta_" : "_fit_")
                                      + data->fit->fitName(false) + ".dat");
  
  ds->setMetaData("fit", data->fit->fitName());
  QHash<QString, double> params = parameters.parametersForDataset(i);
  for(QHash<QString, double>::iterator i = params.begin(); 
      i != params.end(); i++)
    ds->setMetaData(i.key(), i.value());

  return ds;
}

void FitDialog::pushSubFunctions()
{
  int base = 0;
  for(int i = 0; i < views.size(); i++) {
    const DataSet * ds = data->datasets[i];
    int sz = ds->x().size();
    for(int j = 0; j < subFunctions.size(); j++) {
      Vector subY = subFunctions[j].mid(base, sz);

      soas().
        pushDataSet(ds->
                    derivedDataSet(subY,
                                   QString("_fit_%1_sub%2.dat").
                                   arg(data->fit->fitName(false)).arg(j+1)));
    }
    base += sz;
  }
}

                
void FitDialog::pushSimulatedCurves()
{
  for(int i = 0; i < views.size(); i++)
    soas().pushDataSet(simulatedData(i));
}

void FitDialog::pushResiduals()
{
  for(int i = 0; i < views.size(); i++)
    soas().pushDataSet(simulatedData(i, true));
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
  else {
    try {
      loadParametersFile(load);
    }
    catch (const Exception & e) {
      message(QString("Could not load parameters from '%1':").
              arg(load) + e.message());
    }
  }
}

void FitDialog::loadParametersForCurrent()
{
  QString load = 
    QFileDialog::getOpenFileName(this, 
                                 tr("Load parameters for dataset #%1").
                                 arg(currentIndex));
  if(load.isEmpty())
    return;
  else {
    try {
      loadParametersFile(load, currentIndex);
    }
    catch (const Exception & e) {
      message(QString("Could not load parameters from '%1':").
              arg(load) + e.message());
    }
  }
}

void FitDialog::loadParametersFile(const QString & file, int targetDS, 
                                   bool recompute)
{
  /// @todo When splitting this, the exceptions handling should be a
  /// lot better than just that.
  QFile f(file);
  Utils::open(&f,QIODevice::ReadOnly);

  QString msg;
  try {
    message(QString("Loading from file %1...").arg(file));
    parameters.loadParameters(&f, targetDS);
    updateEditors();
    if(recompute)
      internalCompute();
    msg = QString("Loaded fit parameters from file %1").arg(file);
  }
  catch (const Exception & e) {
    msg = QString("Could not load parameters from '%1':").
      arg(file) + e.message();
  }
  Terminal::out << msg << endl;
  message(msg);
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

void FitDialog::setParameterValue(const QString & name, double value, int ds)
{
  parameters.setValue(name, value, ds);
  updateEditors();
}





void FitDialog::promptExport(bool errors)
{
  QString save = 
    QFileDialog::getSaveFileName(this, tr("Export parameters"));
  if(save.isEmpty())
    return;
           

  QFile f(save);
  if(! f.open(QIODevice::WriteOnly))
    return;                     /// @todo Signal !

  parameters.exportParameters(&f, errors);
  Terminal::out << "Exported fit parameters to file " << save << endl;
}

void FitDialog::exportParametersWithErrors()
{
  promptExport(true);
}

void FitDialog::exportParameters()
{
  promptExport(false);
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
  message("Restored parameters");
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
  if(nup->isNup())
    s = "";
  else {
    if(! parameters.pointResiduals.size())
      s = "(impossible to compute residuals)";
    else
      s = s.arg(parameters.pointResiduals[currentIndex], 0, 'g', 2).
        arg(parameters.overallPointResiduals, 0, 'g', 2).
        arg(parameters.relativeResiduals[currentIndex], 0, 'g', 2).
        arg(parameters.overallRelativeResiduals, 0, 'g', 2);
  }
  residualsDisplay->setText(s);
}

void FitDialog::recomputeErrors()
{
  parameters.prepareFit(fitEngineFactory);
  parameters.recomputeJacobian();
}

void FitDialog::setSoftOptions()
{
  QDialog dlg;
  
  QVBoxLayout * l= new QVBoxLayout(&dlg);
  QGridLayout * gd = new QGridLayout;
  
  l->addLayout(gd);

  QHBoxLayout * bot = new QHBoxLayout;
  
  QPushButton * bt = new QPushButton(tr("OK"));
  dlg.connect(bt, SIGNAL(clicked()), SLOT(accept()));
  bot->addWidget(bt);

  bt = new QPushButton(tr("Cancel"));
  dlg.connect(bt, SIGNAL(clicked()), SLOT(reject()));
  bot->addWidget(bt);

  l->addLayout(bot);

  // Now, we populate the grid with editors;

  CommandOptions co = data->fit->currentSoftOptions();

  QHash<QString, QWidget *> widgets;

  for(int i = 0; i < softOptions->size(); i++) {
    Argument * ag = softOptions->value(i);
    gd->addWidget(new QLabel(ag->publicName()), i, 0);
    QWidget * w = ag->createEditor(&dlg);
    gd->addWidget(w, i, 1);
    widgets[ag->argumentName()] = w;

    if(co.contains(ag->argumentName())) {
      ag->setEditorValue(w, co[ag->argumentName()]);
    }
  }

  for(auto i = co.begin(); i != co.end(); i++)
    delete i.value();

  co.clear();

  if(dlg.exec()) {
    for(int i = 0; i < softOptions->size(); i++) {
      Argument * ag = softOptions->value(i);
      QWidget * w = widgets[ag->argumentName()];
      ArgumentMarshaller * val = ag->getEditorValue(w);
      if(val)
        co[ag->argumentName()] = val;
    }
    data->fit->processSoftOptions(co);
    for(auto i = co.begin(); i != co.end(); i++)
      delete i.value();
  }


  
}

void FitDialog::showTransposed()
{
  // First, check that all the X values are the same. Else, we don't
  // show transposed data.

  const Vector & x = data->datasets[0]->x();
  for(int i = 1; i < data->datasets.size(); i++) {
    if(data->datasets[i]->x() != x) {
      // Will wreak havoc is X is NaN, but you shouldn't really have
      // that in fits anyway.
      QMessageBox::warning(this, "Operation impossible",
                           "Cannot show transposed data if X "
                           "values are not identical");
      return;
    }
  }
  
  CurveBrowser dlg(x.size());

  const Vector & nx = parameters.perpendicularCoordinates;
  
  for(int i = 0; i < x.size(); i++) {
    CurveData * dat = new CurveData(nx, nx); // Transposed data
    CurveData * fit = new CurveData(nx, nx); // Transposed fit
    CurveData * diff = new CurveData(nx, nx); // Transposed diff

    for(int j = 0; j < data->datasets.size(); j++) {
      double yval = data->datasets[j]->y()[i];
      dat->yvalues[j] = yval;

      gsl_vector_view v = data->viewForDataset(j, data->storage);
      double yfit = gsl_vector_get(&v.vector, i);
      fit->yvalues[j] = yfit;

      diff->yvalues[j] = yval - yfit;
    }
    dat->countBB = true;
    dat->pen.setColor("#F00");

    fit->pen.setStyle(Qt::DashLine);
    fit->pen.setColor("#080");

    dlg.view(i)->addItem(dat);
    dlg.view(i)->addItem(fit);

    diff->pen.setStyle(Qt::DashLine);
    diff->pen.setColor("#080");
    diff->countBB = true;

    CurvePanel * bottomPanel = new CurvePanel(); // Leaks memory !
    bottomPanel->stretch = 30;
    bottomPanel->drawingXTicks = false;
    bottomPanel->drawingLegend = false;

    bottomPanel->addItem(diff);
    dlg.view(i)->addPanel(bottomPanel);

    dlg.setLabel(i, QString("Original X: %1").arg(x[i]));
  }
  dlg.setPage(0);
  dlg.exec();
}

void FitDialog::showParameters()
{
  ParametersViewer dlg(&parameters);
  dlg.exec();
}




