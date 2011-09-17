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
#include <terminal.hh>
#include <commandprompt.hh>
#include <soas.hh>
#include <dataset.hh>
#include <utils.hh>

#include <exceptions.hh>

using namespace Terminal;

CommandWidget * CommandWidget::theCommandWidget = NULL;

CommandWidget::CommandWidget() 
{
  if(! theCommandWidget)
    theCommandWidget = this;    // Or always ?
  QVBoxLayout * layout = new QVBoxLayout(this);

  QHBoxLayout * h1 = new QHBoxLayout();
  terminalDisplay = new QTextEdit();
  terminalDisplay->setReadOnly(true);
  // We use a monospace font !
  terminalDisplay->setFont(QFont("monospace")); /// @todo customize settings
  // terminalDisplay->document()-> 
  //   setDefaultStyleSheet("p { white-space: pre; }");
  // Doesn't seem to have any effect...
  h1->addWidget(terminalDisplay);

  sideBarLabel = new QLabel();
  h1->addWidget(sideBarLabel);

  layout->addLayout(h1);

  h1 = new QHBoxLayout();
  promptLabel = new QLabel("Soas> ");
  h1->addWidget(promptLabel);

  commandLine = new CommandPrompt;
  connect(commandLine, SIGNAL(returnPressed()), 
          SLOT(commandEntered()));
  connect(commandLine, SIGNAL(scrollRequested(int)), 
          SLOT(scrollTerminal(int)));
  h1->addWidget(commandLine);

  restrictedPrompt = new QLineEdit;
  h1->addWidget(restrictedPrompt);
  

  layout->addLayout(h1);

  this->setFocusProxy(commandLine);
  terminalDisplay->setFocusProxy(commandLine);
  terminalDisplay->setFocusPolicy(Qt::NoFocus);

  setLoopMode(false);
  restrictedPrompt->setVisible(false);
}

CommandWidget::~CommandWidget()
{
}

void CommandWidget::runCommand(const QStringList & raw)
{
  /// @todo use a different prompt whether the call is internal or
  /// external ?
  if(raw.isEmpty())
    return;                     // Nothing to do here.
  
  QString cmd = Command::unsplitWords(raw);
  out << bold("Soas> ") << cmd << endl;
  commandLine->addHistoryItem(cmd);
  commandLine->setEnabled(false);
  soas().showMessage(tr("Running: %1").arg(cmd));
  try {
    Command::runCommand(raw, this);
  }
  catch(const RuntimeError & error) {
    out << bold("Error: ") << error.message() << endl;
  }
  catch(const InternalError & error) {
    out << bold("Internal error: ") 
        << error.message() << endl
        << "This is a bug in Soas and may be worth reporting !" << endl;
  }
  commandLine->setEnabled(true);
  commandLine->setFocus();
}

void CommandWidget::runCommand(const QString & str)
{
  QStringList split = Command::wordSplit(str);
  runCommand(split);
}

void CommandWidget::commandEntered()
{
  QString cmd = commandLine->text();
  commandLine->clear();
  runCommand(cmd);
}

void CommandWidget::appendToTerminal(const QString & str)
{
  terminalDisplay->moveCursor(QTextCursor::End);
  terminalDisplay->insertPlainText(str);
  // and scroll to the bottom
  QScrollBar * sb = terminalDisplay->verticalScrollBar();
  sb->setSliderPosition(sb->maximum());
}

void CommandWidget::logString(const QString & str)
{
  if(theCommandWidget)
    theCommandWidget->appendToTerminal(str);
  else {
    QTextStream o(stdout);
    o << str;
  }
}

void CommandWidget::setLoopMode(bool loop)
{
  sideBarLabel->setVisible(loop);
  // commandLine->setEnabled(! loop);
  if(loop)
    commandLine->clearFocus();
  else
    commandLine->setFocus();
  
}

void CommandWidget::setSideBarLabel(const QString & str)
{
  sideBarLabel->setText(str);
}

QLineEdit * CommandWidget::enterPromptMode(const QString & prompt)
{
  commandLine->setVisible(false);
  restrictedPrompt->setText("");
  restrictedPrompt->setVisible(true);
  
  promptLabel->setText(prompt);

  
  restrictedPrompt->setFocus(); // ? 
  return restrictedPrompt;
}

void CommandWidget::leavePromptMode()
{
  commandLine->setVisible(true);
  restrictedPrompt->setVisible(false);
  restrictedPrompt->clearFocus();
  promptLabel->setText("Soas> ");
}

void CommandWidget::scrollTerminal(int nb)
{
  QScrollBar * sb = terminalDisplay->verticalScrollBar();
  int curValue = sb->value();
  int pageValue = sb->pageStep();
  sb->setValue(curValue + (nb * pageValue)/2);
}

void CommandWidget::printCurrentDataSetInfo()
{
  const DataSet * ds = soas().currentDataSet();
  if(ds)
    Terminal::out << tr("Current buffer now is: '") 
                  << ds->name << "'" << endl;
  else
    Terminal::out << tr("No current buffer") << endl;
}

QString CommandWidget::terminalContents() const
{
  return terminalDisplay->toPlainText();
}

void CommandWidget::runCommandFile(QIODevice * source)
{
  QTextStream in(source);
  QRegExp commentRE("^\\s*#.*");
  while(true) {
    QString line = in.readLine();
    if(line.isNull())
      break;
    if(commentRE.indexIn(line) == 0)
      continue;
    runCommand(line);
  }
}

void CommandWidget::runCommandFile(const QString & fileName)
{
  QFile file(fileName);
  Utils::open(&file, QIODevice::ReadOnly);
  runCommandFile(&file);
}

QStringList CommandWidget::history() const
{
  return commandLine->history();
}
