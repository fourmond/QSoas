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
  logDisplay->setReadOnly(true);
  // We use a monospace font !
  logDisplay->setFont(QFont("monospace")); /// @todo customize settings
  logDisplay->setText("biniou");
  layout->addWidget(logDisplay);

  QHBoxLayout * h1 = new QHBoxLayout();
  h1->addWidget(new QLabel("Soas> "));
  commandLine = new QLineEdit;

  connect(commandLine, SIGNAL(returnPressed()), 
          SLOT(commandEntered()));
  h1->addWidget(commandLine);
  layout->addLayout(h1);

  this->setFocusProxy(commandLine);
  logDisplay->setFocusProxy(commandLine);
  // logDisplay
}

CommandWidget::~CommandWidget()
{
}

void CommandWidget::runCommand(const QStringList & raw)
{
  try {
    Command::runCommand(raw, this);
  }
  catch(const std::runtime_error & error) {
    QTextStream o(stderr);
    o << "Error: " << error.what() << endl;
  }
  catch(const std::logic_error & error) {
    QTextStream o(stderr);
    o << "Internal error: " << error.what() << endl;
  }
}

void CommandWidget::runCommand(const QString & str)
{
  QStringList split = Command::wordSplit(str);
  runCommand(split);
}

void CommandWidget::commandEntered()
{
  runCommand(commandLine->text());
  commandLine->clear();
}
