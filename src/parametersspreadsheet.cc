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
  QHash<QAction *, int> actions;
  actions[menu.addAction("Fix parameters")] = FixSelected;
  actions[menu.addAction("Unfix parameters")] = UnfixSelected;
  actions[menu.addAction("Set parameters")] = EditSelected;
  actions[menu.addAction("Reset to initial guess")] = ResetToInitialGuess;

  int ac = actions.value(menu.exec(view->viewport()->mapToGlobal(pos)), Noop);
  switch(ac) {
  case FixSelected:
  case UnfixSelected:
    model->setFixed(view->selectionModel()->selectedIndexes(),
                    ac == FixSelected);
    break;
  case ResetToInitialGuess:
    model->resetValuesToInitialGuess(view->selectionModel()->selectedIndexes());
    break;
  case EditSelected:
    editSelected();
    break;
  case Noop:
  default:
    break;
  }
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
