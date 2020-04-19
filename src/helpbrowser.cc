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

// A QTextBrowser subclass for handling the documents.
class HelpTextBrowser : public QTextBrowser {
  QHelpEngine * engine;
public:
  HelpTextBrowser(QHelpEngine * e) : engine(e) {
  };

  virtual QVariant loadResource(int type, const QUrl &name) override {
    // QTextStream o(stdout);
    // o << "Request: " << name.toString() << endl;
    if(name.scheme() == "qthelp") {
      return engine->fileData(name);
    }
    return QTextBrowser::loadResource(type, name);
  }
};


HelpBrowser * HelpBrowser::theBrowser = NULL;

HelpBrowser::HelpBrowser() : QWidget(NULL)
{
  QString collection =
    QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("doc/qsoas-help.qhc");
  QTextStream o(stdout);
  o << "Collection: " << collection << endl;
  engine = new QHelpEngine(collection, this);
  // engine->setupData();

  setWindowFlag(Qt::Window);

  theBrowser = this;
  setupFrame();
}

void HelpBrowser::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);

  QSplitter * mainS = new QSplitter(Qt::Horizontal);

  // QSplitter * subS = new QSplitter(Qt::Vertical);
  QWidget * left = new QWidget();
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

  browser = new HelpTextBrowser(engine);
  mainS->addWidget(browser);
  layout->addWidget(mainS);

  QDialogButtonBox * buttons =
    new QDialogButtonBox(QDialogButtonBox::Close,
                         Qt::Horizontal);
  layout->addWidget(buttons);

  connect(buttons, SIGNAL(accepted()), SLOT(hide()));
  connect(buttons, SIGNAL(rejected()), SLOT(hide()));

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



//////////////////////////////////////////////////////////////////////
#include <commandeffector-templates.hh>

static void htCommand(const QString &, const CommandOptions & )
{
  HelpBrowser::browseLocation("doc/qsoas.html#lkjsdf");
}


static Command 
dlwf("tst",  // command name
     effector(htCommand), // action
     "file",                       // group name
     NULL,                    // arguments
     NULL,                    // options
     "...");
