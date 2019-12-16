/**
   \file filepromptwidget.hh
   A dialog box for prompting all the arguments/options of a command.
   Copyright 2019 by CNRS/AMU

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
#ifndef __FILEPROMPTWIDGET_HH
#define __FILEPROMPTWIDGET_HH


/// A simple widget for editing a file name with a button to choose
class FilePromptWidget : public QWidget {
  Q_OBJECT;

  QLineEdit * editor;

  bool isDir;

  bool isSave;

  QString label;
public:
  FilePromptWidget(QWidget * parent, bool isDir, bool isSave,
                   const QString & promptLabel = QString());
  ~FilePromptWidget();

  QString fileName() const;

public slots:
  void setFileName(const QString & name);

protected slots:
  void prompt();
};



#endif
