/*
  fittrajectorydisplay.cc: display of fit trajectories
  Copyright 2013, 2014, 2015, 2016, 2017, 2018 by CNRS/AMU

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
#include <fitworkspace.hh>
#include <fit.hh>
#include <fitdata.hh>
#include <actioncombo.hh>

#include <terminal.hh>
#include <utils.hh>
#include <flowinggridlayout.hh>
#include <curveview.hh>
#include <tuneabledatadisplay.hh>
#include <xyiterable.hh>
#include <curvepoints.hh>

#include <graphicssettings.hh>
#include <soas.hh>

#include <debug.hh>

class TrajectoriesModel : public QAbstractTableModel {
  /// The list of trajectories
  FitTrajectories * trajectories;

  /// The underlying fit data...
  const FitData * fitData;


  class Item  {
  public:
    QString name;

    std::function<QVariant (const FitTrajectory* trj,
                            int role, bool final)> fcn;
    Item(const QString & n,     std::function<QVariant (const FitTrajectory* trj, int role, bool final)> f) :
      name(n), fcn(f) {;};
  };

  QList<Item> prefix;
  QList<Item> suffix;

public:

  void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) {
    Debug::debug() << "sort by " << column << " -- " << order << endl;

    qSort(trajectories->begin(), trajectories->end(), [this, order, column](FitTrajectory a, FitTrajectory b) -> bool {
        QVariant va = data(&a, column, Qt::DisplayRole, true);
        QVariant vb = data(&b, column, Qt::DisplayRole, true);
        if(va.type() == QMetaType::QDateTime) {
          QDateTime da = va.toDateTime();
          QDateTime db = vb.toDateTime();
          return (order == Qt::AscendingOrder ?
                  da < db : db < da);
        }
        if(va.type() == QMetaType::Double) {
          double da = va.toDouble();
          double db = vb.toDouble();
          return (order == Qt::AscendingOrder ?
                  da < db : db < da);
        }
        if(va.type() == QMetaType::Int || va.type() == QMetaType::UInt ||
           va.type() == QMetaType::Long || va.type() == QMetaType::ULong) {
          qint64 da = va.toLongLong();
          qint64 db = vb.toLongLong();
          return (order == Qt::AscendingOrder ?
                  da < db : db < da);
        }
        QString da = va.toString();
        QString db = vb.toString();
        return (order == Qt::AscendingOrder ?
                da < db : db < da);
      });
    update();
  };
  
  int col(const QString & n) {
    for(int i = 0; i < prefix.size(); i++)
      if(prefix[i].name == n)
        return i;
    int nb = fitData->fullParameterNumber();
    for(int i = 0; i < suffix.size(); i++)
      if(suffix[i].name == n)
        return nb+prefix.size()+i;
    return -1;
  };
    

  TrajectoriesModel(FitTrajectories * trj, const FitData * d) :
    trajectories(trj), fitData(d) {
    prefix << Item("status", [](const FitTrajectory* trj,
                                int role, bool final) -> QVariant {
                     if(role == Qt::DisplayRole && final) {
                       return FitTrajectory::endingName(trj->ending);
                     }
                     return QVariant();
                   })
           << Item("residuals", [](const FitTrajectory* trj,
                                   int role, bool final) -> QVariant {
                     if(role == Qt::DisplayRole && final) {
                       return trj->residuals;
                     }
                     return QVariant();
                   })
           << Item("delta", [](const FitTrajectory* trj,
                                   int role, bool final) -> QVariant {
                     if(role == Qt::DisplayRole && final) {
                       return trj->residualsDelta;
                     }
                     return QVariant();
                   })
      ;

    suffix << Item("rel res", [](const FitTrajectory* trj,
                                 int role, bool final) -> QVariant {
                     if(role == Qt::DisplayRole && final) {
                       return trj->relativeResiduals;
                     }
                     return QVariant();
                   })
           << Item("intern res", [](const FitTrajectory* trj,
                                    int role, bool final) -> QVariant {
                     if(role == Qt::DisplayRole && final) {
                       return trj->internalResiduals;
                     }
                     return QVariant();
                   })
           << Item("engine", [](const FitTrajectory* trj,
                                int role, bool final) -> QVariant {
                     if(role == Qt::DisplayRole && final) {
                       return trj->engine;
                     }
                     return QVariant();
                   })
           << Item("date", [](const FitTrajectory* trj,
                              int role, bool final) -> QVariant {
                     if(role == Qt::DisplayRole) {
                       return final ? trj->endTime : trj->startTime;
                     }
                     return QVariant();
                   })
           << Item("duration", [](const FitTrajectory* trj,
                              int role, bool final) -> QVariant {
                     if(role == Qt::DisplayRole && final) {
                       return trj->startTime.msecsTo(trj->endTime)*1e-3;
                     }
                     return QVariant();
                   })
           << Item("iterations", [](const FitTrajectory* trj,
                                   int role, bool final) -> QVariant {
                     if(role == Qt::DisplayRole && final) {
                       return trj->iterations;
                     }
                     return QVariant();
                   })
           << Item("evaluations", [](const FitTrajectory* trj,
                                   int role, bool final) -> QVariant {
                     if(role == Qt::DisplayRole && final) {
                       return trj->evaluations;
                     }
                     return QVariant();
                   })

      ;

  };

  QVariant data(const FitTrajectory* trj, int col, int role, bool final) const {
    int nb = fitData->fullParameterNumber();
    if(role == Qt::BackgroundRole) {
      QColor c;
      if(final) {
        // Now select a color for the bottom line
        switch(trj->ending) {
        case FitWorkspace::Converged:
          c = QColor::fromHsv(120, 15, 255);
          break;
        case FitWorkspace::Cancelled:
          c = QColor::fromHsv(270, 15, 255);
          break;
        case FitWorkspace::TimeOut:
        default:
          c = QColor::fromHsv(359, 15, 255);
          break;
        }
      }
      else {
        col -= prefix.size();
        if(col >= 0 && col < nb) {
          if(trj->fixed[col]) 
            c = QColor(210,210,210);
        }
      }
      if(c.isValid())
        return QBrush(c);
      return QVariant();
    }  
    
    if(col < prefix.size()) {
      return prefix[col].fcn(trj, role, final);
    }
    col -= prefix.size();
    if(col < nb) {
      if(role == Qt::DisplayRole || role == Qt::EditRole) {
        double val = final ? trj->finalParameters[col] :
          trj->initialParameters[col];
        return QVariant(val);
      }
      if(role == Qt::ToolTipRole && final) {
        return QString("Error: %1 %").arg(100 * trj->parameterErrors[col]);
      }
      return QVariant();
    }
    col -= nb;
    return suffix[col].fcn(trj, role, final);
  };

  
  /// @name Reimplemented interface
  ///
  /// @{
  virtual int rowCount(const QModelIndex & parent = QModelIndex()) const {
    return trajectories->size()*2;
  }

  virtual int columnCount(const QModelIndex & parent = QModelIndex()) const {
    return prefix.size() + fitData->fullParameterNumber() + suffix.size();
  };

  virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const {
    int col = index.column();
    const FitTrajectory * trj = &((*trajectories)[index.row()/2]);
    bool final = index.row() % 2;
    return data(trj, col, role, final);
  };

  virtual QVariant headerData(int section, Qt::Orientation orientation,
                              int role) const {
    if(orientation == Qt::Horizontal) {
      if(section < prefix.size()) {
        if(role == Qt::DisplayRole)
          return prefix[section].name;
        return QVariant();
      }
      section -= prefix.size();
      int nb = fitData->fullParameterNumber();
      if(section  < nb) {
        if(role == Qt::DisplayRole) {
          int k = fitData->parametersPerDataset();
          int j = section % k;
          int i = section / k;
          return QString("%1[%2]").arg(fitData->parameterDefinitions[j].name).
            arg(i);
        }
        return QVariant();
      }
      section-= nb;
      if(section < suffix.size()) {
        if(role == Qt::DisplayRole)
          return suffix[section].name;
      }
      return QVariant();
    }
    if(orientation == Qt::Vertical) {
      if(role == Qt::DisplayRole) {
        return QString("%1 %2").arg(section/2+1).
          arg(section % 2 ? "final" : "init");
      }
    }
    return QVariant();
  }
  /// @}

  void update() {
    emit(layoutChanged());
  };
};

//////////////////////////////////////////////////////////////////////

class XYITrajectory : public XYIterable {
  int index;
  const FitTrajectories * trajectories;
  QString name;

  int nbds;
  int nbparams;

  int curds;
  int curtrj;

public:

  XYITrajectory(const FitTrajectories * trjs,
                int idx,
                const FitData * d) :
    index(idx), trajectories(trjs), curds(0), curtrj(0)
  {
    name = d->parameterDefinitions[index].name;
    nbds = d->datasets.size();
    nbparams = d->parametersPerDataset();
  };

  virtual void reset() override {
    curds = 0;
    curtrj = 0;
  };
  
  virtual QString dataName() const override {
    return name;
  };

  virtual bool next(double * x, double * y) override {
    if(curds < nbds && curtrj < trajectories->size()) {
      *x = (*trajectories)[curtrj].residuals;
      *y = (*trajectories)[curtrj].finalParameters[curds * nbparams + index];

      curds++;
      if(curds >= nbds) {
        curds = 0;
        curtrj++;
      }
      return true;
    }
    return false;
  }
  
  virtual bool nextWithErrors(double * x, double * y, double * err) override {
    if(curds < nbds && curtrj < trajectories->size()) {
      *x = (*trajectories)[curtrj].residuals;
      *y = (*trajectories)[curtrj].finalParameters[curds * nbparams + index];
      *err = (*trajectories)[curtrj].parameterErrors[curds * nbparams + index];

      curds++;
      if(curds >= nbds) {
        curds = 0;
        curtrj++;
      }
      return true;
    }
    return false;
  }

};


//////////////////////////////////////////////////////////////////////

FitTrajectoryDisplay::FitTrajectoryDisplay(FitWorkspace * ws) :
  workspace(ws), fitData(ws->data())
{
  setupFrame();
  // Save from initial
}

/// @todo Several things to do here:
/// @li cluster the trajectories according to whether the parameters
/// "match" (i.e. whether they are within each other's errors)
/// @li plot convergence regions ?
void FitTrajectoryDisplay::setupFrame()
{
  QVBoxLayout * t = new QVBoxLayout(this);
  overallLayout = t;            // t is just a handy shortcut

  //////////////////////////////
  // Setup of the tab widget
  tabs = new QTabWidget;
  t->addWidget(tabs);


  //////////////////////////////
  // Tab1: table view + parameter ranges
  parametersDisplay = new QTableView();

  QWidget * wdgt = new QWidget;
  QSplitter * splt = new QSplitter(Qt::Vertical);
  t->addWidget(splt);
  QVBoxLayout * l = new QVBoxLayout(wdgt);


  splt->addWidget(parametersDisplay);
  splt->addWidget(wdgt);
  parametersDisplay->setContextMenuPolicy(Qt::CustomContextMenu);

  model = new TrajectoriesModel(&workspace->trajectories, fitData);
  parametersDisplay->setModel(model);

  parametersDisplay->setSortingEnabled(true);
  parametersDisplay->horizontalHeader()->
    setSortIndicator(model->col("date"), Qt::AscendingOrder);

  connect(parametersDisplay, 
          SIGNAL(customContextMenuRequested(const QPoint&)),
          SLOT(contextMenuOnTable(const QPoint&)));
  

  QHBoxLayout * hb = new QHBoxLayout();

  QPushButton * bt = new QPushButton(tr("Export"));
  connect(bt, SIGNAL(clicked()), SLOT(exportToFile()));
  hb->addWidget(bt);

  bt = new QPushButton(tr("Import"));
  connect(bt, SIGNAL(clicked()), SLOT(importFromFile()));
  hb->addWidget(bt);

  bt = new QPushButton(tr("Sort"));
  connect(bt, SIGNAL(clicked()), SLOT(sortByResiduals()));
  hb->addWidget(bt);
  
  bt = new QPushButton(tr("Trim"));
  connect(bt, SIGNAL(clicked()), SLOT(trim()));
  hb->addWidget(bt);

  bt = new QPushButton(tr("Cluster"));
  connect(bt, SIGNAL(clicked()), SLOT(clusterTrajectories()));
  hb->addWidget(bt);

  l->addLayout(hb);

  // explorationControls = new QScrollArea;
  // l->addWidget(explorationControls);



  tabs->addTab(splt, "Table");

  //////////////////////////////
  // Setup of curve view
  
  QWidget * viewtab = new QWidget;
  QVBoxLayout * vl = new QVBoxLayout(viewtab);
  view = new CurveView;
  vl->addWidget(view, 1);

  FlowingGridLayout * ml = new FlowingGridLayout;
  vl->addLayout(ml);        // I prefer to add afterwards, but it

  int nbparams = fitData->parametersPerDataset();
  for(int i = 0; i < nbparams; i++) {
    QColor color = soas().graphicsSettings().dataSetPen(i).color();
    TuneableDataDisplay * tdd =
      new TuneableDataDisplay(fitData->parameterDefinitions[i].name,
                              view, i != nbparams-1, color);
    
    CurvePoints * cds =
      tdd->addSource(new XYITrajectory(&workspace->trajectories, i, fitData));
    cds->relativeErrorBar = true;
    cds->pen = color;
    cds->brush = color;
    cds->size = 5;              /// @todo make that customizable
    cds->countBB = true;

    ml->addWidget(tdd);
  }

  tabs->addTab(viewtab, "Display");


  //////////////////////////////

  // setupExploration();

  connect(workspace, SIGNAL(finishedFitting(int)), 
          SLOT(update()), Qt::QueuedConnection);

  connect(workspace, SIGNAL(finishedFitting(int)), 
          view, SLOT(repaint()), Qt::QueuedConnection);

}

void FitTrajectoryDisplay::update()
{
  
  // parametersDisplay->setRowCount(trajectories->size() * 2);
  model->update();
  parametersDisplay->resizeColumnsToContents();
}

FitTrajectoryDisplay::~FitTrajectoryDisplay()
{
}



void FitTrajectoryDisplay::contextMenuOnTable(const QPoint & pos)
{
  QMenu menu;
  QAction * a = new QAction(tr("Reuse parameters"), this);
  connect(a, SIGNAL(triggered()), SLOT(reuseCurrentParameters()));
  menu.addAction(a);

  a = new QAction(tr("Delete trajectory"), this);
  connect(a, SIGNAL(triggered()), SLOT(deleteCurrentParameters()));
  menu.addAction(a);

  menu.exec(parametersDisplay->mapToGlobal(pos));
}

void FitTrajectoryDisplay::reuseCurrentParameters()
{
  // Send back the given parameters to the fitdialog
  int idx = parametersDisplay->currentIndex().row();
  if(idx < 0)
    return;
  Vector parameters;
  if(idx % 2)
    parameters = workspace->trajectories[idx/2].finalParameters;
  else
    parameters = workspace->trajectories[idx/2].initialParameters;
  workspace->restoreParameterValues(parameters);
}

void FitTrajectoryDisplay::deleteCurrentParameters()
{
  // Send back the given parameters to the fitdialog
  int idx = parametersDisplay->currentIndex().row();
  if(idx < 0)
    return;
  workspace->trajectories.remove(idx/2);
  model->update();
}

void FitTrajectoryDisplay::sortByResiduals()
{
  throw NOT_IMPLEMENTED;
  // qSort(*trajectories);
  // update();
}



void FitTrajectoryDisplay::clusterTrajectories()
{
  throw NOT_IMPLEMENTED;
  // QList<FitTrajectoryCluster> clusters = 
  //   FitTrajectoryCluster::clusterTrajectories(trajectories);

  // Terminal::out << "Found " << clusters.size() << " clusters" << endl;
  // for(int i = 0; i < clusters.size(); i++)
  //   Terminal::out  << "Cluster #" << i << ":\n" 
  //                  << clusters[i].dump() << endl;
}

void FitTrajectoryDisplay::exportToFile()
{
  QString save = QFileDialog::getSaveFileName(this, tr("Export data"));
  if(save.isEmpty())
    return;
  
  QFile f(save);
  if(! f.open(QIODevice::WriteOnly))
    return;

  Terminal::out << "Saving fit trajectories data to " << save << endl;

  QTextStream o(&f);
  throw NOT_IMPLEMENTED;
  // FitTrajectory::exportToFile(*trajectories, fitData, o);
}

void FitTrajectoryDisplay::importFromFile()
{
  QString ld = QFileDialog::getOpenFileName(this, tr("Import data"));
  importFromFile(ld);
}

void FitTrajectoryDisplay::importFromFile(const QString & file)
{
  QFile fl(file);
  Utils::open(&fl, QIODevice::ReadOnly);

  int nb;
  throw NOT_IMPLEMENTED;
  // nb = FitTrajectory::importFromFile(trajectories, fitData, &fl);
  
  Terminal::out << "Imported " << nb << " trajectories from "
                << file << endl;
  update();
}


void FitTrajectoryDisplay::trim(double threshold)
{
  int nb = workspace->trajectories.trim(threshold);
  Terminal::out << "Trimming at threshold " << threshold
                << ", removed " << nb
                << "trajectories" << endl;
  model->update();
}

void FitTrajectoryDisplay::trim()
{
  bool ok = false;
  double thr =
    QInputDialog::getDouble(this, "Trim threshold",
                            "Enter trim threshold",
                            2, 0, 1e5, 3, &ok);
  if(ok)
    trim(thr);
}
