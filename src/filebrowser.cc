/*
  filebrowser.cc: implementation of the FileBrowser class
  Copyright 2021 by CNRS/AMU

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
#include <filebrowser.hh>

#include <command.hh>
#include <commandeffector-templates.hh>
// #include <general-arguments.hh>


//////////////////////////////////////////////////////////////////////

FileBrowser::FileBrowser()
{
  setupFrame();
}

FileBrowser::~FileBrowser()
{
  delete model;
}


void FileBrowser::setupFrame()
{
  QVBoxLayout * global = new QVBoxLayout(this);

  model = new FileBrowserModel();

  treeView = new QTreeView;
  treeView->setModel(model);

  QModelIndex curDirIdx = model->index(model->rootPath());
  QTextStream o(stdout);
  o << "Rt: " << model->rootPath() << endl;
  model->setRootPath(QDir::currentPath());
  treeView->setRootIndex(model->index(QDir::currentPath()));

  global->addWidget(treeView);



  QDialogButtonBox * buttons =
    new QDialogButtonBox(QDialogButtonBox::Ok,
                         Qt::Horizontal);
  global->addWidget(buttons);

  connect(buttons, SIGNAL(accepted()), SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), SLOT(reject()));
}


//////////////////////////////////////////////////////////////////////


static void filesBrowserCommand(const QString &)
{
  FileBrowser browser;
  browser.exec();
}

static Command 
edit("files-browser", // command name
     optionLessEffector(filesBrowserCommand), // action
     "file",  // group name
     NULL, // arguments
     NULL, // options
     "Browse files");
