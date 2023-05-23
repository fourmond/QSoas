/**
   \file qmain.cc
   entry point of QSoas
   Copyright 2011 by Vincent Fourmond
             2012, 2013 by CNRS/AMU

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.
*/

#include <headers.hh>

#include <mainwin.hh>
#include <commandcontext.hh>

#include <databackend.hh>
#include <fit.hh>

#include <debug.hh>

#include <settings.hh>
#include <soas.hh>
#include <exceptions.hh>

#include <commandlineparser.hh>


extern void updateDocumentationFile(const QString &, QString file);


class QSoasApplication : public QApplication {
public:
  bool notify(QObject * receiver, QEvent * event) 
  {
    // debug !
    // QTextStream o(stdout);
    // o << "notify: " << receiver << " -- "
    //   << receiver->metaObject()->className()
    //   << " -> event :" << event->type() << endl;
    
    try {
      // We override keyboard shortcuts within QLineEdit, those are
      // rather painful.
      if(event->type() == QEvent::ShortcutOverride &&
         dynamic_cast<QLineEdit*>(receiver)) {
        event->ignore();
        return false;
      }
      return QApplication::notify(receiver, event);
    } 
    catch(Exception & e) {
      Debug::debug()
        << "Exception thrown from an event handler:" << e.message() 
        << "\nBacktrace: " << e.exceptionBacktrace().join("\n\t") 
        << endl;
    }
    return false;
  }
  
  QSoasApplication(int & argc, char ** argv) : QApplication(argc, argv) {
  };
};

#ifdef Q_OS_MAC

#include <CoreFoundation/CoreFoundation.h>

// A function to locate the application bundle on MacOS
QString findApplicationPath()
{
  QString s;
  CFURLRef appUrlRef = CFBundleCopyExecutableURL(CFBundleGetMainBundle());
  CFStringRef macPath = CFURLCopyFileSystemPath(appUrlRef, kCFURLPOSIXPathStyle);
  const char *pathPtr = CFStringGetCStringPtr(macPath, kCFStringEncodingUTF8);

  return pathPtr;
}

#endif 
int main(int argc, char ** argv)
{
  const char * qsdebug = getenv("QSOAS_DEBUG");
  if(qsdebug) {
    int level = atoi(qsdebug);
    if(level >= 0)
      Debug::setDebugLevel(level);
  }
  DataBackend::registerBackendCommands();
  CommandContext::crosslinkAllCommands();

  // OK, this is ugly, we need to make sure the directory containing
  // the bundle on mac is loaded as priority, but unfortunately, we can't use
  // QCoreApplication::applicationDirPath() because that only works
  // *after* the QCoreApplication object is created...
  //
  // ... which is way too late...
#ifdef Q_OS_MAC
  { 
    QString path(::findApplicationPath());
    QTextStream o(stdout);
    o << "Found EXE path: " << path << endl;
    QFileInfo info(path);
    QCoreApplication::addLibraryPath(info.absoluteDir().path());
    o << "Set library paths:\n - " << QCoreApplication::libraryPaths().join("\n - ") << endl;
  }
#endif

  QSoasApplication main(argc, argv);
  
  main.setApplicationName("QSoas");

  
  // We convert GSL's hard errors into C++ exceptions
  GSLError::setupGSLHandler();
  Exception::setupQtMessageHandler();

  bool startup = true;
  CommandLineOption hlp("--no-startup-files", [&startup](const QStringList & /*args*/) {
      startup = false;
    }, 0, "disable the load of startup files");

  try {
    CommandLineParser::parseCommandLine();
  }
  catch (const RuntimeError & e) {
    QTextStream o(stderr);
    o << "Failed processing the command-line: " << e.message() << endl;
    return 1;
  }

  int retval;

  // loadDocumentationFile("load-documentation", ":/doc/qsoas.kd");

  Soas theSoas;
  /// Has to be called
  // Ruby::initInterface();

  // Why on earth do we still call that soas ?
  Settings::loadSettings("bip.cnrs-mrs.fr", "Soas");
  Settings::loadSettings("qsoas.org", "QSoas");

  try {
    MainWin win(&theSoas, startup);
    win.show();
    retval = main.exec();
  }
  catch(const HeadlessError & e) {
    QTextStream o(stderr);
    o << "Headless mode run failed: " << e.message() << endl;
    return 5;
  }

  Settings::saveSettings("qsoas.org", "QSoas");
  /// @todo This should probably join Soas's destructor ?
  Fit::clearupCustomFits();
  DataBackend::cleanupBackends();
  return retval;
}

