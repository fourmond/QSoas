/*
  helpbrowser.cc: browser for inline help
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
#include <helpbrowser.hh>
#include <command.hh>
#include <commandcontext.hh>

#include <actioncombo.hh>

#include <settings-templates.hh>



// A QTextBrowser subclass for handling the documents.
class HelpTextBrowser : public QTextBrowser {
  QHelpEngine * engine;
  bool doingResize;
public:
  HelpTextBrowser(QHelpEngine * e) : engine(e), doingResize(false) {
  };

  virtual QVariant loadResource(int type, const QUrl &name) override {
    // QTextStream o(stdout);
    // o << "Request: " << name.toString() << endl;
    if(name.scheme() == "qthelp") {
      return engine->fileData(name);
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

};

//////////////////////////////////////////////////////////////////////

static SettingsValue<QSize> helpWinSize("help/size", QSize(700,500));


HelpBrowser * HelpBrowser::theBrowser = NULL;

HelpBrowser::HelpBrowser() :
  QWidget(NULL), lastSearchFailed(false)
{
  QString collection =
    QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("doc/qsoas-help.qhc");
  QTextStream o(stdout);
  o << "Collection: " << collection << endl;
  engine = new QHelpEngine(collection, this);
  // engine->setupData();

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
  subS->addWidget(engine->contentWidget());
  connect(engine->contentWidget(),
          SIGNAL(linkActivated(const QUrl &)),
          SLOT(showLocation(const QUrl&)));
  
  subS->addWidget(new QLabel("Index"));
  subS->addWidget(engine->indexWidget());
  connect(engine->indexWidget(),
          SIGNAL(linkActivated(const QUrl &, const QString&)),
          SLOT(showLocation(const QUrl&)));

  mainS->addWidget(left);

  //////////////////////////////
  // Right part 
  QWidget * right = new QWidget();
  browser = new HelpTextBrowser(engine);

  // Navigation:
  subS = new QVBoxLayout(right);
  QHBoxLayout * top = new QHBoxLayout;
  QPushButton * bt =
    new QPushButton(qApp->style()->standardIcon(QStyle::SP_ArrowLeft),
                    QString());
  top->addWidget(bt);
  browser->connect(bt, SIGNAL(clicked()), SLOT(backward()));
  bt = new QPushButton(qApp->style()->standardIcon(QStyle::SP_ArrowRight),
                       QString());
  top->addWidget(bt);
  browser->connect(bt, SIGNAL(clicked()), SLOT(forward()));

  // Search box
  top->addWidget(new QLabel("Search: "));

  searchText = new QLineEdit;
  top->addWidget(searchText, 1);

  connect(searchText, SIGNAL(returnPressed()),
          SLOT(searchForward()));

  bt = new QPushButton(qApp->style()->standardIcon(QStyle::SP_ArrowDown),
                       QString());
  top->addWidget(bt);
  connect(bt, SIGNAL(clicked()), SLOT(searchForward()));

  bt = new QPushButton(qApp->style()->standardIcon(QStyle::SP_ArrowUp),
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




HelpBrowser::~HelpBrowser()
{
  delete engine;
}

void HelpBrowser::showLocation(const QUrl & url)
{
  browser->setSource(url);
}

void HelpBrowser::showLocation(const QString & location)
{
  // Find the namespace
  QStringList ls = engine->registeredDocumentations();
  if(ls.size() == 0)
    return;                     // Hmmm
  QString l = QString("qthelp://%1/%2").arg(ls[0]).arg(location); 

  QUrl url = engine->findFile(l);
  showLocation(url);
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
