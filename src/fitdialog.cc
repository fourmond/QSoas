/*
  fitdialog.cc: Main window for QSoas
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014, 2023 by CNRS/AMU

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
#include <datastack.hh>
#include <terminal.hh>

#include <commandwidget.hh>
#include <commandcontext.hh>

#include <curveitems.hh>

#include <actioncombo.hh>
#include <vector.hh>

#include <settings-templates.hh>
#include <graphicssettings.hh>

#include <flowinggridlayout.hh>
#include <parametersdialog.hh>

#include <utils.hh>
#include <parametersviewer.hh>
#include <parametersspreadsheet.hh>

#include <argumentsdialog.hh>

#include <nupwidget.hh>

#include <curvebrowser.hh>
#include <debug.hh>

#include <idioms.hh>
#include <icons.hh>
#include <helpbrowser.hh>

#include <gsl-types.hh>

#include <file.hh>
#include <datasetwriter.hh>

#include <fittrajectorydisplay.hh>

static SettingsValue<QSize> fitDialogSize("fitdialog/size", QSize(700,500));

static SettingsValue<int> fitIterationLimit("fitdialog/iteration-limit", 100);


static const char * saveFilters = "Parameter files (*.params);;Any file (*)";
static const char * exportFilters = "Data files (*.dat);;Any file (*)";

FitDialog::FitDialog(FitData * d, bool displayWeights, const QString & pm, bool expert, const QString & extra) :
  warnings(this),
  data(d),
  nup(NULL),
  softOptions(NULL),
  parameters(d),
  currentIndex(0),
  settingEditors(false), 
  displaySubFunctions(false),
  alreadyChangingPage(false),
  perpendicularMeta(pm),
  progressReport(NULL),
  residualsDisplay(NULL),
  extraTitleInfo(extra),
  menuBar(NULL)
{
  setWindowModality(Qt::WindowModal);
  resize(fitDialogSize);

  if(! theDialog)
    theDialog = new QPointer<FitDialog>;
  *theDialog = this;


  if(displayWeights && d->datasets.size() > 1)
    bufferWeightEditor = new QLineEdit;
  else
    bufferWeightEditor = NULL;

  ArgumentList opts = data->fit->fitSoftOptions();
  if(opts.size() > 0)
    softOptions = new ArgumentList(opts);

  parameters.computePerpendicularCoordinates(perpendicularMeta);
  parameters.warnings = &warnings;


  setupFrame(expert);
  updateEngineSelection(data->engineFactory);

  if(parameters.hasSubFunctions())
    displaySubFunctions = parameters.displaySubFunctions();
  compute();

  setIterationLimit(::fitIterationLimit);

  dataSetChanged(currentIndex);
  // updateEditors();

  // Sets the window title
  setWindowTitle(QString("QSoas fit: %1 -- %2 datasets%3").
                 arg(parameters.fitName()).
                 arg(data->datasets.size()).
                 arg(extraTitleInfo));

  connect(&parameters, SIGNAL(iterated(int, double,
                                       const Vector &)),
          SLOT(onIterate(int, double)));
  connect(&parameters, SIGNAL(finishedFitting(int)),
          SLOT(onFitEnd(int)));

  connect(&parameters, SIGNAL(startedFitting(int)),
          SLOT(onFitStart()));

  connect(&parameters, SIGNAL(quitWorkspace()),
          SLOT(accept()), Qt::QueuedConnection);

  connect(&parameters, SIGNAL(fitEngineFactoryChanged(FitEngineFactoryItem *)),
          SLOT(updateEngineSelection(FitEngineFactoryItem *)));
}

FitDialog::~FitDialog()
{
  fitDialogSize = size();
  if(parametersViewer)
    delete parametersViewer;
  for(int i = 0; i < subFunctionCurves.size(); i++)
    delete subFunctionCurves[i];
  delete softOptions;
  for(CurvePanel * p : bottomPanels)
    delete p;
  for(CurveView * v : views)
    delete v;
  delete menuBar;
}

void FitDialog::message(const QString & str, bool error)
{
  if(progressReport) {
    progressText = str;
    progressError = error;
    QString s;
    if(progressError)
      s = "<b><font color='red'>Error: </font></b> ";
    else
      s = "<b>Status: </b> ";
    s += progressText;
    progressReport->setText(s);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  }
  QString s = str;
  // Hmm. This isn't going to work on strings with HTML entities (&lt;)
  s.replace(QRegExp("<[^>]+>"),"");
  Terminal::out << s << endl;
}

void FitDialog::appendToMessage(const QString & str)
{
  if(progressReport) {
    QString txt = progressText;
    txt += str;
    message(txt, progressError);
  }
  else {
    QString s = str;
    s.replace(QRegExp("<[^>]+>"),"");
    Terminal::out << s << endl;
  }
}

static QAction * addMenuAction(QMenu * menu, const QString & name,
                               QObject * receiver, const char * slot,
                               const QKeySequence & shortCut = QKeySequence())
{
  QAction * ac = ActionCombo::createAction(name, receiver, slot,
                                           shortCut, receiver);
  menu->addAction(ac);
  return ac;
}

                          

void FitDialog::setupFrame(bool expert)
{
  QVBoxLayout * layout = new QVBoxLayout(this);

  
  nup = new NupWidget;
  bufferSelection = new QComboBox;

  menuBar = new QMenuBar();
  // We make a non native menu bar as it is messed up in macos
  menuBar->setNativeMenuBar(false);
  layout->addWidget(menuBar);

  const GraphicsSettings & gs = soas().graphicsSettings();
  for(int i = 0; i < data->datasets.size(); i++) {
    const DataSet * ds = data->datasets[i];
    CurveView * view = new CurveView;
    gsl_vector_view v = data->viewForDataset(i, data->storage);
    view->showDataSet(ds);

    CurveVector * vec = new CurveVector(ds, v);
    vec->pen = gs.getPen(GraphicsSettings::FitsPen);
    view->addItem(vec, true);

    CurvePanel * bottomPanel = new CurvePanel(); // Leaks memory !
    bottomPanel->stretch = 30;
    bottomPanel->drawingXTicks = false;
    bottomPanel->drawingLegend = false;

    vec = new CurveVector(ds, v, true);
    vec->pen = gs.getPen(GraphicsSettings::FitsPen);
    bottomPanel->addItem(vec, true);
    /// @todo add X tracking for the bottom panel.
    /// @todo Colors !
    view->addPanel(bottomPanel);
    
    nup->addWidget(view);
    views << view;
    bottomPanels << bottomPanel;
    int idx = 0;
    soas().stack().indexOf(ds, &idx);
    bufferSelection->addItem(QString("#%1: %2").
                             arg(idx).
                             arg(Utils::shortenString(ds->name)));
  }
  nup->setNup(1,1);
  connect(nup, SIGNAL(pageChanged(int)), SLOT(chooseDS(int)));
  connect(&parameters, SIGNAL(datasetChanged(int)), SLOT(dataSetChanged(int)));
  connect(nup, SIGNAL(nupChanged(int,int)), SLOT(nupChanged()));


  // Add a terminal
  // THis seems to make macos crash, for reasons that escape me
  if(! soas().isHeadless()) {
    QTabWidget * tabs = new QTabWidget;
    tabs->addTab(nup, "Fits");

    tabs->addTab(soas().prompt().createTerminalDisplay(), "Terminal");
    tabs->setTabPosition(QTabWidget::West);
    layout->addWidget(tabs, 1);
  }
  else
    layout->addWidget(nup, 1);

  
  

  //////////////////////////////////////////////////////////////////////
  // Progress report
  QHBoxLayout * hb = new QHBoxLayout;

  progressReport = new QLabel(" ");
  progressReport->setWordWrap(true);
  progressReport->setTextFormat(Qt::RichText);
  hb->addWidget(progressReport, 3);


  layout->addLayout(hb);

  //////////////////////////////////////////////////////////////////////
  // First line
  hb = new QHBoxLayout;
  QPushButton * bt =
    new QPushButton(Utils::standardIcon(QStyle::SP_ArrowLeft), "");
  nup->connect(bt, SIGNAL(clicked()), SLOT(previousPage()));
  hb->addWidget(bt);

  bt = new QPushButton(Utils::standardIcon(QStyle::SP_ArrowRight),"");
  nup->connect(bt, SIGNAL(clicked()), SLOT(nextPage()));
  hb->addWidget(bt);


  // Here, we try to be clever a little about the size of that...
  fitName = new QLabel();
  fitName->setWordWrap(true);
  updateFitName();
  hb->addWidget(fitName, 1);

  bt = new QPushButton("Hide Fixed");
  connect(bt, SIGNAL(clicked()), SLOT(hideFixedParameters()));
  hb->addWidget(bt);

  bt = new QPushButton("Show All");
  connect(bt, SIGNAL(clicked()), SLOT(showAllParameters()));
  hb->addWidget(bt);

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
    hb->addWidget(new QLabel("weight: "));
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

  bt = new QPushButton(Utils::standardIcon(QStyle::SP_ArrowLeft), "");
  nup->connect(bt, SIGNAL(clicked()), SLOT(previousPage()));
  hb->addWidget(bt);
  
  bt = new QPushButton(Utils::standardIcon(QStyle::SP_ArrowRight), "");
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


  // Prompt line, contains iterations too
  // First, prompt:
  hb = new QHBoxLayout;
  
  fitPrompt = new CommandWidget(CommandContext::fitContext());
  hb->addWidget(fitPrompt, 2);

  if(! expert)
    Terminal::out << "Now always using expert mode for fits" << endl;
  
  hb->addWidget(new QLabel("Max iterations:"));
  iterationLimitEditor = new QLineEdit();
  hb->addWidget(iterationLimitEditor);

  layout->addLayout(hb);

  // Then, the actions line

  

  hb = new QHBoxLayout;
  residualsDisplay = new QLabel(" ");
  hb->addWidget(residualsDisplay, 1);

  // Fit engine selection
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


  

  {
    auto addMenu = [this](QMenu * menu) {
                     menuBar->addMenu(menu);
                     menus << menu;
                   };

    //////////////////////////////
    // Menus

    QMenu * menu = NULL;

    menu = new QMenu("Fit");
    addMenuAction(menu, "Update curves",
                  this, SLOT(compute()), QKeySequence("Ctrl+U"));
    addMenuAction(menu, "Fit options",
                  this, SLOT(setSoftOptions()));
    addMenuAction(menu, "Start fit",
                  this, SLOT(startFit()), QKeySequence("Ctrl+F"));
    menu->addSeparator();
    addMenuAction(menu, "Save all as PDF", this, SLOT(saveAllPDF()));
    menu->addSeparator();
    addMenuAction(menu, "Close",
                  this, SLOT(close()), QKeySequence("Ctrl+W"));
    addMenu(menu);

    menu = new QMenu("Data");
    addMenuAction(menu, "Push all to stack",
                  this, SLOT(pushSimulatedCurves()));
    addMenuAction(menu, "Push current to stack",
                  this, SLOT(pushCurrentCurve()));
    addMenuAction(menu, "Push annotated datasets",
                  this, SLOT(pushAnnotatedData()));
    addMenuAction(menu, "Save all", this, SLOT(saveSimulatedCurves()));
    addMenuAction(menu, "Push all residuals to stack", this, SLOT(pushResiduals()));

    QAction * action = addMenuAction(menu, "Show transposed data",
                                     this, SLOT(showTransposed()));
    if(parameters.perpendicularCoordinates.size() <= 1)
      action->setEnabled(false);

    if(parameters.hasSubFunctions()) {
      menu->addSeparator();
      addMenuAction(menu, "Toggle subfunctions display", this, 
                    SLOT(toggleSubFunctions()));
      addMenuAction(menu, "Push all subfunctions", this, 
                    SLOT(pushSubFunctions()));
    }
    addMenu(menu);


  
    menu = new QMenu("Parameters");
    addMenuAction(menu, "Edit", this, 
                  SLOT(editParameters()),
                  QKeySequence(tr("Ctrl+E")));
    addMenuAction(menu, "Spreadsheet", this, 
                  SLOT(parametersSpreadsheet()),
                  QKeySequence(tr("Ctrl+Shift+E")));
    if(data->datasets.size() > 1)
      addMenuAction(menu, "Show parameters", 
                    this, SLOT(showParameters()),
                    QKeySequence(tr("Ctrl+Shift+P")));

    addMenuAction(menu, "Show covariance matrix", this, 
                  SLOT(showCovarianceMatrix()),
                  QKeySequence(tr("Ctrl+M")));
    menu->addSeparator();

  
    addMenuAction(menu, "Load from file", this, 
                  SLOT(loadParameters()),
                  QKeySequence(tr("Ctrl+L")));
    addMenuAction(menu, "Load for this buffer only", this, 
                  SLOT(loadParametersForCurrent()),
                  QKeySequence(tr("Ctrl+Shift+L")));
    action = addMenuAction(menu, "Load using Z values",
                           this, SLOT(loadUsingZValues()));
    if(parameters.perpendicularCoordinates.size() <= 1)
      action->setEnabled(false);
    menu->addSeparator();
    
    addMenuAction(menu, "Save to file (for reusing later)", 
                  this, SLOT(saveParameters()),
                  QKeySequence(tr("Ctrl+S")));
  
    addMenuAction(menu, "Export (for drawing/manipulating)", 
                  this, SLOT(exportParameters()),
                  QKeySequence(tr("Ctrl+X")));
    addMenuAction(menu, "Export with errors", 
                  this, SLOT(exportParametersWithErrors()),
                  QKeySequence(tr("Ctrl+Shift+X")));
    addMenuAction(menu, "Export to output file", this, 
                  SLOT(exportToOutFile()),
                  QKeySequence(tr("Ctrl+O")));
    addMenuAction(menu, "Export to output file (w/errors)", this, 
                  SLOT(exportToOutFileWithErrors()),
                  QKeySequence(tr("Ctrl+Shift+O")));
    menu->addSeparator();
    addMenuAction(menu, "Push with errors", 
                  this, SLOT(pushParametersWithErrors()));
    menu->addSeparator();
    addMenuAction(menu, "Reset this to initial guess", this, 
                  SLOT(resetThisToInitialGuess()),
                  QKeySequence(tr("Ctrl+T")));
    addMenuAction(menu, "Reset all to initial guess !", this, 
                  SLOT(resetAllToInitialGuess()));
    addMenuAction(menu, "Reset to backup", &parameters, 
                  SLOT(resetToBackup()),
                  QKeySequence(tr("Ctrl+Shift+R")));
    menu->addSeparator();
    addMenuAction(menu, "Browse fit trajectories", this, 
                  SLOT(browseTrajectories()),
                  QKeySequence(tr("Ctrl+Shift+T")));
    addMenu(menu);


    if(bufferWeightEditor) {
      menu = new QMenu("Weights");
      addMenuAction(menu, "Reset all weights to 1", this, 
                    SLOT(resetWeights()),
                    QKeySequence(tr("Ctrl+Shift+W")));
      addMenuAction(menu, "Give equal importance to all buffers", this, 
                    SLOT(equalWeightsPerBuffer()),
                    QKeySequence(tr("Ctrl+Shift+B")));
      addMenu(menu);
    }


    menuBar->setEnabled(true);
    menuBar->setNativeMenuBar(false);
    menuBar->setVisible(true);
  }

  
  // hb->addStretch(1);

  bt = new QPushButton(tr("Update curves (Ctrl+U)"));
  connect(bt, SIGNAL(clicked()), SLOT(compute()));
  if(fitPrompt)
    bt->setAutoDefault(false);

  hb->addWidget(bt);

  // bt = new QPushButton(tr("Fit options"));
  // connect(bt, SIGNAL(clicked()), SLOT(setSoftOptions()));
  // if(fitPrompt)
  //   bt->setAutoDefault(false);
  // hb->addWidget(bt);

  startButton = new QPushButton(tr("Fit (Ctrl+F)"));
  connect(startButton, SIGNAL(clicked()), SLOT(startFit()));
  if(fitPrompt)
    startButton->setAutoDefault(false);
  hb->addWidget(startButton);

  cancelButton = new QPushButton(tr("Abort (Ctrl+B)"));
  parameters.connect(cancelButton, SIGNAL(clicked()), SLOT(cancelFit()));
  hb->addWidget(cancelButton);
  if(fitPrompt)
    cancelButton->setAutoDefault(false);
  cancelButton->setVisible(false);


  bt = new QPushButton(Utils::standardIcon(QStyle::SP_DialogHelpButton),
                       "Help");
  connect(bt, SIGNAL(clicked()), SLOT(showHelp()));
  if(fitPrompt)
    bt->setAutoDefault(false);
  hb->addWidget(bt);

  bt = new QPushButton(Utils::standardIcon(QStyle::SP_DialogCloseButton),
                       "Close (Ctrl+W)");
  connect(bt, SIGNAL(clicked()), SLOT(close()));
  if(fitPrompt)
    bt->setAutoDefault(false);
  hb->addWidget(bt);


  layout->addLayout(hb);

  // Registering shortcuts:
  // Utils::registerShortCut(QKeySequence(tr("Ctrl+U")), 
  //                         this, SLOT(compute()));
  // Utils::registerShortCut(QKeySequence(tr("Ctrl+W")), 
  //                         this, SLOT(close()));
  // Utils::registerShortCut(QKeySequence(tr("Ctrl+F")), 
  //                         this, SLOT(startFit()));
  Utils::registerShortCut(QKeySequence(tr("Ctrl+B")), 
                          &parameters, SLOT(cancelFit()),
                          this /* here, we need the parent */);

  // Ctr + PgUp/PgDown to navigate the buffers
  Utils::registerShortCut(QKeySequence(tr("Ctrl+PgUp")), 
                          nup, SLOT(previousPage()), this);
  Utils::registerShortCut(QKeySequence(tr("Ctrl+PgDown")), 
                          nup, SLOT(nextPage()), this);

  

  if(fitPrompt) {
    fitPrompt->setFocus();
    fitPrompt->setPrompt("QSoas.fit> ");
  }
  nup->showWidget(0);
}



