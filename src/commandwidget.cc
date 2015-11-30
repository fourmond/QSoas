/*
  commandwidget.cc: Main window for QSoas
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014 by CNRS/AMU

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
#include <datastack.hh>

#include <settings-templates.hh>
#include <idioms.hh>

using namespace Terminal;

class SideBarLabel : public QScrollArea {
protected:

  QLabel * lbl;
public:
  SideBarLabel(QWidget * parent = NULL) : QScrollArea(parent)
  {
    lbl = new QLabel;
    setWidget(lbl);
    // Align the label on top, nicer.
    lbl->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    lbl->setMargin(4);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
  }

  void setText(const QString & s) {
    lbl->setText(s);
    lbl->resize(lbl->sizeHint());
    updateGeometry();
  }

  virtual QSize sizeHint() const {
    QSize sz = QScrollArea::sizeHint();
    QSize sz2 = lbl->sizeHint();
    QSize sz3 = verticalScrollBar()->sizeHint();
    sz2.setWidth(sz.width() + sz2.width() + sz3.width());
    return sz2;
  }
  

  
};

//////////////////////////////////////////////////////////////////////

/// @todo Replace with a QPointer to avoid sending stuff once it's
/// destroyed ?

QPointer<CommandWidget> CommandWidget::theCommandWidget(NULL);
static SettingsValue<QString> logFileName("command/logfile", 
                                          QString("soas.log"));

// If 0, do not rotate. If positive, rotate only that many files, if
// negative, rotate forever.
static SettingsValue<int> logRotateNumber("command/logrotate", 5);


static SettingsValue<QStringList> startupFiles("command/startup-files",
                                               QStringList());


static SettingsValue<QString> terminalFont("command/terminal-font",
                                           "Courier");

QStringList & CommandWidget::startupFiles()
{
  return ::startupFiles.ref();
}

CommandWidget::CommandWidget() : 
  watcherDevice(NULL),
  addToHistory(true)
{
  if(! theCommandWidget)
    theCommandWidget = this;    // Or always ?
  QVBoxLayout * layout = new QVBoxLayout(this);

  if(! ((QString)logFileName).isEmpty()) {
    /// @todo Find a writable place
    int rotation = logRotateNumber;
    if(rotation != 0) {
      QTextStream o(stdout);
      o << "Rotating file " << logFileName << endl;
      Utils::rotateFile(logFileName, rotation);
    }
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
  QFontMetrics mt(terminalDisplay->font());
  QSize sz = mt.size(0, "1.771771771766");
  terminalDisplay->setTabStopWidth(sz.width());
  
  // terminalDisplay->document()-> 
  //   setDefaultStyleSheet("p { white-space: pre; }");
  // Doesn't seem to have any effect...
  h1->addWidget(terminalDisplay);

  sideBarLabel = new SideBarLabel();
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

  restrictedPrompt = new LineEdit;
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

bool CommandWidget::runCommand(const QStringList & raw)
{
  /// @todo use a different prompt whether the call is internal or
  /// external ?

  bool status = true;
  if(raw.isEmpty())
    return true;                     // Nothing to do here.
  
  QString cmd = Command::unsplitWords(raw);
  out << bold("QSoas> ") << cmd << endl;
  
  if(addToHistory)
    commandLine->addHistoryItem(cmd);
  commandLine->setEnabled(false);

  soas().showMessage(tr("Running: %1").arg(cmd));
  try {
    TemporaryChange<QString> ch(curCmdline, raw.join(" "));
    soas().stack().startNewCommand();
    Command::runCommand(raw, this);
  }
  catch(const RuntimeError & error) {
    out << bold("Error: ") << error.message() << endl;
    status = false;
  }
  catch(const InternalError & error) {
    out << bold("Internal error: ") 
        << error.message() << endl
        << "This is a bug in Soas and may be worth reporting !" << endl;
    status = false;
  }
  catch(const ControlFlowException & flow) {
    /// @todo This should be implemented as an idiom when clang
    /// supports std::function
    commandLine->setEnabled(true);
    commandLine->setFocus();
    throw;                      // rethrow
  }
  commandLine->setEnabled(true);
  commandLine->setFocus();
  return status;
}

bool CommandWidget::runCommand(const QString & str)
{
  QStringList split = Command::wordSplit(str);
  return runCommand(split);
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

LineEdit * CommandWidget::enterPromptMode(const QString & prompt, 
                                           const QString & init)
{
  commandLine->setVisible(false);
  restrictedPrompt->setText(init);
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
  const DataSet * ds = soas().stack().currentDataSet(true);
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
                                   const QStringList & args,
                                   bool addToHist)
{
  QTextStream in(source);
  QRegExp commentRE("^\\s*#.*");

  // Disable history when applicable
  TemporaryChange<bool> ch(addToHistory, addToHist);
  try {
    while(true) {
      QString line = in.readLine();
      if(line.isNull())
        break;
      if(commentRE.indexIn(line) == 0)
        continue;

      // Argument substitution


      // Now, we look for all possible argument substitutions
      QRegExp substitutionRE("\\$\\{(\\d+)(?:(%%|##|:-|:\\+|\\?)([^}]*))?\\}");

      typedef enum {
        Plain, RemoveSuffix, RemovePrefix, DefaultValue, AlternateValue,
        TernaryValue
      } SubstitutionType;

      // We prepare the substitutions in advance in this hash:
      QHash<QString, QString> substitutions;

      int pos = 0;
      while((pos = substitutionRE.indexIn(line, pos)) != -1) {
        // We build one substitution
        QString key = substitutionRE.cap(0);
        int argn = substitutionRE.cap(1).toInt() - 1;
        QString w = substitutionRE.cap(2);
        SubstitutionType type = Plain;
        QString oa = substitutionRE.cap(3);
        QString subst;

        if(w == "%%")
          type = RemoveSuffix;
        else if(w == "##")
          type = RemovePrefix;
        else if(w == ":-")
          type = DefaultValue;
        else if(w == ":+")
          type = AlternateValue;
        else if(w == "?")
          type = TernaryValue;

        if(type != DefaultValue && type != AlternateValue && 
           type != TernaryValue && argn >= args.size()) {
          throw RuntimeError("Script was given %1 parameters, "
                             "but it needs at least %2, while parsing line '%3'").
            arg(args.size()).arg(argn+1).
            arg(line);
        }
        switch(type) {
        case RemoveSuffix:
          subst = args[argn];
          if(subst.endsWith(oa))
            subst.truncate(subst.size() - oa.size());
          break;
        case RemovePrefix:
          subst = args[argn];
          if(subst.startsWith(oa))
            subst = subst.mid(oa.size());
          break;
        case DefaultValue:
          if(argn >= args.size())
            subst = oa;
          else
            subst = args[argn];
          break;
        case AlternateValue:
          if(argn >= args.size())
            subst = "";
          else
            subst = oa;         // the value of the parameter is not used :
          break;
        case TernaryValue: {
          QStringList bits = oa.split(":");
          if(bits.size() != 2)
            throw RuntimeError("No ':' found in parameter ternary expansion: "
                               "'%1'").
            arg(oa);
          if(argn >= args.size())
            subst = bits[1];
          else
            subst = bits[0];         // the value of the parameter is not used
        }
          break;
        default:
        case Plain:
          subst = args[argn];
          break;
        }
        pos += substitutionRE.matchedLength();
        substitutions[key] = subst;
      }
      

      /// @todo escape ?
      for(QHash<QString, QString>::const_iterator i = substitutions.begin();
          i != substitutions.end(); i++)
        line.replace(i.key(), i.value());

      /// @todo Make that configurable
      if(! runCommand(line)) {
        Terminal::out << "Command failed: aborting script" << endl;
        break;
      }
      // And we allow for deferred slots to take place ?
      QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
  }
  catch(const ControlFlowException & flow) {
    // Nothing to do !
  }
}

void CommandWidget::runCommandFile(const QString & fileName, 
                                   const QStringList & args,
                                   bool addToHist)
{
  QFile file(fileName);
  Utils::open(&file, QIODevice::ReadOnly);
  TemporaryChange<QString> ch(scriptFile, fileName);
  runCommandFile(&file, args, addToHist);
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
