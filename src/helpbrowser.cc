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

// A QTextBrowser subclass for handling the documents.
class HelpTextBrowser : public QTextBrowser {
  QHelpEngine * engine;
public:
  HelpTextBrowser(QHelpEngine * e) : engine(e) {
  };

  virtual QVariant loadResource(int type, const QUrl &name) override {
    QTextStream o(stdout);
    o << "Request: " << name.toString() << endl;
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

  theBrowser = this;
  setupFrame();
}

void HelpBrowser::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);

  browser = new HelpTextBrowser(engine);
  layout->addWidget(browser);
}




HelpBrowser::~HelpBrowser()
{
  delete engine;
}

void HelpBrowser::showLocation(const QString & location)
{
  // QString l("qthelp://qsoas.org.qsoas/doc/");
  // l += location;
  // browser->setSource(l);

  QTextStream o(stdout);
  // QUrl l = engine->findFile(location);
  // o << "Looking for " << location << " -> " << l.toString() << endl;

  // QStringList ls = engine->registeredDocumentations();
  // o << "Registered: " << endl;
  // for(const QString & l: ls) {
  //   o << " - " << l << " -> "
  //     << engine->documentationFileName(l) << endl;
  //   QList<QUrl> urls = engine->files(l, QStringList());
  //   for(const QUrl & u : urls)
  //     o << "    ->" << u.toString() << endl;
  // }

  // Find the namespace
  QStringList ls = engine->registeredDocumentations();
  if(ls.size() == 0)
    return;                     // Hmmm
  QString l = QString("qthelp://%1/%2").arg(ls[0]).arg(location); 

  QUrl url = engine->findFile(l);
  o << "Looking for " << location << "(" << l
    << ") -> " << url.toString() << endl;
  
  browser->setSource(url);
}

void HelpBrowser::browseLocation(const QString & location)
{
  HelpBrowser * b = getBrowser();
  b->show();
  b->showLocation(location);
}

HelpBrowser * HelpBrowser::getBrowser()
{
  if(!theBrowser)
    theBrowser = new HelpBrowser;
  return theBrowser;
}



//////////////////////////////////////////////////////////////////////
#include <command.hh>
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