const QList<FitTrajectory> & FitDialog::fitTrajectories() const
{
  return trajectories;
}

FitData * FitDialog::getData() const
{
  return data;
}

void FitDialog::updateFitName()
{
  QString fn = parameters.fitName();
  if(fn.size() > 50) {
    // Insert invisible spaces at non-word boundaries
    QChar spc(0x200A);          // Not invisible in the end
    spc = ' ';
    QString repl = QString("\\1") + spc;
    // QRegExp re("(\\W+\\w+\\W+\\w+\\W+)");
    QRegExp re("(\\W+\\w+\\W+)");
    fn = fn.replace(re, repl);
  }
  
  fitName->setText("<b>Fit:</b> " + fn);
}

FitWorkspace * FitDialog::getWorkspace()
{
  return &parameters;
}

const FitWorkspace * FitDialog::getWorkspace() const
{
  return &parameters;
}

void FitDialog::closeEvent(QCloseEvent * event)
{
  parameters.cancelFit();
  event->ignore();
  reject();
  // For some reason, reverting to normal close events causes
  // segfaults from time to time
  
  // QDialog::closeEvent(event);
}

void FitDialog::setIterationLimit(int lim)
{
  iterationLimit = lim;
  iterationLimitEditor->setText(QString::number(lim));
}

