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
#include <fitparametereditor.hh>
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
#include <parametersdialog.hh>

#include <utils.hh>


static SettingsValue<QSize> fitDialogSize("fitdialog/size", QSize(700,500));


FitDialog::FitDialog(FitData * d, bool displayWeights) : 
  data(d),
  stackedViews(NULL), 
  parameters(d),
  currentIndex(0),
  settingEditors(false), 
  progressReport(NULL)
{
  resize(fitDialogSize);

  compute();
  if(displayWeights && d->datasets.size() > 1)
    bufferWeightEditor = new QLineEdit;
  else
    bufferWeightEditor = NULL;
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

  //////////////////////////////////////////////////////////////////////
  // First line
  QHBoxLayout * hb = new QHBoxLayout;
  QPushButton * bt = new QPushButton(tr("<-"));
  connect(bt, SIGNAL(clicked()), SLOT(previousDataset()));
  hb->addWidget(bt);

  bt = new QPushButton(tr("->"));
  connect(bt, SIGNAL(clicked()), SLOT(nextDataset()));
  hb->addWidget(bt);

  hb->addWidget(new QLabel("<b>Fit:</b> " + data->fit->fitName()), 1);

  hb->addWidget(bufferSelection);
  connect(bufferSelection, SIGNAL(currentIndexChanged(int)),
          SLOT(dataSetChanged(int)));
  
  bufferNumber = new QLabel();
  hb->addWidget(bufferNumber);

  if(bufferWeightEditor) {
    hb->addWidget(bufferWeightEditor);
    connect(bufferWeightEditor, SIGNAL(textEdited(const QString &)),
            SLOT(weightEdited(const QString &)));
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


  int nbParams = data->parameterDefinitions.size();
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
  // Bottom

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
  ac->addAction("Load from file", this, 
                SLOT(loadParameters()),
                QKeySequence(tr("Ctrl+L")));
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
  ac->addAction("Show covariance matrix", this, 
                SLOT(showCovarianceMatrix()),
                QKeySequence(tr("Ctrl+M")));
  hb->addWidget(ac);

  ac = new ActionCombo(tr("Print..."));
  ac->addAction("Save all as PDF", this, SLOT(saveAllPDF()));


  hb->addWidget(ac);
  hb->addStretch(1);

  bt = new QPushButton(tr("Update curves (Ctrl+U)"));
  connect(bt, SIGNAL(clicked()), SLOT(compute()));


  hb->addWidget(bt);

  bt = new QPushButton(tr("Edit parameters"));
  connect(bt, SIGNAL(clicked()), SLOT(editParameters()));
  hb->addWidget(bt);

  startButton = new QPushButton(tr("Fit (Ctrl+F)"));
  connect(startButton, SIGNAL(clicked()), SLOT(startFit()));
  startButton->setDefault(true);
  hb->addWidget(startButton);

  cancelButton = new QPushButton(tr("Abort (Ctrl+A)"));
  connect(cancelButton, SIGNAL(clicked()), SLOT(cancelFit()));
  hb->addWidget(cancelButton);
  cancelButton->setVisible(false);


  bt = new QPushButton(tr("Close (Ctrl+C)"));
  connect(bt, SIGNAL(clicked()), SLOT(close()));
  hb->addWidget(bt);


  layout->addLayout(hb);

  // Registering shortcuts:
  Utils::registerShortCut(QKeySequence(tr("Ctrl+U")), 
                          this, SLOT(compute()));
  Utils::registerShortCut(QKeySequence(tr("Ctrl+C")), 
                          this, SLOT(close()));
  Utils::registerShortCut(QKeySequence(tr("Ctrl+F")), 
                          this, SLOT(startFit()));
  Utils::registerShortCut(QKeySequence(tr("Ctrl+A")), 
                          this, SLOT(cancelFit()));

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
}


void FitDialog::compute()
{
  try {
    parameters.recompute();
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


void FitDialog::updateEditors()
{
  settingEditors = true;
  int sz = editors.size();
  for(int i = 0; i < sz; i++)
    editors[i]->updateFromParameters();

  if(bufferWeightEditor)
    bufferWeightEditor->
      setText(QString::number(data->weightsPerBuffer[currentIndex]));

  settingEditors = false;
}


void FitDialog::startFit()
{
  QTime timer;
  timer.start();

  try {
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
      double relres = data->relativeResiduals();
      QString str = QString("Iteration #%1, residuals: %2, "
                            "relative residuals: %3").
        arg(it).arg(residuals).arg(relres);
      Terminal::out << str << endl;

      progressReport->setText(str);
      parameters.retrieveParameters();
      updateEditors();

      QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
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
      else
        mention = " <font color=#080>(done)</font>";
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
  }
  Terminal::out << "Fitting took an overall " << timer.elapsed() * 1e-3
                << " seconds" << endl;

  parameters.writeToTerminal();
  compute();
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
  ds->name = base->cleanedName() + "_fit_" + data->fit->fitName(false) + ".dat";
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
  else 
    loadParametersFile(load);
}

void FitDialog::loadParametersFile(const QString & file)
{
  QFile f(file);
  if(! f.open(QIODevice::ReadOnly))
    return;                     /// @todo Signal !
  QString msg;
  try {
    parameters.loadParameters(&f);
    updateEditors();
    compute();
    msg = QString("Loaded fit parameters from file %1").arg(file);
  }
  catch (const Exception & e) {
    msg = QString("Could not load parameters from '%1':").
      arg(file) + e.message();
  }
  Terminal::out << msg;
  progressReport->setText(msg);
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
  QPrinter p;
  p.setOrientation(QPrinter::Landscape);
  p.setOutputFileName("fits.pdf");
  CurveView::nupPrint(&p, views, 3, 2);
}

void FitDialog::showCovarianceMatrix()
{
  QDialog dlg;//  = new QDialog();
  // dlg->setAttribute(Qt::WA_DeleteOnClose);

  QVBoxLayout * layout = new QVBoxLayout(&dlg);
  QTableWidget * tw = new QTableWidget();
  parameters.setupWithCovarianceMatrix(tw);
  layout->addWidget(tw);

  dlg.exec();

  /// @todo Add possibility to save the covariance matrix + normalize
  /// each row and column by the value of the parameter ?
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

