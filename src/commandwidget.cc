/*
  commandwidget.cc: Main window for QSoas
  Copyright 2011 by Vincent Fourmond

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
#include <commandwidget.hh>
#include <command.hh>

CommandWidget::CommandWidget() 
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  logDisplay = new QTextEdit();
  layout->addWidget(logDisplay);

  QHBoxLayout * h1 = new QHBoxLayout();
  h1->addWidget(new QLabel("Soas> "));
  commandLine = new QLineEdit;
  h1->addWidget(commandLine);
  layout->addLayout(h1);
}

CommandWidget::~CommandWidget()
{
}
