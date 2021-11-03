/**
   \file filebrowser.hh
   File handling
   Copyright 2020 by CNRS/AMU

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

/// A model to display files with 
class FileBrowserModel : public QFileSystemModel {
  Q_OBJECT;
public:
};

/// A dialog box to display the FileBrowserModel
class FileBrowser : public QDialog {
  Q_OBJECT;
  
  /// Setup the frame
  void setupFrame();

  /// The table widget
  QTreeView * treeView;

  /// The model
  FileBrowserModel * model;
public:
  FileBrowser();
  ~FileBrowser();

};


#endif
