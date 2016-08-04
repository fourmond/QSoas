/*
  parametersspreadsheet.cc: Implementation of the parameters spreadsheet
  Copyright 2015 by CNRS/AMU

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
#include <parametersspreadsheet.hh>
#include <settings-templates.hh>

#include <fitworkspace.hh>
#include <parametersitemmodel.hh>

// For the static function
#include <actioncombo.hh>


//////////////////////////////////////////////////////////////////////

static SettingsValue<QSize> spreadsheetSize("parametersspreadsheet/size", QSize(700,500));

ParametersSpreadsheet::ParametersSpreadsheet(FitWorkspace * params) :
  workspace(params)
{
  resize(spreadsheetSize);
  model = new ParametersItemModel(workspace);
  setupFrame();
}

ParametersSpreadsheet::~ParametersSpreadsheet()
{
  spreadsheetSize = size();
  delete model;
}

void ParametersSpreadsheet::addCMAction(const QString & name,
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


void ParametersSpreadsheet::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  view = new QTableView;
  view->setModel(model);
  layout->addWidget(view, 1);


  view->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(view, SIGNAL(customContextMenuRequested(const QPoint &)),
          SLOT(spawnContextMenu(const QPoint &)));

  // Then, bottom line with buttons...
  QHBoxLayout * bl = new QHBoxLayout;
  layout->addLayout(bl);
  QPushButton * bt;

  bl->addStretch(1);

  // bt = new QPushButton("Push visible");
  // bl->addWidget(bt);
  // connect(bt, SIGNAL(clicked()), SLOT(pushVisible()));

  // bt = new QPushButton("Push all");
  // bl->addWidget(bt);
  // connect(bt, SIGNAL(clicked()), SLOT(pushAll()));

  bt = new QPushButton("Close");
  bl->addWidget(bt);
  connect(bt, SIGNAL(clicked()), SLOT(accept()));


  addCMAction("Fix parameters", this, SLOT(fixParameters()),
              QKeySequence(QString("Ctrl+F")));
  addCMAction("Unfix parameters", this, SLOT(unfixParameters()),
              QKeySequence(QString("Ctrl+Shift+F")));
  addCMAction("Set parameters", this, SLOT(editSelected()),
              QKeySequence(QString("Ctrl+E")));
  addCMAction("Propagate parameters", this, SLOT(propagateDown()),
              QKeySequence(QString("Ctrl+P")));
  addCMAction("Reset to initial guess", this, SLOT(resetParameters()));
}

bool ParametersSpreadsheet::dataChanged() const
{
  return model->dataChanged();
}



void ParametersSpreadsheet::spawnContextMenu(const QPoint & pos)
{
  typedef enum {
    FixSelected,
    UnfixSelected,
    ResetToInitialGuess,
    EditSelected,
    Noop
  } En;
  QMenu menu;
  for(int i = 0; i < contextActions.size(); i++)
    menu.addAction(contextActions[i]);

  QAction * ac = menu.exec(view->viewport()->mapToGlobal(pos));
  if(ac)
    ac->trigger();
}

void ParametersSpreadsheet::editSelected()
{
  QModelIndexList indexes = view->selectionModel()->selectedIndexes();
  if(indexes.size() > 0) {
    bool ok = false;
    QVariant s = model->data(indexes[0], Qt::DisplayRole);
    QString txt = QInputDialog::getText(this, "edit", "change item",
                                        QLineEdit::Normal, s.toString(), &ok);
    if(ok) {
      for(int i = 0; i < indexes.size(); i++) {
        model->setData(indexes[i], txt,  Qt::EditRole);
      }
    }
  }
}

static bool lower(const QModelIndex & a, const QModelIndex & b)
{
  if(a.column() < b.column())
    return true;
  if(a.column() > b.column())
    return false;
  return a.row() < b.row();
}


void ParametersSpreadsheet::propagateDown()
{
  QModelIndexList indexes = view->selectionModel()->selectedIndexes();

  // We first sort, and then we just have to propagate the values
  qSort(indexes.begin(), indexes.end(), &::lower);

  int curCol = -1;
  QVariant curVal;

  for(int i = 0; i < indexes.size(); i++) {
    int col = indexes[i].column();
    if(col != curCol) {
      curVal = model->data(indexes[i], Qt::DisplayRole);
      curCol = col;
    }
    else 
      model->setData(indexes[i], curVal,  Qt::EditRole);
  }
}

void ParametersSpreadsheet::fixParameters(bool fix)
{
    model->setFixed(view->selectionModel()->selectedIndexes(),
                    fix);
}

void ParametersSpreadsheet::unfixParameters()
{
  fixParameters(false);
}

void ParametersSpreadsheet::resetParameters()
{
  model->resetValuesToInitialGuess(view->selectionModel()->selectedIndexes());
}
