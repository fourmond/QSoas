/**
   \file filebrowser.hh
   A file browser to display/edit meta-data
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
#ifndef __FILEBROWSER_HH
#define __FILEBROWSER_HH

#include <fileinfo.hh>

class FileData;

/// A model to display files with 
class FileListModel : public QAbstractTableModel {
  Q_OBJECT;

  /// The underlying file list
  QList<FileInfo> fileList;

  /// the current directory
  QString directory;

  
  /// A cache for storing the file information
  mutable QCache<QString, FileData> * cachedData;

  FileData * cachedInfo(const FileInfo & info) const;

  /// The names of the meta data currently displayed.
  QStringList metaDataNames;

  /// The number of the first column of meta-data
  int metaBaseColumn() const;


public:

  FileListModel();
  ~FileListModel();

  /// @name Reimplemented interface
  ///
  /// @{
  virtual int rowCount(const QModelIndex & parent) const override;

  virtual int columnCount(const QModelIndex & parent) const override;

  virtual QVariant data(const QModelIndex & index, int role) const override;


  virtual bool setData(const QModelIndex & index,
                       const QVariant & value, int role) override;

  virtual QVariant headerData(int section, Qt::Orientation orientation,
                              int role) const override;
  
  virtual Qt::ItemFlags flags(const QModelIndex & index) const override;
  /// @}


public slots:

  /// Sets the given directory
  void setDirectory(const QString & dir);

  /// Adds a meta data name to the list, and ensure the column is
  /// added properly.
  ///
  /// Returns the index of the newly created column.
  int addMetaData(const QString & name);

};

/// A dialog box to display the FileBrowserModel
class FileBrowser : public QDialog {
  Q_OBJECT;
  
  /// Setup the frame
  void setupFrame();

  /// The horizontal splitter
  QSplitter * splitter;

  /// The display of the directory hierarchy
  QTreeView * directoryView;

  /// The model for displaying
  QFileSystemModel * directoryModel;

  /// The display of the files
  QTableView * listView;

  /// The model for displaying the propertoes
  FileListModel * listModel;

  

public:
  FileBrowser();
  ~FileBrowser();

protected slots:
  /// Called whenever a new directory is selected.
  void directorySelected(const QModelIndex &index);

};


#endif