int FitDialog::getIterationLimit() const
{
  return iterationLimitEditor->text().toInt();
}

void FitDialog::chooseDS(int newds)
{
  if(alreadyChangingPage)
    return;
  alreadyChangingPage = true;
  parameters.selectDataSet(nup->widgetIndex());
  alreadyChangingPage = false;
}


void FitDialog::dataSetChanged(int newds)
{
  // stackedViews->setCurrentIndex(newds);
  currentIndex = newds;
  
  emit(currentDataSetChanged(currentIndex));
  if(! nup->isNup())
    updateEditors();
  QString txt = QString("%1/%2 %3").
    arg(newds + 1).arg(data->datasets.size()).
    arg(parameters.annotateDataSet(currentIndex));
  if(parameters.perpendicularCoordinates.size() > 0) {
    QString str = QString(" %1 = %2").arg(perpendicularMeta.isEmpty() ? 
                                          "Z" : perpendicularMeta).
      arg(parameters.perpendicularCoordinates[currentIndex]);
    txt += str;
  }
  bufferNumber->setText(txt);
  bufferSelection->setCurrentIndex(currentIndex);
  

  updateResidualsDisplay();
}

void FitDialog::runCommandFile(const QString & file, const QStringList & args)
{
  fitPrompt->runCommandFile(file, args);
}

