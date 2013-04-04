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

#include <settings-templates.hh>
#include <temporarychange.hh>

using namespace Terminal;

CommandWidget * CommandWidget::theCommandWidget = NULL;
static SettingsValue<QString> logFileName("command/logfile", 
                                          QString("soas.log"));


static SettingsValue<QStringList> startupFiles("command/startup-files",
                                               QStringList());


static SettingsValue<QString> terminalFont("command/terminal-font",
                                           "Courier");

QStringList & CommandWidget::startupFiles()
{
  return ::startupFiles.ref();
}

CommandWidget::CommandWidget() : watcherDevice(NULL)
{
  if(! theCommandWidget)
    theCommandWidget = this;    // Or always ?
  QVBoxLayout * layout = new QVBoxLayout(this);

  if(! ((QString)logFileName).isEmpty()) {
    watcherDevice = new QFile(logFileName);
    watcherDevice->open(QIODevice::Append);
  }

  QHBoxLayout * h1 = new QHBoxLayout();
  terminalDisplay = new QTextEdit();
  terminalDisplay->setReadOnly(true);
  terminalDisplay->setContextMenuPolicy(Qt::NoContextMenu);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), 
          SLOT(onMenuRequested(const QPoint &)));

  // We use a monospace font !
  QFont mono(terminalFont);
  QFontInfo m(mono);
  QTextStream o(stdout);
  o << "Font used for terminal display: " << m.family() << endl;
  terminalDisplay->setFont(mono);
  // terminalDisplay->document()-> 
  //   setDefaultStyleSheet("p { white-space: pre; }");
  // Doesn't seem to have any effect...
  h1->addWidget(terminalDisplay);

  sideBarLabel = new QLabel();
  // Align the label on top, nicer.
  sideBarLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  h1->addWidget(sideBarLabel);

  layout->addLayout(h1);

  h1 = new QHBoxLayout();
  promptLabel = new QLabel("QSoas> ");
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
  terminalDisplay->setFocusPolicy(Qt::StrongFocus);

  setLoopMode(false);
  restrictedPrompt->setVisible(false);
}

CommandWidget::~CommandWidget()
{
  delete watcherDevice;
}

void CommandWidget::runCommand(const QStringList & raw)
{
  /// @todo use a different prompt whether the call is internal or
  /// external ?
  if(raw.isEmpty())
    return;                     // Nothing to do here.
  
  QString cmd = Command::unsplitWords(raw);
  out << bold("QSoas> ") << cmd << endl;
  commandLine->addHistoryItem(cmd);
  commandLine->setEnabled(false);

  soas().showMessage(tr("Running: %1").arg(cmd));
  try {
    TemporaryChange<QString> ch(curCmdline, raw.join(" "));
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
  catch(const ControlFlowException & flow) {
    commandLine->setEnabled(true);
    commandLine->setFocus();
    throw;                      // rethrow
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
  QStringList cmds = cmd.split("\n");
  try {
    for(int i = 0; i < cmds.size(); i++)
      runCommand(cmds[i]);
  }
  catch(const ControlFlowException & flow) {
    out << bold("Error: ") << "control flow command " << flow.message()
        << " cannot be used outside of a script" << endl;
  }

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
  if(theCommandWidget) {
    theCommandWidget->appendToTerminal(str);
    // If there is a watcher file, we duplicate there too.
    if(theCommandWidget->watcherDevice) {
      QTextStream o(theCommandWidget->watcherDevice);
      o << str;
    }
  }
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

void CommandWidget::runCommandFile(QIODevice * source, 
                                   const QStringList & args)
{
  QTextStream in(source);
  QRegExp commentRE("^\\s*#.*");
  try {
    while(true) {
      QString line = in.readLine();
      if(line.isNull())
        break;
      if(commentRE.indexIn(line) == 0)
        continue;

      // Argument substitution

      /// @todo escape ?
      for(int i = 0; i < args.size(); i++) {
        QString argname = QString("${%1}").arg(i + 1);
        line.replace(argname, args[i]);
      }
      runCommand(line);
    }
  }
  catch(const ControlFlowException & flow) {
    // Nothing to do !
  }
}

void CommandWidget::runCommandFile(const QString & fileName, 
                                   const QStringList & args)
{
  QFile file(fileName);
  TemporaryChange<QString> ch(scriptFile, fileName);
  Utils::open(&file, QIODevice::ReadOnly);
  runCommandFile(&file, args);
}

QStringList CommandWidget::history() const
{
  return commandLine->history();
}

void CommandWidget::onMenuRequested(const QPoint & pos)
{
  QPoint inner = terminalDisplay->mapFromParent(pos);
  if(terminalDisplay->rect().contains(inner)) {
    QMenu * menu = terminalDisplay->createStandardContextMenu(inner);
    QPoint glb = mapToGlobal(pos);
    menu->exec(glb);
  }
}

const QString & CommandWidget::scriptFileName() const
{
  return scriptFile;
}

const QString & CommandWidget::currentCommandLine() const
{
  return curCmdline;
}



void CommandWidget::runStartupFiles()
{
  QStringList sf = CommandWidget::startupFiles();
  for(int i = 0; i < sf.size(); i++) {
    Terminal::out << "Running startup file : " << sf[i] << endl;
    try {
      soas().prompt().runCommandFile(sf[i]);
    }
    catch(Exception & e) {
      Terminal::out << "There was a problem running " << sf[i] 
                    << ": " << e.message() << endl;
    }
  }
}
