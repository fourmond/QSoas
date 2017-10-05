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
#include <command.hh>
#include <group.hh>
#include <databackend.hh>

#include <debug.hh>

#include <settings.hh>
#include <soas.hh>
#include <exceptions.hh>

#include <commandlineparser.hh>

extern void loadDocumentationFile(const QString &, QString file, 
                                  const CommandOptions & opts = CommandOptions());
extern void updateDocumentationFile(const QString &, QString file);


class QSoasApplication : public QApplication {
public:
  bool notify(QObject * receiver, QEvent * event) 
  {
    // debug !
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


int main(int argc, char ** argv)
{
  DataBackend::registerBackendCommands();
  Command::crosslinkCommands();

  QSoasApplication main(argc, argv);
  main.setApplicationName("QSoas");

  // Ruby::initRuby();
  
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

  loadDocumentationFile("load-documentation", ":/doc/qsoas.kd");

  Soas theSoas;
  /// Has to be called
  // Ruby::initInterface();

  Settings::loadSettings("bip.cnrs-mrs.fr", "Soas");
  
  {
    MainWin win(&theSoas, startup);
    win.show();
    retval = main.exec();
  }

  Settings::saveSettings("bip.cnrs-mrs.fr", "Soas");
  return retval;
}