void FitDialog::setupSubFunctionCurves(bool dontSend)
{
  
  // Cleanup the storage
  for(int i = 0; i < subFunctionCurves.size(); i++)
    delete subFunctionCurves[i];
  subFunctionCurves.clear();

  if(! displaySubFunctions)
    return;

  QStringList ann;
  subFunctions = parameters.computeSubFunctions(dontSend, &ann);

  int base = 0;
  const GraphicsSettings & gs = soas().graphicsSettings();

  for(int i = 0; i < data->datasets.size(); i++) {
    const DataSet * ds = data->datasets[i];
    int sz = ds->x().size();
    for(int j = 0; j < subFunctions.size(); j++) {
      Vector subY = subFunctions[j].mid(base, sz);
      CurveData * dt = new CurveData(ds->x(), subY);
      dt->legend = ann[j];
      dt->pen = gs.dataSetPen(j+3, true);

      views[i]->addItem(dt);
      subFunctionCurves << dt;
    }
    base += sz;
  }
    
}


void FitDialog::internalCompute(bool dontSend)
{
  parameters.recompute(dontSend, false);
  setupSubFunctionCurves(dontSend);
  updateResidualsDisplay();

  nup->repaint();
}

void FitDialog::compute()
{
  try {
    message("Computing...");
    internalCompute();
    appendToMessage(" done");
  }
  catch (const Exception & re) {
    QString s = QString("An error occurred while computing: ") +
      re.message();
    message(s);
    Terminal::out << s << endl;
    if(Debug::debugLevel() > 0 || data->debug > 0)
      Debug::debug() << "Error: " << re.message() << endl
                     << "\nBacktrace:\n\t"
                     << re.exceptionBacktrace().join("\n\t") << endl;
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


void FitDialog::onFitStart()
{
  cancelButton->setVisible(true);
  startButton->setVisible(false);
  iterationLimitEditor->setEnabled(false);
}

void FitDialog::startFit()
{
  try {
    parameters.runFit(getIterationLimit());
  }
  catch (const Exception & re) {
    updateEditors();
    message(QString("An error occurred while fitting: ") +
            re.message());
    Debug::debug()
      << "Backtrace:\n\t" << re.exceptionBacktrace().join("\n\t") << endl;
  }
}

/// @todo Should this join Utils
QString htmlColor(const QColor & color)
{
  return QString("#%1%2%3").
    arg(color.red(), 2, 16, QChar('0')).
    arg(color.green(), 2, 16, QChar('0')).
    arg(color.blue(), 2, 16, QChar('0'));
}

void FitDialog::onFitEnd(int ending)
{
  FitWorkspace::Ending end =
    static_cast<FitWorkspace::Ending>(ending);
  QString msg = FitWorkspace::endingDescription(end);
  QColor col = FitWorkspace::endingColor(end);
  
  msg = QString("Finished fitting, final residuals: %1 "
                "<font color=%2>%3</font>").
    arg(FitWorkspace::formatResiduals(parameters.
                                      trajectories.last().residuals)).
    arg(htmlColor(col)).arg(msg);
  
  message(msg);
  cancelButton->setVisible(false);
  startButton->setVisible(true);
  iterationLimitEditor->setEnabled(true);
  try {
    internalCompute(true);
  }
  catch (const Exception & e) {
    message(msg + " -- error while computing: " + e.message(), true);
  }
}

void FitDialog::onIterate(int nb, double residuals)
{
  QString str = QString("Iteration #%1, current internal residuals: %2, %3 s elapsed").
    arg(nb).arg(FitWorkspace::formatResiduals(residuals)).
    arg(parameters.elapsedTime());
  message(str);

  parameters.retrieveParameters();
  updateEditors();
  QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}



void FitDialog::pushSubFunctions()
{
  parameters.pushComputedData(false, true);
}

                
void FitDialog::pushSimulatedCurves()
{
  parameters.pushComputedData();
}

void FitDialog::pushAnnotatedData()
{
  parameters.pushAnnotatedData();
}

void FitDialog::pushResiduals()
{
  parameters.pushComputedData(true);
}

void FitDialog::pushCurrentCurve()
{
  if(currentIndex >= 0)
    soas().pushDataSet(parameters.computedData(currentIndex));
}

void FitDialog::saveSimulatedCurves()
{
  QList<DataSet *> newDs;
  QStringList fileNames;
  for(int i = 0; i < views.size(); i++) {
    DataSet * ds = parameters.computedData(i);
    newDs << ds;
    fileNames << ds->name;
  }

  QString msg = tr("Save all simulated curves as %1 ?").
    arg(fileNames.join(", "));
  /// @todo check for overwrite !
  if(Utils::askConfirmation(msg, tr("Save simulated curves"))) {
    for(DataSet * ds : newDs) {
      DataSetWriter writer;
      File f(ds->name, File::TextWrite);
      writer.writeDataSet(&f, ds);
      delete ds;
    }
  }
  
}

void FitDialog::saveParameters()
{
  QString save = 
    QFileDialog::getSaveFileName(this, tr("Save parameters"),
                                 QString(), saveFilters);
  if(save.isEmpty())
    return;
  try {
    // We disable confirmation because we already asked for
    // confirmation before.
    parameters.saveParameters(save, true);
  }
  catch(RuntimeError & e) {
    message(e.message(), true);
  }
}

void FitDialog::loadParameters()
{
  QString load = 
    QFileDialog::getOpenFileName(this, tr("Load parameters"), QString(),
                                 saveFilters);
  if(load.isEmpty())
    return;
  else {
    try {
      loadParametersFile(load);
    }
    catch (const Exception & e) {
      message(QString("Could not load parameters from '%1': ").
              arg(load) + e.message(), true);
    }
  }
}

void FitDialog::loadUsingZValues()
{
  QString load = 
    QFileDialog::getOpenFileName(this, tr("Load parameter values"), QString(),
                                 saveFilters);
  if(load.isEmpty())
    return;
  else {
    try {
      loadParametersFile(load, -1, true, true);
    }
    catch (const Exception & e) {
      message(QString("Could not load parameters from '%1': ").
              arg(load) + e.message(), true);
    }
  }
}

void FitDialog::loadParametersForCurrent()
{
  QString load = 
    QFileDialog::getOpenFileName(this, 
                                 tr("Load parameters for dataset #%1").
                                 arg(currentIndex), QString(),
                                 saveFilters);
  if(load.isEmpty())
    return;
  else {
    try {
      loadParametersFile(load, currentIndex);
    }
    catch (const Exception & e) {
      message(QString("Could not load parameters from '%1': ").
              arg(load) + e.message(), true);
    }
  }
}

void FitDialog::loadParametersFile(const QString & file, int targetDS, 
                                   bool recompute, bool onlyVals)
{
  QString msg;
  bool error = false;
  try {
    message(QString("Loading from file %1...").arg(file));
    if(onlyVals)
      parameters.loadParametersValues(file);
    else
      parameters.loadParameters(file, targetDS);
    updateEditors();
    if(recompute)
      internalCompute();
    msg = QString("Loaded fit parameters %2from file %1").
      arg(file).arg(onlyVals ? "values " : "");
  }
  catch (const Exception & e) {
    msg = QString("Could not load parameters from '%1': ").
      arg(file) + e.message();
    error = true;
  }
  Terminal::out << msg << endl;
  message(msg, error);
}

void FitDialog::setParameterValue(const QString & name, double value, int ds)
{
  parameters.setValue(name, value, ds);
  updateEditors();
}

bool FitDialog::checkEngineForExport()
{
  if(! data->hasEngine()) {
    QMessageBox::warning(this, "No fit engine for export",
                         QString("There is no fit engine for exporting parameters. You must run a fit before exporting parameters (or try looking at the /operation=reexport option to the sim-%1 command)").arg(data->fit->fitName(false)));
    return false;
  }
  return true;
}


void FitDialog::promptExport(bool errors)
{
  if(! checkEngineForExport())
    return;
  QString save = 
    QFileDialog::getSaveFileName(this, tr("Export parameters"), QString(),
                                 exportFilters);
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
  if(! checkEngineForExport())
    return;
  parameters.exportToOutFile();
  Terminal::out << "Exported fit parameters to output file"  << endl;
}

void FitDialog::pushParametersWithErrors()
{
  if(! checkEngineForExport())
    return;
  Terminal::out << "Pushing fit parameters to the stack"  << endl;
  DataSet * ds = parameters.exportAsDataSet(true, true);
  soas().pushDataSet(ds);
}

void FitDialog::exportToOutFileWithErrors()
{
  if(! checkEngineForExport())
    return;
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
  if(parameters.totalParameterNumber() > 60) {
    if(! warnings.warnOnce("edit-large",
                           "Many parameters",
                           QString("There are many (%1) parameters in this fit, launching edit will probably be way too slow").
                           arg(parameters.totalParameterNumber())))
      return;
  }

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
  GSLVector dat(data->dataPoints());

  // Copy buffer
  for(int i = 0; i < data->datasets.size(); i++) {
    data->weightsPerBuffer[i] = 1;
    gsl_vector_view v = data->viewForDataset(i, dat);
    gsl_vector_memcpy(&v.vector, data->datasets[i]->y());
  }
  data->weightVector(dat);

  double max = 0;
  for(int i = 0; i < data->datasets.size(); i++) {
    gsl_vector_view v = data->viewForDataset(i, dat);
    double w;
    gsl_blas_ddot(&v.vector, &v.vector, &w);

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
  throw NOT_IMPLEMENTED;

  // // Now, we'll have to compute a weight for each buffer based on
  // // their number of points/magnitude
  // QVarLengthArray<double, 1024> weight(data->datasets.size());

  // double max = 0;
  // for(int i = 0; i < data->datasets.size(); i++) {
  //   data->weightsPerBuffer[i] = 1;
  //   double w = data->weightedSquareSumForDataset(i, NULL, true);

  //   weight[i] = data->datasets[i]->nbRows()/sqrt(w);
  //   if(weight[i] > max)
  //     max = weight[i];
  // }
  // for(int i = 0; i < data->datasets.size(); i++)
  //   data->weightsPerBuffer[i] = weight[i]/max;
  // updateEditors();
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


void FitDialog::setFitEngineFactory(const QString & name)
{
  FitEngineFactoryItem * fact = FitEngine::namedFactoryItem(name);
  if(fact)
    parameters.setFitEngineFactory(fact);
}

void FitDialog::engineSelected(int id)
{
  setFitEngineFactory(fitEngineSelection->itemData(id).toString());
}

void FitDialog::updateEngineSelection(FitEngineFactoryItem * fact)
{
  int idx = fitEngineSelection->findData(fact->name);
  fitEngineSelection->setCurrentIndex(idx);
}

void FitDialog::toggleSubFunctions()
{
  if(parameters.hasSubFunctions()) {
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
      s = s.arg(FitWorkspace::formatResiduals(parameters.
                                              pointResiduals[currentIndex])).
        arg(FitWorkspace::formatResiduals(parameters.
                                          overallPointResiduals)).
        arg(FitWorkspace::formatResiduals(parameters.
                                          relativeResiduals[currentIndex])).
        arg(FitWorkspace::formatResiduals(parameters.
                                          overallRelativeResiduals));
  }
  residualsDisplay->setText(s);
}

void FitDialog::setSoftOptions()
{
  QDialog dlg;
  
  QVBoxLayout * l= new QVBoxLayout(&dlg);


  // Now, we populate the grid with editors;
  CommandOptions co;
  ArgumentsWidget * fitSoftOpts = NULL;
  ArgumentsWidget * engineOpts= NULL;

  
  if(! fitEngineParameters.contains(data->engineFactory))
    fitEngineParameters[data->engineFactory] =
      data->engineFactory->engineOptions;

  if(softOptions) {
    co = parameters.currentSoftOptions();
    l->addWidget(new QLabel("<b>Fit options:<b>"));
    
    fitSoftOpts = new ArgumentsWidget(*softOptions, false);
    l->addWidget(fitSoftOpts);
    fitSoftOpts->setFromOptions(co);
  }


  CommandOptions * engineOptionValues =
    parameters.fitEngineParameters(data->engineFactory);
  ArgumentList * engineOptions = fitEngineParameters[data->engineFactory];
  
  if(engineOptions && engineOptions->size() > 0) {
    l->addWidget(new QLabel("<b>Fit engine options:<b>"));
    engineOpts = new ArgumentsWidget(*engineOptions, false);
    l->addWidget(engineOpts);
    engineOpts->setFromOptions(*engineOptionValues);
  }

  QHBoxLayout * bot = new QHBoxLayout;
  
  QPushButton * bt = new QPushButton(tr("OK"));
  dlg.connect(bt, SIGNAL(clicked()), SLOT(accept()));
  bot->addWidget(bt);

  bt = new QPushButton(tr("Cancel"));
  dlg.connect(bt, SIGNAL(clicked()), SLOT(reject()));
  bot->addWidget(bt);

  l->addLayout(bot);


  if(dlg.exec()) {
    if(fitSoftOpts) {
      fitSoftOpts->setOptions(co);
      parameters.processSoftOptions(co);
      for(auto i = co.begin(); i != co.end(); i++)
        delete i.value();
    }

    if(engineOpts) {
      engineOpts->setOptions(*engineOptionValues);
    }
  }
  updateFitName();
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

    const GraphicsSettings & gs = soas().graphicsSettings();

    dat->pen = gs.dataSetPen(0);
    fit->pen = gs.getPen(GraphicsSettings::FitsPen);

    dlg.view(i)->addItem(dat);
    dlg.view(i)->addItem(fit);

    diff->pen = gs.getPen(GraphicsSettings::FitsPen);
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
#ifdef Q_OS_MAC
  // Dialog has to be modal on mac, as of now
  ParametersViewer dlg(&parameters);
  dlg.exec();
#else
  if(! parametersViewer)
    parametersViewer = new ParametersViewer(&parameters);
  parametersViewer->show();
#endif
}

void FitDialog::showAllParameters()
{
  for(int i = 0; i < editors.size(); i++)
    editors[i]->show();
}

void FitDialog::hideFixedParameters()
{
  for(int i = 0; i < editors.size(); i++) {
    if(editors[i]->isFixed())
      editors[i]->hide();
  }
}

void FitDialog::parametersSpreadsheet()
{
  ParametersSpreadsheet dlg(&parameters);
  dlg.exec();
  if(dlg.dataChanged())
    updateEditors();
}

void FitDialog::showHelp()
{
  QString tgt = QString("doc/qsoas.html#cmd-fit-%1").
    arg(parameters.fitName(false));
  // QTextStream o(stdout);
  // o << "T: " << tgt << endl;
  HelpBrowser::browseLocation(tgt);
}


void FitDialog::browseTrajectories()
{
  FitTrajectoryDisplay::browseTrajectories();
}

QPointer<FitDialog> * FitDialog::theDialog = NULL;


FitDialog * FitDialog::currentDialog()
{
  if(theDialog)
    return (*theDialog);
  return NULL;
}
