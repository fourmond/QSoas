/*
  fittrajectorydisplay.cc: display of fit trajectories
  Copyright 2013, 2014, 2015, 2016, 2017, 2018, 2023 by CNRS/AMU

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
#include <file.hh>

#include <synchronizedtables.hh>

/// A model suitable to display the trajectories, displaying both the
/// beginning and ending position.
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

    /// Number of the dataset
    int dataset;

    /// Index in the parameters vector
    int index;

    /// The parameter index -- not unique, unlike index.
    int parameterIndex;

    Item(const QString & n,
         std::function<QVariant (const FitTrajectory* trj,
                                 int role, bool fnal)> f,
         int ds = -1, int idx = -1, int pidx = - 1) :
      name(n), fcn(f), dataset(ds), index(idx), parameterIndex(pidx) {;};
  };

  QList<Item> columns;

  /// Whether or not to color via flags
  bool flagColor;

  mutable QHash<QString, QColor> flagColors;
  
public:
  
  QColor getFlagColor(const QString & flag) const {
    if(! flagColors.contains(flag)) {
      int idx = flagColors.size();
      flagColors[flag] = QColor::fromHsv((idx * 111)%360, 50, 150 +
                                         ((idx+1) % 2)*100);
    }
    return flagColors[flag];
  };



  /// A reference trajectory to color stuff
  const FitTrajectory * referenceTrajectory;

  void setFlagColor(bool b) {
    flagColor = b;
    update();
  };

  void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) {
    Debug::debug() << "sort by " << column << " -- " << order << endl;

    std::sort(trajectories->begin(), trajectories->end(),
              [this, order, column](FitTrajectory a, FitTrajectory b) -> bool {
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
                  // We put the nans always at the end regardless of the order.
                  if(std::isnan(da))
                    return false;
                  if(std::isnan(db))
                    return true;
                  return (order == Qt::AscendingOrder ?
                          da < db : db < da);
                }
                if(va.type() == QMetaType::Int || va.type() ==
                   QMetaType::UInt ||
                   va.type() == QMetaType::Long || va.type() ==
                   QMetaType::ULong) {
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
    for(int i = 0; i < columns.size(); i++)
      if(columns[i].name == n)
        return i;
    return -1;
  };
    

  TrajectoriesModel(FitTrajectories * trj, const FitData * d) :
    trajectories(trj), fitData(d), flagColor(false),
    referenceTrajectory(NULL) {
    columns << Item("status", [](const FitTrajectory* trj,
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
           << Item("flags", [](const FitTrajectory* trj,
                               int role, bool /*final*/) -> QVariant {
                     if(role == Qt::DisplayRole) {
                       return QStringList(trj->flags.toList()).join(",");
                     }
                     return QVariant();
                   })
      ;
    int nbds = fitData->datasets.size();
    int n = fitData->parametersPerDataset();
    QList<QPair<double, QColor> > colors;
    colors << QPair<double, QColor>(0, QColor(220,255,220))
           << QPair<double, QColor>(0.1, QColor(220,220,255))
           << QPair<double, QColor>(1, QColor(255,220,220));

    for(int i = 0; i < nbds; i++) {
      for(int j = 0; j < n; j++) {
        QString na = QString("%1[%2]").
          arg(fitData->parameterDefinitions[j].name).
          arg(i);

        int col = i*n+j;
        columns << Item(na, [this,j,i,n,col,colors](const FitTrajectory* trj,
                                             int role, bool fnl) -> QVariant {
                          if(role == Qt::DisplayRole || role == Qt::EditRole) {
                            double val = fnl ? trj->finalParameters[col] :
                              trj->initialParameters[col];
                            return QVariant(val);
                          }
                          if(role == Qt::ToolTipRole && fnl) {
                            return QString("Error: %1 %").arg(100 * trj->parameterErrors[col]);
                          }
                          if(role == Qt::BackgroundRole && !fnl) {
                            if(trj->fixed[col]) 
                              return QBrush(QColor(210,210,210));
                          }
                          if(role == Qt::BackgroundRole && fnl && referenceTrajectory) {
                            if(trj == referenceTrajectory)
                              return QBrush(QColor(200,255,200));
                            double dv = trj->finalParameters[col]/
                              referenceTrajectory->finalParameters[col];
                            if(dv < 0)
                              dv = 10;
                            else
                              dv = fabs(log10(dv));
                            return QBrush(Utils::gradientColor(dv, colors));
                            
                          }
                          return QVariant();
                        }, i, col, j);
      }
      QString na = QString("point_residuals[%1]").arg(i);
      columns << Item(na, [i](const FitTrajectory* trj,
                              int role, bool final) -> QVariant {
                        if((role == Qt::DisplayRole || role == Qt::EditRole) && final) {
                          return QVariant(trj->pointResiduals[i]);
                        }
                        if(role == Qt::BackgroundRole)
                          return QBrush(QColor(220,220,255));
                        return QVariant();
                      }, i);
      
      na = QString("weights[%1]").arg(i);
      columns << Item(na, [i](const FitTrajectory* trj,
                              int role, bool final) -> QVariant {
                            if((role == Qt::DisplayRole || role == Qt::EditRole) && final) {
                              return QVariant(trj->weights[i]);
                            }
                            if(role == Qt::BackgroundRole)
                              return QBrush(QColor(220,220,255));
                            return QVariant();
                          }, i);
      
    }


    columns << Item("rel res", [](const FitTrajectory* trj,
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
           << Item("delta", [](const FitTrajectory* trj,
                                   int role, bool final) -> QVariant {
                     if(role == Qt::DisplayRole && final) {
                       return trj->residualsDelta;
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
            << Item("pid", [](const FitTrajectory* trj,
                                      int role, bool final) -> QVariant {
                      if(role == Qt::DisplayRole) {
                        return trj->pid;
                      }
                      return QVariant();
                    })

      ;

  };

  QVariant data(const FitTrajectory* trj, int col, int role, bool fnl) const {
    QVariant v = columns[col].fcn(trj, role, fnl);
    if(! v.isValid()) {
      if(role == Qt::BackgroundRole) {
        QColor c;
        if(flagColor) {
          QStringList flg = trj->flags.toList();
          if(flg.size() == 1)
            c = getFlagColor(flg[0]);
        }
        else {
          if(fnl) {
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
        }
        if(c.isValid())
          return QBrush(c);
      }  
    }
    return v;
  };

  int dataset(int col) const {
    if(col < 0 || col >= columns.size())
      return -1;
    return columns[col].dataset;
  };

  int parameterIndex(int col) const {
    if(col < 0 || col >= columns.size())
      return -1;
    return columns[col].parameterIndex;
  };

  bool isFixed(int col, const FitTrajectory * trj) const {
    int idx = columns[col].index;
    if(idx < 0)
      return false;
    return trj->fixed[idx];
  };

  
  /// @name Reimplemented interface
  ///
  /// @{
  virtual int rowCount(const QModelIndex & parent = QModelIndex()) const {
    return trajectories->size()*2;
  }

  virtual int columnCount(const QModelIndex & parent = QModelIndex()) const {
    return columns.size();
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
      if(role == Qt::DisplayRole) {
        return columns[section].name;
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





TrajectoryParametersDisplay::TrajectoryParametersDisplay(FitWorkspace * ws) :
  workspace(ws)
{
  QHBoxLayout * layout = new QHBoxLayout(this);
  view = new CurveView;
  layout->addWidget(view, 1);
  parametersLayout = new QGridLayout;
  layout->addLayout(parametersLayout);
  QStringList pnames = workspace->parameterNames();
  for(int i = 0; i < pnames.size(); i++) {
    QCheckBox * cb = new QCheckBox(pnames[i]);
    parametersLayout->addWidget(cb, i, 0);
    parameters << cb;
    views << QList<gsl_vector_view>();

    parameterDisplays << QList<TuneableDataDisplay *>();

    connect(cb, &QCheckBox::stateChanged, this, [i, this](int state) {
        for(TuneableDataDisplay * d : parameterDisplays[i])
          d->toggleDisplay(state == Qt::Checked);
      }
      );
  }

  perpendicularCoordinates = workspace->perpendicularCoordinates;
  int nbds = workspace->data()->datasets.size();
  if(perpendicularCoordinates.size() != nbds) {
    Terminal::out << "Wrong number of perpendicular coordinates, making them up" << endl;
    perpendicularCoordinates.clear();
    for(int i = 0; i < nbds; i++)
      perpendicularCoordinates << i;
  }
  zero = perpendicularCoordinates;
  for(int i = 0; i < zero.size(); i++)
    zero[i] = 0;

}

void TrajectoryParametersDisplay::setupTrajectory(int index,
                                                  const FitTrajectory * traj,
                                                  bool isFinal)
{
  QPair<const FitTrajectory *, bool> p(traj, isFinal);
  QTextStream o(stdout);
  o << "parameterDisplays: "
    << parameterDisplays.size() << endl;

  int trjIndex = -1;
  int i = 0;
  for(FitTrajectory & t : workspace->trajectories) {
    if(&t == traj) {
      trjIndex = i;
      break;
    }
    ++i;
  }

  QString n = QString("#%1 (%2)").arg(trjIndex).
    arg(isFinal ? "F" : "S");
  
  while(parameterDisplays[0].size() <= index) {
    int sz = parameterDisplays[0].size();
    o << "Sz: : " << sz << endl;
    for(int i = 0; i < parameterDisplays.size(); i++) {
      TuneableDataDisplay * d = new TuneableDataDisplay("", view);
      QColor c;
      if(sz == 0)
        c = soas().graphicsSettings().dataSetPen(i).color();
      else {
        c = parameterDisplays[i][sz-1]->currentColor();
        double h,s,v;
        c = c.toHsv();
        c.getHsvF(&h, &s, &v);
        s *= 0.5;
        c.setHsvF(h, s, v);
      }
      d->changeColor(c);
      parametersLayout->addWidget(d, i, sz+1);
      d->toggleDisplay(parameters[i]->isChecked());
      o << "Sz2: " << i << endl;
      parameterDisplays[i] << d;
      gsl_vector_view v;
      views[i] << v << v;
    }
    trajectories << QPair<const FitTrajectory *, bool>(NULL, false);
  }

  o << "Index: " << index << " -- " << trajectories.size() << endl;
  if(trajectories[index] == p)
    return;

  trajectories[index] = p;

  for(int i = 0; i < parameterDisplays.size(); i++) {
    int nbparams = parameters.size();
    int nbds = workspace->data()->datasets.size();
    if(isFinal) {
      views[i][index*2] =
        gsl_vector_view_array_with_stride(const_cast<double*>(traj->finalParameters.data() + i),
                                          nbparams, nbds);
      views[i][index*2+1] =
        gsl_vector_view_array_with_stride(const_cast<double*>(traj->parameterErrors.data() + i),
                                          nbparams, nbds);
    }
    else {
      views[i][index*2] =
        gsl_vector_view_array_with_stride(const_cast<double*>(traj->initialParameters.data() + i),
                                          nbparams, nbds);
      views[i][index*2+1] =
        gsl_vector_view_array_with_stride(const_cast<double*>(zero.data() + i),
                                          nbparams, nbds);
    }
    XYIterable * xy =
      new XYIGSLVectors(perpendicularCoordinates.toGSLVector(),
                        &views[i][index*2].vector,
                        &views[i][index*2+1].vector);
    o << "Vector: " << perpendicularCoordinates.size()
      << " -- " << views[i][index*2+1].vector.size << endl;
    
    CurvePoints * cds =
      parameterDisplays[i][index]->setSource(xy);
    cds->relativeErrorBar = true;
    cds->brush = parameterDisplays[i][index]->currentColor();
    cds->size = 5;              /// @todo make that customizable
    cds->countBB = true;

    parameterDisplays[i][index]->setName(n);
  }

  
}

void TrajectoryParametersDisplay::displayRows(const QSet<int>& trjs)
{
  int idx = 0;
  for(int i : trjs)
    setupTrajectory(idx++, &(workspace->trajectories)[i/2], i % 2);
  for(QList<TuneableDataDisplay * > & lst : parameterDisplays) {
    while(lst.size() > idx) {
      delete lst.takeLast();
    }
  }
}

//////////////////////////////////////////////////////////////////////

/// The class that displays both a table view of the trajectories and 
class DoubleDisplay : public QWidget {
public:

  TrajectoryParametersDisplay * display;
  QTableView * tableView;

  DoubleDisplay(TrajectoriesModel * model, FitWorkspace * workspace)  {
    QSplitter * main = new QSplitter(Qt::Horizontal);
    tableView = new QTableView;
    tableView->setModel(model);
    main->addWidget(tableView);

    display = new TrajectoryParametersDisplay(workspace);
    main->addWidget(display);

    QHBoxLayout * l = new QHBoxLayout(this);
    l->addWidget(main);

    connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this,
            [this](const QItemSelection &selected,
                   const QItemSelection &deselected) {
              QSet<int> d;
              for(const QModelIndex & cur :
                    tableView->selectionModel()->selectedIndexes()) {
                int idx = cur.row();
                d.insert(idx);
              }
              display->displayRows(d);
            });
  };
  
};


//////////////////////////////////////////////////////////////////////

/// A model for displaying all the "clusters" (the flagged trajectories)
class FlaggedTrajectoriesModel : public QAbstractTableModel {
  /// The list of trajectories
  FitTrajectories * trajectories;

  /// The underlying fit data...
  const FitData * fitData;


  class Item  {
  public:
    QString name;


    std::function<QVariant (const QList<const FitTrajectory*> & trj,
                            const QString & name,
                            int role)> fcn;
    
    /// Number of the dataset
    int dataset;

    /// Index in the parameters vector
    int index;

    /// The parameter index -- not unique, unlike index.
    int parameterIndex;

    Item(const QString & n,
         std::function<QVariant (const QList<const FitTrajectory*> & trj,
                                 const QString & name,
                                 int role)> f,
         int ds = -1, int idx = -1, int pidx = - 1) :
      name(n), fcn(f), dataset(ds), index(idx), parameterIndex(pidx) {;};
  };

  
  QList<Item> columns;

  /// The current list of flags. Always sorted (?)
  QStringList flags;

  QList<const FitTrajectory *> flaggedTrajectories(const QString & flag) const {
    /// @todo setup some cache ?
    QList<const FitTrajectory *> rv;
    for(int i = 0; i < trajectories->size(); i++) {
      if((*trajectories)[i].flagged(flag))
        rv << &((*trajectories)[i]);
    }
    return rv;
  };

  /// The trajectories model, mainly for coloring the lines. Can be NULL
  const TrajectoriesModel * trajectoriesModel;

public:

  // int col(const QString & n) {
  //   for(int i = 0; i < columns.size(); i++)
  //     if(columns[i].name == n)
  //       return i;
  //   return -1;
  // };
    

  FlaggedTrajectoriesModel(FitTrajectories * trj, const FitData * d,
                           const TrajectoriesModel * md = NULL) :
    trajectories(trj), fitData(d), trajectoriesModel(md) {
    columns << Item("flag", [](const QList<const FitTrajectory*> & /*trj*/,
                               const QString & name,
                               int role) -> QVariant {
                              if(role == Qt::DisplayRole) {
                                return name;
                              }
                              return QVariant();
                            })
            << Item("count",
                    [](const QList<const FitTrajectory*> & trjs,
                       const QString & /*name*/,
                       int role) -> QVariant {
                      if(role == Qt::DisplayRole) {
                                 return trjs.size();
                               }
                               return QVariant();
                             })
            << Item("residuals",
                    [](const QList<const FitTrajectory*> & trjs,
                       const QString & /*name*/,
                       int role) -> QVariant {
                      if(role == Qt::DisplayRole) {
                        double min = 0, max = 0, s = 0;
                        int nb = 0;
                        for(const FitTrajectory * trj : trjs) {
                          double res = trj->residuals;
                          if(nb == 0) {
                            min = res;
                            max = res;
                          }
                          min = std::min(res, min);
                          max = std::max(res, max);
                          s += res;
                          nb += 1;
                        }
                        return QString("%1 to %2 (%3 avg)").
                          arg(min).arg(max).arg(s/nb);
                      }
                      return QVariant();
                    })
      ;

    int nbds = fitData->datasets.size();
    int n = fitData->parametersPerDataset();
    for(int i = 0; i < nbds; i++) {
      for(int j = 0; j < n; j++) {
        QString na = QString("%1[%2]").
          arg(fitData->parameterDefinitions[j].name).
          arg(i);

        int col = i*n+j;
        columns << Item(na,
                        [this,j,i,n,col](const QList<const FitTrajectory*> & trjs,
                                         const QString & /*name*/,
                                         int role) -> QVariant {
                          if(role == Qt::DisplayRole || role == Qt::EditRole) {
                            int nb = 0;
                            double sx = 0;
                            double sxx = 0;
                            for(const FitTrajectory * trj : trjs) {
                              nb += 1;
                              double x = trj->finalParameters[col];
                              sx += x;
                              sxx += x*x;
                            }
                            sx /= nb;
                            sxx /= nb;
                            return QString("%1 +- %2").
                              arg(sx/nb).
                              arg(sqrt(sxx - sx*sx));
                          }
                          return QVariant();
                        }, i, col, j);
      }
    }
    update();
  };

  QVariant data(const QString & flag, int col, int role) const {
    QList<const FitTrajectory *> lst = flaggedTrajectories(flag);
    QVariant d = columns[col].fcn(lst, flag, role);
    if(role == Qt::BackgroundRole && d.isNull() && trajectoriesModel)
      d = trajectoriesModel->getFlagColor(flag);
    return d;
  };

  /// @name Reimplemented interface
  ///
  /// @{
  virtual int rowCount(const QModelIndex & parent = QModelIndex()) const {
    return flags.size();
  }

  virtual int columnCount(const QModelIndex & parent = QModelIndex()) const {
    return columns.size();
  };

  virtual QVariant data(const QModelIndex & index,
                        int role) const {
    if(! index.isValid())
      return QVariant();
    
    if(index.column() < 0 || index.column() >= columns.size())
      return QVariant();

    if(index.row() < 0 || index.row() >= flags.size())
      return QVariant();

    return data(flags[index.row()], index.column(), role);
  };

  virtual QVariant headerData(int section, Qt::Orientation orientation,
                              int role) const {
    if(orientation == Qt::Horizontal) {
      if(role == Qt::DisplayRole) {
        return columns[section].name;
      }
      return QVariant();
    }
    if(orientation == Qt::Vertical) {
      if(role == Qt::DisplayRole) {
        return flags.value(section);
      }
    }
    return QVariant();
  }
  /// @}

  void update() {
    flags = trajectories->allFlags().toList();
    std::sort(flags.begin(), flags.end());
    emit(layoutChanged());
  };

  QString flag(const QModelIndex & index) const {
    return flags.value(index.row());
  };

};


//////////////////////////////////////////////////////////////////////


void FitTrajectoryDisplay::onSelectionChanged()
{
  QSet<int> trjs;
  for(const QModelIndex & idx : parametersDisplay->selectionModel()
        ->selectedIndexes()) {
    trjs.insert(idx.row());
  }
  graphicalDisplay->displayRows(trjs);
}

FitTrajectoryDisplay::FitTrajectoryDisplay(FitWorkspace * ws) :
  workspace(ws), fitData(ws->data())
{
  setWindowModality(Qt::WindowModal);
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
  parametersTables = new SynchronizedTables;
  parametersDisplay = new QTableView();

  QWidget * wdgt = new QWidget;
  t->addWidget(wdgt);
  QVBoxLayout * l = new QVBoxLayout(wdgt);

  QSplitter * splt = new QSplitter(Qt::Vertical);
  parametersTables->addTable(parametersDisplay);
  splt->addWidget(parametersTables);

  parametersDisplay->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(parametersDisplay, SIGNAL(doubleClicked(const QModelIndex &)),
          SLOT(onDoubleClick(const QModelIndex &)));

  l->addWidget(splt);

  model = new TrajectoriesModel(&workspace->trajectories, fitData);
  parametersDisplay->setModel(model);

  parametersDisplay->setSortingEnabled(true);
  parametersDisplay->horizontalHeader()->
    setSortIndicator(model->col("date"), Qt::AscendingOrder);

  

  connect(parametersDisplay, 
          SIGNAL(customContextMenuRequested(const QPoint&)),
          SLOT(contextMenuOnTable(const QPoint&)));

  connect(parametersDisplay->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &,
                                  const QItemSelection &)),
          this,
          SLOT(onSelectionChanged()));

  QTableView * tv = new QTableView();
  tv->setModel(model);
  parametersTables->addTable(tv);


  QHBoxLayout * hb = new QHBoxLayout();

  QPushButton * bt = new QPushButton("Trim");
  connect(bt, SIGNAL(clicked()), SLOT(trim()));
  hb->addWidget(bt);

  bt = new QPushButton("Previous buffer");
  connect(bt, SIGNAL(clicked()), SLOT(previousBuffer()));
  hb->addWidget(bt);

  bt = new QPushButton("Next buffer");
  connect(bt, SIGNAL(clicked()), SLOT(nextBuffer()));
  hb->addWidget(bt);

  QCheckBox * check = new QCheckBox("Color by flag");
  QObject::connect(check, &QCheckBox::clicked,
                   this, [this, check]() {
                           model->setFlagColor(check->isChecked());
                         });
  hb->addWidget(check);

  hb->addStretch();


  l->addLayout(hb);

  // explorationControls = new QScrollArea;
  // l->addWidget(explorationControls);



  tabs->addTab(wdgt, "Table");

  //////////////////////////////
  // Setup of curve view


  graphicalDisplay = new TrajectoryParametersDisplay(workspace);

  
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
  // Setup of the mixed display
  doubleDisplay = new DoubleDisplay(model, workspace);
  doubleDisplay->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(doubleDisplay->tableView, 
          SIGNAL(customContextMenuRequested(const QPoint&)),
          SLOT(contextMenuOnTable(const QPoint&)));

  tabs->addTab(doubleDisplay, "Parameters");


  //////////////////////////////
  // Fourth tab
  QWidget * fltab = new QWidget;
  vl = new QVBoxLayout(fltab);
  flagsView = new QTableView;
  vl->addWidget(flagsView);
  
  flagsModel =
    new FlaggedTrajectoriesModel(&workspace->trajectories, fitData, model);
  flagsView->setModel(flagsModel);
  flagsView->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(flagsView, 
          SIGNAL(customContextMenuRequested(const QPoint&)),
          SLOT(flagsViewContextMenu(const QPoint&)));

  tabs->addTab(fltab, "Flags/clusters");


  /// Here the close button at the bottom:
  hb = new QHBoxLayout;

  hb->addStretch();
  
  QDialogButtonBox * box = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(box, SIGNAL(rejected()), SLOT(close()));
  hb->addWidget(box);

  t->addLayout(hb);





  //////////////////////////////

  // setupExploration();

  connect(workspace, SIGNAL(finishedFitting(int)), 
          SLOT(update()), Qt::QueuedConnection);

  connect(workspace, SIGNAL(finishedFitting(int)), 
          view, SLOT(repaint()), Qt::QueuedConnection);



  /////////////////////////////
  /// Context menu preparation

  
  addCMAction("Reuse parameters", this, SLOT(reuseCurrentParameters()),
              QKeySequence(QString("Ctrl+Shift+U")));
  addCMAction("Reuse parameters for this dataset", this,
              SLOT(reuseParametersForThisDataset()),
              QKeySequence(QString("Ctrl+U")));

  addCMSeparator();

  addCMAction("Set as Reference", this, SLOT(setAsReference()),
              QKeySequence(QString("Ctrl+R")));
  addCMAction("Clear reference", this, SLOT(clearReference()),
              QKeySequence(QString("Ctrl+Shift+R")));

  addCMSeparator();

  addCMAction("Next buffer", this, SLOT(nextBuffer()),
              QKeySequence(QString("Ctrl+PgDown")));
  addCMAction("Previous buffer", this, SLOT(previousBuffer()),
              QKeySequence(QString("Ctrl+PgUp")));

  addCMSeparator();

  addCMAction("Hide fixed", this, SLOT(hideFixed()),
              QKeySequence(QString("Ctrl+H")));
  addCMAction("Hide selected", this, SLOT(hideSelected()));
  addCMAction("Hide unselected in this dataset",
              this, SLOT(hideButSelected()));
  addCMAction("Hide unselected everywhere",
              this, SLOT(hideButSelectedEverywhere()));
  addCMAction("Show all", this, SLOT(showAll()),
              QKeySequence(QString("Ctrl+Shift+H")));

  addCMSeparator();

  addCMAction("Sort", this, SLOT(sortByCurrentColumn()),
              QKeySequence(QString("Ctrl+S")));
  addCMAction("Reverse sort", this, SLOT(reverseSortByCurrentColumn()),
              QKeySequence(QString("Ctrl+Shift+S")));

  addCMSeparator();

  addCMAction("Delete trajectory", this, SLOT(deleteSelectedTrajectories()),
              QKeySequence(QString("Del")));
}

void FitTrajectoryDisplay::updateModels()
{
  model->update();
  flagsModel->update();
}

void FitTrajectoryDisplay::update()
{
  updateModels();
  // parametersDisplay->setRowCount(trajectories->size() * 2);
  parametersDisplay->resizeColumnsToContents();
}

FitTrajectoryDisplay::~FitTrajectoryDisplay()
{
}

void FitTrajectoryDisplay::addCMAction(const QString & name,
                                       QObject * receiver, 
                                       const char * slot,
                                       const QKeySequence & shortCut)
{
  QString str = name;
  if(! shortCut.isEmpty())
    str += "   (" + shortCut.toString() + ")";
  QAction * ac = ActionCombo::createAction(name, receiver,
                                           slot, shortCut, this);
  contextActions << ac;
  QWidget::addAction(ac);
}

void FitTrajectoryDisplay::addCMSeparator()
{
  QAction * a = new QAction(this);
  a->setSeparator(true);
  contextActions << a;
}


void FitTrajectoryDisplay::contextMenuOnTable(const QPoint & pos)
{
  QMenu menu;

  // Context menu for the main view
  if(tabs->currentIndex() == 0) {
    for(int i = 0; i < contextActions.size(); i++)
      menu.addAction(contextActions[i]);

    // Adding and removing tags
    menu.addSeparator();
  }
  QMenu s1("Add flags");
  QSet<QString> flgs = workspace->trajectories.allFlags();

  if(flgs.size() > 0) {
    QStringList ls = flgs.toList();
    std::sort(ls.begin(), ls.end());
    for(QString fl : ls) {
      QAction * ac = new QAction(fl, this);
      QObject::connect(ac, &QAction::triggered,
                       this, [fl, this] {
                               addFlagToSelected(fl);
                             });
      s1.addAction(ac);
    }
    s1.addSeparator();
  }
  
  QAction * ac = ActionCombo::createAction("New flag",
                                           this,
                                           SLOT(promptAddFlag()),
                                           QKeySequence(), this);
  s1.addAction(ac);
  menu.addMenu(&s1);

  
  QMenu s2("Remove flags");
  QSet<QString> rFl;
  for(int i : selectedTrajectories())
    rFl += workspace->trajectories[i].flags;

  if(rFl.size() > 0) {
    QStringList ls = rFl.toList();
    std::sort(ls.begin(), ls.end());
    for(QString fl : ls) {
      QAction * ac = new QAction(fl, this);
      QObject::connect(ac, &QAction::triggered,
                       this, [fl, this] {
                               removeFlagToSelected(fl);
                             });
      s2.addAction(ac);
    }
    s2.addSeparator();
    QAction * ac = new QAction("All", this);
    QObject::connect(ac, &QAction::triggered,
                     this, [ls, this] {
                       for(const QString & fl : ls)
                         removeFlagToSelected(fl);
                     });
    s2.addAction(ac);

  }
  
  menu.addMenu(&s2);
  menu.exec(view->viewport()->mapToGlobal(pos));
}

void FitTrajectoryDisplay::flagsViewContextMenu(const QPoint & pos)
{
  QMenu menu;
  QAction * ac = new QAction("Reuse best", this);
  QObject::connect(ac, &QAction::triggered,
                   this,
                   [this] {
                     QString flg = flagsModel->flag(flagsView->currentIndex());
                     if(! flg.isEmpty())
                       reuseFlaggedParameters(flg, true);
                   });
  menu.addAction(ac);

  ac = new QAction("Reuse average", this);
  QObject::connect(ac, &QAction::triggered,
                   this,
                   [this] {
                     QString flg = flagsModel->flag(flagsView->currentIndex());
                     if(! flg.isEmpty())
                       reuseFlaggedParameters(flg, false);
                   });
  menu.addAction(ac);
  menu.exec(view->viewport()->mapToGlobal(pos));
}


void FitTrajectoryDisplay::reuseFlaggedParameters(const QString & flag,
                                                  bool best)
{
  FitTrajectories trjs = workspace->trajectories.flaggedTrajectories(flag);
  Vector v;
  if(best)
    v = trjs.best().finalParameters;
  else {
    int nb = 0;
    for(const FitTrajectory & tr : trjs) {
      if(nb == 0)
        v = tr.finalParameters;
      else
        v += tr.finalParameters;
    }
    v *= (1.0/nb);
  }
    
  
  workspace->restoreParameterValues(v);
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

void FitTrajectoryDisplay::reuseParametersForThisDataset()
{
  // Send back the given parameters to the fitdialog
  int idx = parametersDisplay->currentIndex().row();
  int col = parametersDisplay->currentIndex().column();
  int ds = model->dataset(col);
  if(ds >= 0) {
    Vector parameters;
    if(idx % 2)
      parameters = workspace->trajectories[idx/2].finalParameters;
    else
      parameters = workspace->trajectories[idx/2].initialParameters;
    workspace->restoreParameterValues(parameters, ds);
  }
}

QList<int> FitTrajectoryDisplay::selectedTrajectories() const
{
  QSet<int> trjs;
  QModelIndexList indexes;
  if(tabs->currentIndex() == 0) // Normal view
    indexes =
      parametersDisplay->selectionModel()->selectedIndexes();

  if(tabs->currentIndex() == 2) // Parameters view
    indexes =
      doubleDisplay->tableView->selectionModel()->selectedIndexes();

  for(const QModelIndex & idx : indexes)
    trjs.insert(idx.row()/2);
  
  QList<int> l = trjs.toList();
  std::sort(l.begin(), l.end());
  return l;
}

void FitTrajectoryDisplay::deleteSelectedTrajectories()
{
  QList<int> l = selectedTrajectories();
  while(l.size() > 0)
    workspace->trajectories.remove(l.takeLast());
  updateModels();
}


void FitTrajectoryDisplay::addFlagToSelected(const QString & flag)
{
  QList<int> l = selectedTrajectories();
  for(int i : l)
    workspace->trajectories[i].addFlag(flag);
  updateModels();
}

void FitTrajectoryDisplay::removeFlagToSelected(const QString & flag)
{
  QList<int> l = selectedTrajectories();
  for(int i : l)
    workspace->trajectories[i].removeFlag(flag);
  updateModels();
}

void FitTrajectoryDisplay::promptAddFlag()
{
  QString flag = QInputDialog::getText(this, "Flag name",
                                       "New flag name to use");
  if(! flag.isEmpty())
    addFlagToSelected(flag);
                                       
}

void FitTrajectoryDisplay::nextBuffer()
{
  QModelIndex idx = parametersDisplay->currentIndex();
  int col = idx.column();
  int nbtot = model->columnCount(idx);
  int ds = model->dataset(col);

  while(col < nbtot) {
    if(model->dataset(col) != ds) {
      idx = idx.sibling(idx.row(), col);
      parametersDisplay->setCurrentIndex(idx);
      return;
    }
    col += 1;
  }
}

void FitTrajectoryDisplay::previousBuffer()
{
  QModelIndex idx = parametersDisplay->currentIndex();
  int col = idx.column();
  int ds = model->dataset(col);

  while(col > 0) {
    col -= 1;
    if(model->dataset(col) != ds) {
      idx = idx.sibling(idx.row(), col);
      parametersDisplay->setCurrentIndex(idx);
      return;
    }
  }
}


void FitTrajectoryDisplay::trim(double threshold)
{
  int nb = workspace->trajectories.trim(threshold);
  Terminal::out << "Trimming at threshold " << threshold
                << ", removed " << nb
                << " trajectories" << endl;
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

void FitTrajectoryDisplay::hideFixed()
{
  QModelIndex cur = parametersDisplay->currentIndex();
  int nbtot = model->columnCount(cur);
  // Send back the given parameters to the fitdialog
  int idx = cur.row();
  if(idx < 0)
    return;
  const FitTrajectory * trj = &(workspace->trajectories[idx/2]);
  for(int i = 0; i < nbtot; i++) {
    if(model->isFixed(i, trj))
      parametersDisplay->hideColumn(i);
  }
}


void FitTrajectoryDisplay::hideSelected()
{
  // Slightly different from others
  QList<QModelIndex> selected = parametersDisplay->selectionModel()
    ->selectedIndexes();
  
  for(const QModelIndex & idx : selected)
    parametersDisplay->hideColumn(idx.column());

}



void FitTrajectoryDisplay::hideButSelected()
{
  // Hides all the parameters but the selected within the given
  // dataset.

  QList<QModelIndex> selected = parametersDisplay->selectionModel()
    ->selectedIndexes();
  QSet<int> parameters;
  int ds = -1;
  for(const QModelIndex & idx: selected) {
    int pid = model->parameterIndex(idx.column());
    if(pid >= 0)
      parameters.insert(pid);
    int d = model->dataset(idx.column());
    if(d >= 0 && ds < 0)
      ds = d;
  }

  if(ds < 0)
    return;                     // Nothing to do

  if(selected.size() == 0)
    return;                     // nothing to do ?
  int nbtot = model->columnCount(selected.first());
  for(int i = 0; i < nbtot; i++) {
    int pid = model->parameterIndex(i);
    if(pid >= 0 && model->dataset(i) == ds
       && (! parameters.contains(pid)))
      parametersDisplay->hideColumn(i);
  }
}

void FitTrajectoryDisplay::hideButSelectedEverywhere()
{
  // Hides all the parameters but the selected within the given
  // dataset.

  QList<QModelIndex> selected = parametersDisplay->selectionModel()
    ->selectedIndexes();
  QSet<int> parameters;
  for(const QModelIndex & idx: selected) {
    int pid = model->parameterIndex(idx.column());
    if(pid >= 0)
      parameters.insert(pid);
  }

  if(selected.size() == 0)
    return;                     // nothing to do ?

  int nbtot = model->columnCount(selected.first());
  for(int i = 0; i < nbtot; i++) {
    int pid = model->parameterIndex(i);
    if(pid >= 0 && (! parameters.contains(model->parameterIndex(i))))
      parametersDisplay->hideColumn(i);
  }
}


void FitTrajectoryDisplay::showAll()
{
  QModelIndex idx = parametersDisplay->currentIndex();
  int nbtot = model->columnCount(idx);
  for(int i = 0; i < nbtot; i++)
    parametersDisplay->showColumn(i);
}



void FitTrajectoryDisplay::sortByCurrentColumn()
{
  QModelIndex idx = parametersDisplay->currentIndex();
  model->sort(idx.column());
}

void FitTrajectoryDisplay::reverseSortByCurrentColumn()
{
  QModelIndex idx = parametersDisplay->currentIndex();
  model->sort(idx.column(), Qt::DescendingOrder);
}

void FitTrajectoryDisplay::setAsReference()
{
  QModelIndex cur = parametersDisplay->currentIndex();
  int idx = cur.row();
  if(idx < 0)
    return;
  const FitTrajectory * trj = &(workspace->trajectories[idx/2]);
  model->referenceTrajectory = trj;
  model->update();
}

void FitTrajectoryDisplay::clearReference()
{
  model->referenceTrajectory = NULL;
  model->update();
}

void FitTrajectoryDisplay::browseTrajectories(FitWorkspace * ws)
{
  if(! ws)
    ws = FitWorkspace::currentWorkspace();
  if(! ws)
    throw InternalError("Somehow called when no workspace is available");
  FitTrajectoryDisplay d(ws);
  d.exec();
}


#include <fitdialog.hh>

void FitTrajectoryDisplay::onDoubleClick(const QModelIndex & /*index*/)
{
  reuseCurrentParameters();
  FitDialog * dlg = FitDialog::currentDialog();
  if(dlg)
    dlg->compute();
}
