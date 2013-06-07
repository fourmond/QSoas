/*
  fittrajectorydisplay.cc: display of fit trajectories
  Copyright 2012 by Vincent Fourmond

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


FitTrajectoryDisplay::FitTrajectoryDisplay(FitDialog * dlg, FitData * data, 
                                           QList<FitTrajectory> * tr) :
  fitDialog(dlg), fitData(data), trajectories(tr)
{
  setupFrame();

  //  update(); // called systematically anyway

}

/// @todo Several things to do here:
/// @li sort trajectories according to final residuals
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
  }

  parametersDisplay->resizeColumnsToContents();
}

FitTrajectoryDisplay::~FitTrajectoryDisplay()
{
  
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
