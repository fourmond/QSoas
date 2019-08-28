/*
  filepromptwidget.cc: implementation of arguments/option prompting
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
#include <filepromptwidget.hh>


FilePromptWidget::FilePromptWidget(QWidget * parent, bool id, bool is,
                                   const QString & l) :
  QWidget(parent),
  isDir(id),
  isSave(is),
  label(l)
{
  QHBoxLayout * layout = new QHBoxLayout(this);
  editor = new QLineEdit;
  layout->addWidget(editor);

  QPushButton * bt = new QPushButton("...");
  layout->addWidget(bt);
  connect(bt, SIGNAL(clicked()), SLOT(prompt()));
}

FilePromptWidget::~FilePromptWidget()
{
}

QString FilePromptWidget::fileName() const
{
  return editor->text();
}

void FilePromptWidget::setFileName(const QString & file) 
{
  editor->setText(file);
}

void FilePromptWidget::prompt()
{
  QString f;
  if(isDir)
    f = QFileDialog::getExistingDirectory(this, (label.isEmpty() ?
                                                 QString("Choose directory") :
                                                 label),
                                          QDir::currentPath());
  else {
    if(isSave)
      f = QFileDialog::getSaveFileName(this, (label.isEmpty() ?
                                              QString("Choose file name") :
                                              label),
                                       QDir::currentPath());
    else
      f = QFileDialog::getOpenFileName(this, (label.isEmpty() ?
                                              QString("Choose existing file") :
                                              label),
                                       QDir::currentPath());
  }
  if(! f.isEmpty())
    editor->setText(f);
}

