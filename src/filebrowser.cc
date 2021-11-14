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
#include <metadatafile.hh>

#include <settings-templates.hh>


/// A simple cache for the data
class FileData {
public:

  /// The underlying backend
  DataBackend * backend;

  /// If true, there is no backend to load this file (or the ignore
  /// backend)
  bool noBackend;

  ValueHash cachedMeta;

  /// To check
  QDateTime lastModified;

  QString fullPath;

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
    dt->fullPath = info.absoluteFilePath();
    try {
      File fl(dt->fullPath, File::BinaryRead);
      dt->backend = DataBackend::backendForStream(fl, info.absoluteFilePath());
      if(dt->backend && dt->backend->codeName() != "ignore") {
        dt->noBackend = false;
        // Here, we read the meta-data
        MetaDataFile md(dt->fullPath);
        md.read();
        dt->cachedMeta = md.metaData;
      }
    }
    catch(RuntimeError & re) {
    }

    return dt;
  };

  void setMeta(const QString & meta, const QVariant & data) {
    if(noBackend)
      return;
    MetaDataFile md(fullPath);
    md.read();
    cachedMeta = md.metaData;
    cachedMeta[meta] = data;
    md.metaData = cachedMeta;
    md.write();
  };
  
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
  QSet<QString> meta;
  directory = dir;
  fileList = File::listDirectory(dir);
  QTextStream o(stdout);
  for(int i = fileList.size() - 1; i >=0; i--) {
    if(fileList[i].isDir())
      fileList.takeAt(i);
    else {
      QStringList lst = cachedInfo(fileList[i])->cachedMeta.keys();
      for(const QString & s : lst) {
        meta.insert(s);
      }
    }
  }
  metaDataNames = meta.toList();
  std::sort(metaDataNames.begin(), metaDataNames.end());
  endResetModel();
}

void FileListModel::addMetaData(const QString & name)
{
  int found = -1;
  for(int i = 0; i < metaDataNames.size(); i++) {
    if(name == metaDataNames[i])
      return;                   // nothing to do
    if(name < metaDataNames[i]) {
      found = i;
      break;
    }
  }
  if(found == -1)
    found = metaDataNames.size();
  beginInsertColumns(index(0,0), metaBaseColumn() + found,
                     metaBaseColumn() + found + 1);
  metaDataNames.insert(found, name);
  endInsertColumns();
}

int FileListModel::metaBaseColumn() const
{
  return 4;
}

int FileListModel::columnCount(const QModelIndex & /*parent*/) const
{
  return metaBaseColumn() + metaDataNames.size();
}

int FileListModel::rowCount(const QModelIndex & /*parent*/) const
{
  return fileList.size();
}

QVariant FileListModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const
{
  if(orientation == Qt::Vertical)
    return QVariant();
  if(role == Qt::DisplayRole || role == Qt::EditRole) {
    if(section >= metaBaseColumn())
      return metaDataNames[section - metaBaseColumn()];
    switch(section) {
    case 0:
      return "File name";
    case 1:
      return "Size";
    case 2:
      return "Last modification";
    case 3:
      return "Backend";
    }
  }
  return QVariant();
}

Qt::ItemFlags FileListModel::flags(const QModelIndex & index) const
{
  if(! index.isValid())
    return Qt::NoItemFlags;
  int idx = index.row();
  if(idx < 0 || idx >= fileList.size())
    return Qt::NoItemFlags;

  int col = index.column();
  if(col < metaBaseColumn())
    return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
  FileData * fd = cachedInfo(fileList[idx]);
  if(fd->noBackend)
    return Qt::ItemIsSelectable|Qt::ItemIsEnabled;

  return Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable;
}

bool FileListModel::setData(const QModelIndex & index,
                            const QVariant & value, int role)
{
  if(role != Qt::EditRole)
    return false;
  if(! index.isValid())
    return false;
  int idx = index.row();
  if(idx < 0 || idx >= fileList.size())
    return false;

  int col = index.column();
  if(col < metaBaseColumn())
    return false;
  FileData * fd = cachedInfo(fileList[idx]);
  if(fd->noBackend)
    return false;
  
  fd->setMeta(metaDataNames[col - metaBaseColumn()], value);
  return true;
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

  if(col >= metaBaseColumn()) {
    int mdi = col - metaBaseColumn();
    if(fd->noBackend)
      return QVariant();// ?
    switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return fd->cachedMeta.value(metaDataNames[mdi], QVariant());
    };
  }

  return QVariant();
}

  


//////////////////////////////////////////////////////////////////////

static SettingsValue<QSize> browserSize("filesbrowser/size", QSize(700,500));
static SettingsValue<QByteArray> splitterState("filesbrowser/splitter", 
                                               QByteArray());


FileBrowser::FileBrowser()
{
  setupFrame();
  resize(browserSize);
}


FileBrowser::~FileBrowser()
{
  browserSize = size();
  splitterState = splitter->saveState();
  delete directoryView;
  delete directoryModel;
}


void FileBrowser::setupFrame()
{
  QVBoxLayout * global = new QVBoxLayout(this);

  splitter = new QSplitter(Qt::Horizontal);

  ////////////////////
  // directory
  
  directoryModel = new QFileSystemModel;
  directoryModel->setFilter(QDir::AllDirs|QDir::NoDotAndDotDot);
  QString base = directoryModel->myComputer().toString();
  directoryModel->setRootPath(base);

  directoryView = new QTreeView;
  directoryView->setModel(directoryModel);

  directoryView->setRootIndex(directoryModel->index(base));
  QModelIndex cd = directoryModel->index(QDir::currentPath());
  directoryView->setCurrentIndex(cd);

  connect(directoryView->selectionModel(),
          SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
          SLOT(directorySelected(const QModelIndex&)));

  for(int col : {1, 2, 3})
    directoryView->hideColumn(col);

  splitter->addWidget(directoryView);


  ////////////////////
  // file list

  listModel = new FileListModel;
  listModel->setDirectory(QDir::currentPath());

  listView = new QTableView;
  listView->setModel(listModel);
  listView->verticalHeader()->setVisible(false);
  listView->setShowGrid(false);

  splitter->addWidget(listView);
  splitter->setStretchFactor(1, 2);
  

  global->addWidget(splitter);

  if(! splitterState->isEmpty())
    splitter->restoreState(splitterState);


  QDialogButtonBox * buttons =
    new QDialogButtonBox(QDialogButtonBox::Ok,
                         Qt::Horizontal);
  global->addWidget(buttons);

  connect(buttons, SIGNAL(accepted()), SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), SLOT(reject()));

  // Delay the call to scroll to when we go back to the event loop.
  QMetaObject::invokeMethod(this, [this, cd] {
                                    directoryView->scrollTo(cd);
                                  },
    Qt::QueuedConnection);
}

void FileBrowser::directorySelected(const QModelIndex &index)
{
  QString dir = directoryModel->filePath(index);
  listModel->setDirectory(dir);
  listView->resizeColumnsToContents();
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
