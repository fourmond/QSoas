/*
  filebrowser.cc: implementation of the FileBrowser and related class
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

#include <file.hh>
#include <fileinfo.hh>

#include <exceptions.hh>

#include <command.hh>
#include <commandeffector-templates.hh>

#include <databackend.hh>


/// A simple cache for the data
class FileData {
public:

  /// The underlying backend
  DataBackend * backend;

  /// If true, there is no backend to load this file (or the ignore
  /// backend)
  bool noBackend;

  /// To check
  QDateTime lastModified;

  QString backendName() const {
    if(backend) {
      if(noBackend)
        return "(ignored)";
      return backend->codeName();
    }
    return "(no backend)";
  };

  /// Returns a newly created FileData object.
  static FileData * dataForFile(const FileInfo & info) {
    FileData * dt = new FileData;

    dt->lastModified = info.lastModified();

    dt->backend = NULL;
    dt->noBackend = true;
    try {
      File fl(info.absoluteFilePath(), File::BinaryRead);
      dt->backend = DataBackend::backendForStream(fl, info.absoluteFilePath());
      if(dt->backend && dt->backend->codeName() != "ignore")
        dt->noBackend = false;
    }
    catch(RuntimeError & re) {
    }

    return dt;
  }
  
};


//////////////////////////////////////////////////////////////////////

FileListModel::FileListModel() :
  cachedData(1000)
{
}


FileData * FileListModel::cachedInfo(const FileInfo & info) const
{
  FileData * rv = cachedData.object(info.absoluteFilePath());
  if(! rv) {
    rv = FileData::dataForFile(info);
    cachedData.insert(info.absoluteFilePath(), rv);
  }
  return rv;
}

void FileListModel::setDirectory(const QString & dir)
{
  beginResetModel();
  directory = dir;
  fileList = File::listDirectory(dir);
  for(int i = fileList.size() - 1; i >=0; i--) {
    if(fileList[i].isDir())
      fileList.takeAt(i);
  }
  endResetModel();
}


int FileListModel::columnCount(const QModelIndex & /*parent*/) const
{
  return 4;
}

int FileListModel::rowCount(const QModelIndex & /*parent*/) const
{
  return fileList.size();
}

QVariant FileListModel::data(const QModelIndex & index, int role) const
{
  if(! index.isValid())
    return QVariant();
  int idx = index.row();
  if(idx < 0 || idx >= fileList.size())
    return QVariant();

  int col = index.column();

  FileData * fd = cachedInfo(fileList[idx]);
  // All ignore files are ignored by default
  if(fd->noBackend && role == Qt::ForegroundRole)
    return QColor(130, 130, 130);


  /// The name column
  if(col == 0) {
    switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return fileList[idx].fileName();
    }
    return QVariant();
  }

  /// The size column:
  if(col == 1) {
    switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return fileList[idx].size();
    }
    return QVariant();
  }

  /// The date column:
  if(col == 2) {
    switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return fileList[idx].lastModified();
    }
    return QVariant();
  }

  /// The backend column:
  if(col == 3) {
    switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return fd->backendName();
    };
  }

  return QVariant();
}

  


//////////////////////////////////////////////////////////////////////

FileBrowser::FileBrowser()
{
  setupFrame();
}

FileBrowser::~FileBrowser()
{
  delete directoryView;
  delete directoryModel;
}


void FileBrowser::setupFrame()
{
  QVBoxLayout * global = new QVBoxLayout(this);

  QSplitter * models = new  QSplitter(Qt::Horizontal);

  ////////////////////
  // directory
  
  directoryModel = new QFileSystemModel;
  directoryModel->setFilter(QDir::AllDirs|QDir::NoDotAndDotDot);
  QString base = directoryModel->myComputer().toString();
  directoryModel->setRootPath(base);

  directoryView = new QTreeView;
  directoryView->setModel(directoryModel);

  directoryView->setRootIndex(directoryModel->index(base));
  directoryView->setCurrentIndex(directoryModel->index(QDir::currentPath()));

  connect(directoryView->selectionModel(),
          SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
          SLOT(directorySelected(const QModelIndex&)));

  for(int col : {1, 2, 3})
    directoryView->hideColumn(col);

  models->addWidget(directoryView);


  ////////////////////
  // file list

  listModel = new FileListModel;
  listModel->setDirectory(QDir::currentPath());

  listView = new QTableView;
  listView->setModel(listModel);

  // A splitted would be better ?
  models->addWidget(listView);

  global->addWidget(models);

  QDialogButtonBox * buttons =
    new QDialogButtonBox(QDialogButtonBox::Ok,
                         Qt::Horizontal);
  global->addWidget(buttons);

  connect(buttons, SIGNAL(accepted()), SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), SLOT(reject()));
}

void FileBrowser::directorySelected(const QModelIndex &index)
{
  QString dir = directoryModel->filePath(index);
  listModel->setDirectory(dir);
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
