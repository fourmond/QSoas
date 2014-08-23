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

#include <ruby.hh>

#include <settings.hh>
#include <soas.hh>
#include <exceptions.hh>

extern void loadDocumentationFile(const QString &, QString file, 
                                  const CommandOptions & opts = CommandOptions());
extern void updateDocumentationFile(const QString &, QString file);


class QSoasApplication : public QApplication {
public:
  bool notify(QObject * receiver, QEvent * event) 
  {
    try {
      return QApplication::notify(receiver, event);
    } 
    catch(Exception & e) {
      QTextStream o(stdout);
      o << "Exception thrown from an event handler:" << e.message() 
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
  if(QString(argv[1]) == "--spec") {
    QTextStream o(stdout);
    Command::writeSpecFile(o);
    return 0;
  }

  QSoasApplication main(argc, argv);
  main.setApplicationName("QSoas");

  Ruby::initRuby();
  
  // We convert GSL's hard errors into C++ exceptions
  GSLError::setupGSLHandler();


  QString arg1;
  if(argc > 1)
    arg1 = argv[1];

  // And we do the same for Qt's hard errors, but only if no --debug
  // flag is given
  if(arg1 != "--debug")
    Exception::setupQtMessageHandler();

  /// @todo This is a rudimentary command-line parsing, but it does
  /// the job -- for the time being ;-)...
  if(arg1 == "--tex-help") {
    QTextStream o(stdout);
    o << Group::latexDocumentationAllGroups() << endl;
    return 0;
  }

  if(arg1 == "--update-documentation") {
    updateDocumentationFile("ud", argv[2]);
    return 0;
  }

  bool startup = true;
  if(arg1 == "--no-startup-files")
    startup = false;

  int retval;

  loadDocumentationFile("load-documentation", ":/doc/qsoas.kd");

  Soas theSoas;

  Settings::loadSettings("bip.cnrs-mrs.fr", "Soas");
  
  {
    MainWin win(&theSoas, startup);
    win.show();
    retval = main.exec();
  }

  Settings::saveSettings("bip.cnrs-mrs.fr", "Soas");
  return retval;
}

