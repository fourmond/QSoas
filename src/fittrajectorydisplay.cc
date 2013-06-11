/*
  fittrajectorydisplay.cc: display of fit trajectories
  Copyright 2013 by Vincent Fourmond

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
#include <fittrajectorydisplay.hh>
#include <fitdialog.hh>
#include <fit.hh>
#include <fitdata.hh>
#include <actioncombo.hh>


void ParameterRangeEditor::showEditors(bool show)
{
  lowLabel->setVisible(show);
  lowerRange->setVisible(show);
  upLabel->setVisible(show);
  upperRange->setVisible(show);
  isLog->setVisible(show);
}


void ParameterRangeEditor::insertIntoGrid(QGridLayout * grid, int row, 
                                          int baseColumn)
{
  grid->addWidget(variable, row, baseColumn++);
  grid->addWidget(lowLabel, row, baseColumn++);
  grid->addWidget(lowerRange, row, baseColumn++);
  grid->addWidget(upLabel, row, baseColumn++);
  grid->addWidget(upperRange, row, baseColumn++);
  grid->addWidget(isLog, row, baseColumn++);
}

ParameterRangeEditor::ParameterRangeEditor(const QString & name, int idx, 
                                           int ds, 
                                           bool fixed, double val) : 
  index(idx), dataset(ds)
{
    variable = new QCheckBox(name);
    lowLabel = new QLabel("Lower: ");
    upLabel = new QLabel("Upper: ");

    QString a = QString::number(val * 0.1);
    
    lowerRange = new QLineEdit(a);
    a = QString::number(val * 10);
    upperRange = new QLineEdit(a);

    isLog = new QCheckBox("log ?");
    
    connect(variable, SIGNAL(toggled(bool)),
            SLOT(variableChanged()));

    variable->setChecked(! fixed);
    variableChanged();
};

ParameterRangeEditor::~ParameterRangeEditor() {
  delete variable;
  delete lowLabel;
  delete lowerRange;
  delete upLabel;
  delete upperRange;
  delete isLog;
}

void ParameterRangeEditor::variableChanged()
{
  showEditors(isVariable());
}

double ParameterRangeEditor::value(double alpha)
{
  double min = lowerRange->text().toDouble();
  double max = upperRange->text().toDouble();
  if(isLog->isChecked()) {
    min = log(min);
    max = log(max);
  }
  double val = min + (max - min) * alpha;
  if(isLog->isChecked())
    return exp(val);
  return val;
}

//////////////////////////////////////////////////////////////////////

FitTrajectoryDisplay::FitTrajectoryDisplay(FitDialog * dlg, FitData * data, 
                                           QList<FitTrajectory> * tr) :
  fitDialog(dlg), fitData(data), trajectories(tr),
  currentIteration(-1), lastIteration(-1)
{
  setupFrame();

  //  update(); // called systematically anyway

}

/// @todo Several things to do here:
/// @li cluster the trajectories according to whether the parameters
/// "match" (i.e. whether they are within each other's errors)
/// @li plot convergence regions ?
void FitTrajectoryDisplay::setupFrame()
{
  QVBoxLayout * l = new QVBoxLayout(this);
  parametersDisplay = new QTableWidget();
  l->addWidget(parametersDisplay);
  parametersDisplay->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(parametersDisplay, 
          SIGNAL(customContextMenuRequested(const QPoint&)),
          SLOT(contextMenuOnTable(const QPoint&)));
  

  heads.clear();
  heads << "status";
  for(int i = 0; i < fitData->datasets.size(); i++)
    for(int j = 0; j < fitData->parametersPerDataset(); j++)
      heads << QString("%1[%2]").arg(fitData->parameterDefinitions[j].name).
        arg(i);
  heads << "residuals" << "rel res" << "engine"; 
  parametersDisplay->setColumnCount(heads.size());
  parametersDisplay->setHorizontalHeaderLabels(heads);


  QHBoxLayout * hb = new QHBoxLayout();

  QPushButton * bt = new QPushButton(tr("Export"));
  connect(bt, SIGNAL(clicked()), SLOT(exportToFile()));
  hb->addWidget(bt);

  bt = new QPushButton(tr("Sort"));
  connect(bt, SIGNAL(clicked()), SLOT(sortByResiduals()));
  hb->addWidget(bt);

  bt = new QPushButton(tr("Close"));
  connect(bt, SIGNAL(clicked()), SLOT(close()));
  hb->addWidget(bt);

  l->addLayout(hb);
  setupExploration();

  l->addLayout(explorationLayout);

  connect(fitDialog, SIGNAL(finishedFitting()), 
          SLOT(update()), Qt::QueuedConnection);

}

/// @todo Should join FitTrajectory...
static QString describe(FitTrajectory::Ending e)
{
  switch(e) {
  case FitTrajectory::Converged:
    return "ok";
  case FitTrajectory::Cancelled:
    return "(cancelled)";
  case FitTrajectory::TimeOut:
    return "(time out)";
  case FitTrajectory::Error:
    return "(fail)";
  }
  return "ARGH!";
}


void FitTrajectoryDisplay::update()
{
  
  parametersDisplay->setRowCount(trajectories->size() * 2);
  
  for(int i = 0; i < trajectories->size(); i++) {
    const FitTrajectory & tr = trajectories->value(i);
    parametersDisplay->setItem(i*2, 0, new QTableWidgetItem("(init)"));

    for(int j = 0; j < fitData->fullParameterNumber(); j++)
      parametersDisplay->
        setItem(i*2, 1+j, 
                new QTableWidgetItem(QString::number(tr.initialParameters[j])));

    parametersDisplay->
      setItem(i*2 + 1, 0, new QTableWidgetItem(describe(tr.ending)));
              
    for(int j = 0; j < fitData->fullParameterNumber(); j++) {
      QTableWidgetItem * it = 
        new QTableWidgetItem(QString::number(tr.finalParameters[j]));
      it->setToolTip(QString("Error: %1 %").arg(100 * tr.parameterErrors[j] /
                                                tr.finalParameters[j]));
      parametersDisplay->setItem(i*2 + 1, 1+j, it);
    }

    parametersDisplay->
      setItem(i*2 + 1, fitData->fullParameterNumber() + 1, 
              new QTableWidgetItem(QString::number(tr.residuals)));
    parametersDisplay->
      setItem(i*2 + 1, fitData->fullParameterNumber() + 2, 
              new QTableWidgetItem(QString::number(tr.relativeResiduals)));
    parametersDisplay->
      setItem(i*2 + 1, fitData->fullParameterNumber() + 3, 
              new QTableWidgetItem(tr.engine));

    // Now select a color for the bottom line
    QColor c;
    switch(tr.ending) {
    case FitTrajectory::Converged:
      c = QColor::fromHsv(120, 15, 255);
      break;
    case FitTrajectory::Cancelled:
      c = QColor::fromHsv(270, 15, 255);
      break;
    case FitTrajectory::TimeOut:
    default:
      c = QColor::fromHsv(359, 15, 255);
      break;
    }
    for(int j = 0; j <= fitData->fullParameterNumber() + 3; j++)
      parametersDisplay->item(i*2+1, j)->setBackground(c);
  }

  parametersDisplay->resizeColumnsToContents();
}

FitTrajectoryDisplay::~FitTrajectoryDisplay()
{
  for(int i = 0; i < parameterRangeEditors.size(); i++)
    delete parameterRangeEditors[i];
}

void FitTrajectoryDisplay::setupExploration()
{
  explorationLayout = new QGridLayout();

  int rows = 0;
  
  int nbparams = fitData->parametersPerDataset();
  int nbds = fitData->datasets.size();

  if(nbds > 1) {
    // First pass on global parameters
    for(int i = 0; i < nbparams; i++) {
      if(fitDialog->parameters.isGlobal(i)) {
        ParameterRangeEditor * ed = new 
          ParameterRangeEditor(fitData->parameterDefinitions[i].name,
                               i, -1, 
                               fitDialog->parameters.isFixed(i, 0),
                               fitDialog->parameters.getValue(i, 0));

        parameterRangeEditors << ed;
        ed->insertIntoGrid(explorationLayout, rows++);
      }
    }
  }

  for(int j = 0; j < nbds; j++) {
    for(int i = 0; i < nbparams; i++) {
      if(! fitDialog->parameters.isGlobal(i)) {
        ParameterRangeEditor * ed = new 
          ParameterRangeEditor(fitData->parameterDefinitions[i].name,
                               i, j, 
                               fitDialog->parameters.isFixed(i, j),
                               fitDialog->parameters.getValue(i, j));
        parameterRangeEditors << ed;
        ed->insertIntoGrid(explorationLayout, rows++);
      }
    }
  }

  explorationLayout->addWidget(new QLabel("Repetitions"), 
                               rows, 0, 1, 2);
  repetitions = new QLineEdit("10");

  explorationLayout->addWidget(repetitions, rows, 2, 1, 2);

  rows++;

  iterationDisplay = new QLabel();
  explorationLayout->addWidget(iterationDisplay, rows, 0, 1, 5);

  startStopButton = new QPushButton("Go !");
  explorationLayout->addWidget(startStopButton, rows, 5);

  connect(startStopButton, SIGNAL(clicked()), SLOT(startExploration()));
  connect(fitDialog, SIGNAL(finishedFitting()), 
          SLOT(sendNextParameters()), Qt::QueuedConnection);
  
}


void FitTrajectoryDisplay::exportToFile()
{
  QString save = QFileDialog::getSaveFileName(this, tr("Export data"));
  if(save.isEmpty())
    return;
  
  QFile f(save);
  if(! f.open(QIODevice::WriteOnly))
    return;

  QTextStream o(&f);

  o << heads.join("\t") << endl;
  for(int i = 0; i < trajectories->size(); i++) {
    QStringList lst;
    const FitTrajectory & tr = trajectories->value(i);

    lst << "(init)";

    for(int j = 0; j < fitData->fullParameterNumber(); j++)
      lst << QString::number(tr.initialParameters[j]);
    o << lst.join("\t") << endl;
    
    lst.clear();

    lst << describe(tr.ending);
              
    for(int j = 0; j < fitData->fullParameterNumber(); j++)
      lst << QString::number(tr.finalParameters[j]);

    lst << QString::number(tr.residuals) 
        << QString::number(tr.relativeResiduals);

    o << lst.join("\t") << endl;
  }

}

void FitTrajectoryDisplay::contextMenuOnTable(const QPoint & pos)
{
  QMenu menu;
  QAction * a = new QAction(tr("Reuse parameters"), this);
  menu.addAction(a);
  QAction * got = menu.exec(parametersDisplay->mapToGlobal(pos));
  if(got == a) {
    // Send back the given parameters to the fitdialog
    int idx = parametersDisplay->currentRow();
    if(idx < 0)
      return;
    Vector parameters;
    if(idx % 2)
      parameters = (*trajectories)[idx/2].finalParameters;
    else
      parameters = (*trajectories)[idx/2].initialParameters;
    fitDialog->setParameterValues(parameters);
  }
}

void FitTrajectoryDisplay::sortByResiduals()
{
  qSort(*trajectories);
  update();
}


void FitTrajectoryDisplay::stopExploration()
{
  startStopButton->setText("Go");

  lastIteration = currentIteration = -1;
  iterationDisplay->setText("");
}


void FitTrajectoryDisplay::startStopExploration()
{
  // Actually stop !
  if(lastIteration > 0)
    stopExploration();
  else
    startExploration();
}

void FitTrajectoryDisplay::startExploration()
{
  startStopButton->setText("Stop");

  currentIteration = 0;
  lastIteration = repetitions->text().toInt();

  sendNextParameters();
}

void FitTrajectoryDisplay::sendNextParameters()
{
  if(currentIteration < 0 || lastIteration < 0)
    return;
  if(currentIteration >= lastIteration) {
    stopExploration();
    return;
  }

  currentIteration++;
  iterationDisplay->setText(QString("Iteration %1/%2").
                            arg(currentIteration).
                            arg(lastIteration));

  for(int i = 0; i < parameterRangeEditors.size(); i++) {
    ParameterRangeEditor * ed = parameterRangeEditors[i];
    if(! ed->isVariable())
      continue;
    double x = rand() * 1.0/RAND_MAX;
    x = ed->value(x);
    fitDialog->parameters.setValue(ed->index, ed->dataset, x);
  }
  fitDialog->updateEditors();
  fitDialog->startFit();
}


