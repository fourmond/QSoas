/*
  helpbrowser.cc: browser for inline help
  Copyright 2020,2021 by CNRS/AMU

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
#include <helpbrowser.hh>
#include <command.hh>
#include <commandcontext.hh>

#include <terminal.hh>
#include <actioncombo.hh>
#include <utils.hh>
#include <settings-templates.hh>

#include <debug.hh>

#include <soas.hh>
#include <commandwidget.hh>


// A QTextBrowser subclass for handling the documents.
class HelpTextBrowser : public QTextBrowser {
  QHelpEngine * engine;
  bool doingResize;
public:
  HelpTextBrowser(QHelpEngine * e) : engine(e), doingResize(false) {
  };

  virtual QVariant loadResource(int type, const QUrl &name) override {
    // QTextStream o(stdout);
    if(Debug::debugLevel() > 0)
      Debug::debug() << "Request: " << name.toString() << endl;
    if(name.scheme() == "qthelp") {
      QByteArray data = engine->fileData(name);
      if(Debug::debugLevel() > 0)
        Debug::debug() << " -> " << data.size() << " bytes" << endl;
      return data;
    }
    return QTextBrowser::loadResource(type, name);
  }

  virtual void resizeEvent(QResizeEvent * event) override {
    // Now save the position
    if(doingResize) {
      QTextBrowser::resizeEvent(event);
      return;
    }
    doingResize = true;
    QTextCursor c = cursorForPosition(QPoint(0,0));
    QTextBrowser::resizeEvent(event);
    setTextCursor(c);
    doingResize = false;
  };

  

  void contextMenuEvent(QContextMenuEvent *event) override
  {
    QMenu *menu = new QMenu;

    QString sel = textCursor().selectedText();
    QString cmd;
    
    
    QTextCursor c = cursorForPosition(mapFromGlobal(event->globalPos()));
    QString link = c.charFormat().anchorHref();
    if(! link.startsWith("http"))
      link = "";
    c.select(QTextCursor::LineUnderCursor);
    QString line = c.selectedText().trimmed();

    QRegExp re("^QSoas(\\..*)?>\\s*(.+)");


    if(re.indexIn(line, 0) == 0)
      cmd = re.cap(2);

    // Commands in the selection
    QStringList selCmds;
    int idx = 0;
    // QTextStream o(stdout);
    // o << "Sel: " << sel.size() << " ->  '" << sel << "'" << endl;
    // for(QChar c : sel)
    //   o << "Ch: " << c << " -> " << c.unicode() << " -- "
    //     << (int) c.toLatin1() << endl;

    // Unicode x2029 is the "new paragraph" code point.

    for(const QString & l : sel.split(QRegExp("\\r|\\n|\\x2029"))) {
      // o << "ln: '" << l << "'" << endl;
      if(re.indexIn(l) >= 0)
        selCmds << re.cap(2);
    }
    

    QAction * action = new QAction("Back");
    if(! isBackwardAvailable())
      action->setEnabled(false);
    else
      connect(action, SIGNAL(triggered()), SLOT(backward()));
    menu->addAction(action);

    action = new QAction("Forward");
    if(! isForwardAvailable())
      action->setEnabled(false);
    else
      connect(action, SIGNAL(triggered()), SLOT(forward()));

    menu->addAction(action);
    menu->addSeparator();



    menu->addSection("Selection");

    action = new QAction("Copy");
    if(sel.isEmpty())
      action->setEnabled(false);
    else
      QObject::connect(action, &QAction::triggered, this, [sel]() {
          QApplication::clipboard()->setText(sel);
        }
        );
    menu->addAction(action);

    action = new QAction("Copy to prompt");
    if(sel.isEmpty())
      action->setEnabled(false);
    else
      QObject::connect(action, &QAction::triggered, this, [sel]() {
          soas().prompt().copyToPrompt(sel);
        }
        );
    menu->addAction(action);

    action = new QAction("Run selection");
    if(sel.isEmpty())
      action->setEnabled(false);
    else
      QObject::connect(action, &QAction::triggered, this,
                       [sel]() {
                         soas().prompt().runCommand(sel);
                       }
                       );
    menu->addAction(action);

    
    action = new QAction(QString("Run %1 commands").arg(selCmds.size()));
    if(selCmds.isEmpty())
      action->setEnabled(false);
    else 
      QObject::connect(action, &QAction::triggered, this,
                       [selCmds]() {
                         for(const QString & cmd : selCmds)
                           soas().prompt().runCommand(cmd);
                       }
                       );
    menu->addAction(action);


    menu->addSection("Command from line");

    action = new QAction("Copy command");
    if(cmd.isEmpty())
      action->setEnabled(false);
    else
      QObject::connect(action, &QAction::triggered, this, [cmd]() {
          QApplication::clipboard()->setText(cmd);
        }
        );
    menu->addAction(action);

        action = new QAction("Copy command to prompt");
    if(cmd.isEmpty())
      action->setEnabled(false);
    else
      QObject::connect(action, &QAction::triggered, this, [cmd]() {
          soas().prompt().copyToPrompt(cmd);
        }
        );
    menu->addAction(action);

    action = new QAction("Run command");
    if(cmd.isEmpty())
      action->setEnabled(false);
    else
      QObject::connect(action, &QAction::triggered, this, [cmd]() {
          soas().prompt().runCommand(cmd);
        }
        );
    menu->addAction(action);


    menu->addSeparator();

    action = new QAction("Copy Link Location");
    if(link.isEmpty())
      action->setEnabled(false);
    else
      QObject::connect(action, &QAction::triggered, this, [link]() {
          QApplication::clipboard()->setText(link);
        }
        );
    menu->addAction(action);





    
    menu->exec(event->globalPos());
    delete menu;
  }
};

//////////////////////////////////////////////////////////////////////

static SettingsValue<QSize> helpWinSize("help/size", QSize(700,500),
                                        "size of the help window");


HelpBrowser * HelpBrowser::theBrowser = NULL;

QHelpEngine * HelpBrowser::theEngine = NULL;

QHelpEngine * HelpBrowser::getEngine()
{
  if(! theEngine) {
    // We search in various locations
    QStringList docPaths;
    docPaths << QDir(QCoreApplication::applicationDirPath()).
      absoluteFilePath("doc");

#ifdef Q_OS_WIN32
    {
      QSettings settings("HKEY_CURRENT_USER\\Software\\qsoas.org\\QSoas",
                         QSettings::NativeFormat);
      QString v = settings.value("DocDir").toString();
      if(! v.isEmpty())
        docPaths << v;
    }
#endif
#ifdef Q_OS_MAC
    docPaths << QDir(QCoreApplication::applicationDirPath()).
      absoluteFilePath("../Resources");
#endif
  
    QString collection;
    for(const QString & d : docPaths) {
      QDir dir(d);
      if(dir.exists("qsoas-help.qhc")) {
        collection = dir.absoluteFilePath("qsoas-help.qhc");
        break;
      }
    }     
  
    QTextStream o(stdout);
    o << "Collection: '" << collection << "'" << endl;
    theEngine = new QHelpEngine(collection);
    if(collection.isEmpty())
      o << "Could not find collection file, search paths used: '"
        << docPaths.join("', '") << "'" << endl;
    // engine->setupData();
  }
  return theEngine;
}

QString HelpBrowser::collectionFile()
{
  return getEngine()->collectionFile();
}

void HelpBrowser::dumpHelp()
{
  QHelpEngine * engine = getEngine();
  Terminal::out << "Collection file: " << engine->collectionFile() << endl;
  QFileInfo info(engine->collectionFile());
  Terminal::out << " -> " << info.absoluteFilePath() << " ("
                << info.size() << " -- "
                << info.lastModified().toString() << ")" << endl;

  Terminal::out << "Filters: '" << engine->customFilters().join("', '")
                << "'\nCurrent filter:" << engine->currentFilter() << endl;

  
  for(const QString & s : engine->registeredDocumentations()) {
    Terminal::out << " * " << s
                  << "\n    -> " << engine->documentationFileName(s) << endl;
    QFileInfo info(engine->documentationFileName(s));
    Terminal::out << "    -> " << info.absoluteFilePath() << " ("
                  << info.size() << " -- "
                  << info.lastModified().toString() << ")" << endl;
    for(const QUrl & f : engine->files(s, QStringList()))
      Terminal::out << " |- " << f.toString() << endl;
  }
  Terminal::out << "Last error was: " << engine->error() << endl;
}

HelpBrowser::HelpBrowser() :
  QWidget(NULL), lastSearchFailed(false)
{

  // setWindowFlags(Qt::Window);
  setWindowTitle("QSoas Help");

  theBrowser = this;
  setupFrame();
  resize(::helpWinSize);
}

void HelpBrowser::resizeEvent(QResizeEvent * event)
{
  QWidget::resizeEvent(event);
  helpWinSize = size();
}


void HelpBrowser::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);

  QSplitter * mainS = new QSplitter(Qt::Horizontal);

  // QSplitter * subS = new QSplitter(Qt::Vertical);
  QWidget * left = new QWidget();

  //////////////////////////////
  // Left part 
  QVBoxLayout * subS = new QVBoxLayout(left);

  subS->addWidget(new QLabel("Contents"));
  subS->addWidget(getEngine()->contentWidget());
  connect(getEngine()->contentWidget(),
          SIGNAL(linkActivated(const QUrl &)),
          SLOT(showLocation(const QUrl&)));
  
  subS->addWidget(new QLabel("Index"));
  subS->addWidget(getEngine()->indexWidget());
  connect(getEngine()->indexWidget(),
          SIGNAL(linkActivated(const QUrl &, const QString&)),
          SLOT(showLocation(const QUrl&)));

  mainS->addWidget(left);

  //////////////////////////////
  // Right part 
  QWidget * right = new QWidget();
  browser = new HelpTextBrowser(getEngine());

  // Navigation:
  subS = new QVBoxLayout(right);
  QHBoxLayout * top = new QHBoxLayout;
  QPushButton * bt =
    new QPushButton(Utils::standardIcon(QStyle::SP_ArrowLeft),
                    QString());
  top->addWidget(bt);
  browser->connect(bt, SIGNAL(clicked()), SLOT(backward()));
  bt = new QPushButton(Utils::standardIcon(QStyle::SP_ArrowRight),
                       QString());
  top->addWidget(bt);
  browser->connect(bt, SIGNAL(clicked()), SLOT(forward()));

  // Search box
  top->addWidget(new QLabel("Search: "));

  searchText = new QLineEdit;
  top->addWidget(searchText, 1);

  connect(searchText, SIGNAL(returnPressed()),
          SLOT(searchForward()));

  bt = new QPushButton(Utils::standardIcon(QStyle::SP_ArrowDown),
                       QString());
  top->addWidget(bt);
  connect(bt, SIGNAL(clicked()), SLOT(searchForward()));

  bt = new QPushButton(Utils::standardIcon(QStyle::SP_ArrowUp),
                       QString());
  top->addWidget(bt);
  connect(bt, SIGNAL(clicked()), SLOT(searchBackward()));

  subS->addLayout(top);
  subS->addWidget(browser);
  
  mainS->addWidget(right);

  mainS->setStretchFactor(0, 1);
  mainS->setStretchFactor(1, 3);
  
  layout->addWidget(mainS);

  QDialogButtonBox * buttons =
    new QDialogButtonBox(QDialogButtonBox::Close,
                         Qt::Horizontal);
  layout->addWidget(buttons);

  connect(buttons, SIGNAL(accepted()), SLOT(hide()));
  connect(buttons, SIGNAL(rejected()), SLOT(hide()));

  browser->setOpenLinks(false);
  connect(browser, SIGNAL(anchorClicked(const QUrl&)),
          SLOT(linkClicked(const QUrl&)));

  // Keyboard shortcut:

  addAction(ActionCombo::createAction("Search", this,
                                      SLOT(searchForwardShortcut()),
                                      QKeySequence("Ctrl+F"),
                                      this));
  addAction(ActionCombo::createAction("Search backward", this,
                                      SLOT(searchBackwardShortcut()),
                                      QKeySequence("Ctrl+Shift+F"),
                                      this));
}



QUrl HelpBrowser::urlForFile(const QString & file)
{
  // Find the namespace
  QStringList ls = getEngine()->registeredDocumentations();
  if(ls.size() == 0)
    return QUrl();                     // Hmmm
  QString l = QString("qthelp://%1/%2").arg(ls[0]).arg(file); 

  return getEngine()->findFile(l);
}



void HelpBrowser::showLocation(const QUrl & url)
{
  browser->setSource(url);
}

void HelpBrowser::showLocation(const QString & location)
{
  showLocation(urlForFile(location));
}

void HelpBrowser::browseLocation(const QString & location)
{
  HelpBrowser * b = getBrowser();
  b->setVisible(true);
  b->showLocation(location);
}

HelpBrowser * HelpBrowser::getBrowser()
{
  if(!theBrowser)
    theBrowser = new HelpBrowser;
  return theBrowser;
}

void HelpBrowser::browseCommand(const Command * command)
{
  QString id = "cmd-" + command->commandName();
  if(command->commandContext() != CommandContext::globalContext())
    id = "fit-" + id;
  browseLocation("doc/qsoas.html#"+id);
}

void HelpBrowser::linkClicked(const QUrl & url)
{
  QTextStream o(stdout);
  o << "Link: " << url.toString() << endl;
  if(url.scheme() == "qthelp")  // internal
    showLocation(url);
  else
    QDesktopServices::openUrl(url);
}

void HelpBrowser::search(const QTextDocument::FindFlags & flgs)
{
  QString txt = searchText->text();
  if(! txt.isEmpty())
    browser->find(txt, flgs);
}

void HelpBrowser::searchForward()
{
  search();
}

void HelpBrowser::searchBackward()
{
  search(QTextDocument::FindBackward);
}

void HelpBrowser::searchForwardShortcut()
{
  if(searchText->hasFocus())
    searchForward();
  else
    searchText->setFocus();
}

void HelpBrowser::searchBackwardShortcut()
{
  if(searchText->hasFocus())
    searchBackward();
  else
    searchText->setFocus();
}

//////////////////////////////////////////////////////////////////////

static SettingsValue<bool> showTips("tips/show-at-startup", true,
                                    "whether to show the tips at startup or not");
static SettingsValue<QSize> tipsWinSize("tips/size", QSize(700,500),
                                        "size of the tips window");

class Tip {
public:
  QString id;
  QStringList keywords;

  Tip() {;};
  Tip(const QString & i) : id(i) {;};
};

TipsDisplay * TipsDisplay::theDisplay = NULL;


TipsDisplay * TipsDisplay::getDisplay(bool activate)
{
  if(!theDisplay)
    theDisplay = new TipsDisplay;
  if(activate)
    theDisplay->setVisible(true);
  return theDisplay;
}

QHelpEngine * TipsDisplay::getEngine()
{
  return HelpBrowser::getEngine();
}

TipsDisplay::TipsDisplay()
{
  setWindowTitle("QSoas Tips");
  setupFrame();
  resize(::tipsWinSize);
  setShowStartup(false);
}

TipsDisplay::~TipsDisplay()
{
  delete browser;
}

void TipsDisplay::setupFrame()
{

  QVBoxLayout * layout = new QVBoxLayout(this);
  browser = new HelpTextBrowser(getEngine());
  layout->addWidget(browser);

  browser->setOpenLinks(false);
  connect(browser, SIGNAL(anchorClicked(const QUrl&)),
          SLOT(linkClicked(const QUrl&)));

  keywordLine = new QLabel;
  layout->addWidget(keywordLine);
  // The links are links to keywords
  connect(keywordLine, SIGNAL(linkActivated(const QString &)),
          SLOT(showRandomKeyword(const QString &)));

  dontShowAgain = new QCheckBox("Don't show tips again");
  layout->addWidget(dontShowAgain);
  
  
  QHBoxLayout * bottom = new QHBoxLayout;
  bottom->insertStretch(1);
  QPushButton * bt = new QPushButton("Show random");
  bottom->addWidget(bt);
  connect(bt, SIGNAL(clicked()), SLOT(showRandomTip()));
  
  bt =
    new QPushButton(Utils::standardIcon(QStyle::SP_DialogCloseButton),
                    "Close");
  bottom->addWidget(bt);
  connect(bt, SIGNAL(clicked()), SLOT(doHide()));

  layout->addLayout(bottom);
}


QHash<QString, Tip*> * TipsDisplay::tips = NULL;
QHash<QString, QList<Tip*> > * TipsDisplay::tipsByKeyword = NULL;

void TipsDisplay::readTips()
{
  if(tips)
    return;                     // not reading twice.
  tips = new QHash<QString, Tip*>;
  tipsByKeyword = new QHash<QString, QList<Tip*> >;
   QTextStream o(stdout);
  QByteArray data = getEngine()->
    fileData(HelpBrowser::urlForFile("doc/tips.txt"));
  QStringList tps = QString(data).split('\n');
  QRegExp re(":\\s*");
  for(const QString & t : tps) {
    QStringList l2 = t.split(re);
    // o << "Line: " << l2.join(", ") << l2.size() << endl;
    if(l2.size() == 2) {
      Tip * tip = new Tip(l2[0]);
      (*tips)[tip->id] = tip;
      tip->keywords = l2[1].split(';');
      for(const QString & k : tip->keywords) {
        if(! tipsByKeyword->contains(k))
          (*tipsByKeyword)[k] = QList<Tip*>();
        (*tipsByKeyword)[k] << tip;
      }
    }
  }
  o << "Read " << tips->size() << " tips" << endl;
}

void TipsDisplay::showTip(const Tip * tip)
{
  QTextStream o(stdout);
  o << "Showing tip: " << tip->id << endl;
  QString lbl = "This tip is about: ";
  for(const QString & kw : tip->keywords)
    lbl += QString("<a href='%1'>%1</a> ").
      arg(kw);
  keywordLine->setText(lbl);
  browser->setSource(HelpBrowser::urlForFile(QString("doc/tips-%1.html").
                                             arg(tip->id)));
}

void TipsDisplay::showStartupTips()
{
  if(::showTips) {
    getDisplay()->setShowStartup(true);
    getDisplay(true)->showRandomTip();
  }
  else
    Terminal::out << "Not showing startup tips, but you can see them with the tips command" << endl;
}

void TipsDisplay::doHide()
{
  ::tipsWinSize = size();
  if(dontShowAgain->isVisible()) {
    if(dontShowAgain->checkState() == Qt::Checked) {
      ::showTips = false;
      Terminal::out << "Not showing startup tips anymore, but you can reactivate them with tips /show-at-startup=true " << endl;
    }
    setShowStartup(false);      // not needed anymore.
  }
  hide();
}

void TipsDisplay::setShowStartup(bool show)
{
  dontShowAgain->setVisible(show);
}

void TipsDisplay::showRandomTip()
{
  if(! tips)
    readTips();
  QList<Tip*> tps = tips->values();
  if(tps.size() > 0)
    showTip(tps[::rand() % tps.size()]);
}

void TipsDisplay::showRandomKeyword(const QString & keyword)
{
  if(! tips)
    readTips();
  QList<Tip*> tps = (*tipsByKeyword)[keyword];
  if(tps.size() > 0)
    showTip(tps[::rand() % tps.size()]);
}

void TipsDisplay::linkClicked(const QUrl & url)
{
  QTextStream o(stdout);
  o << "Tips link: " << url.toString() << endl;
  if(url.scheme() == "qthelp") { // internal
    HelpBrowser * b = HelpBrowser::getBrowser();
    b->showLocation(url);
    b->setVisible(true);
  }
  else
    QDesktopServices::openUrl(url);
}

//////////////////////////////////////////////////////////////////////

#include <general-arguments.hh>
#include <commandeffector-templates.hh>


static void tipsCommand(const QString & /*name*/,
                        const CommandOptions & opts)
{
  if(opts.contains("show-at-startup")) {
    bool s;
    updateFromOptions(opts, "show-at-startup", s);
    ::showTips = s;
  }
  else
    TipsDisplay::getDisplay(true)->showRandomTip();
}


static ArgumentList 
tipsO(QList<Argument *>()
      << new BoolArgument("show-at-startup",
                          "Turns on and off the display of tips at startup")
      );


static Command 
hlpc("tips", // command name
     effector(tipsCommand), // action
     "help",  // group name
     NULL, // arguments
     &tipsO, // options
     "Tips");

