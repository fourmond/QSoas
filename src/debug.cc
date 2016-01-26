/*
  debug.cc: various utility functions for debugging
  Copyright 2011 by Vincent Fourmond
            2016 by CNRS/AMU

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
#include <debug.hh>

#include <utils.hh>
#include <exceptions.hh>
#include <soas.hh>
#include <datastack.hh>

void Debug::dumpCurrentFocus(const QString & str)
{
  QTextStream o(stdout);
  QWidget * w = QApplication::focusWidget();
  o << str << w << "(" 
    << (w ? w->metaObject()->className() : "-none-") << ")" << endl;
}

Debug::Debug() :
  level(0), directory(NULL), outputDevice(NULL), output(NULL),
  out(stdout)
{
  
}

Debug * Debug::theDebug = NULL;


Debug & Debug::debug()
{
  if(! theDebug)
    theDebug = new Debug;
  return *theDebug;
}

int Debug::debugLevel()
{
  return debug().level;
}


void Debug::closeDirectory()
{
  if(output) {
    delete output;
    output = NULL;
  }
  if(outputDevice) {
    delete outputDevice;
    outputDevice = NULL;
  }
  if(directory) {
    delete directory;
    directory = NULL;
  }
}


void Debug::openDirectory(const QString & dir)
{
  closeDirectory();
  if(! dir.isEmpty()) {
    directory = new QDir(dir);
    if(! directory->makeAbsolute()) {
      closeDirectory();
      throw RuntimeError("Could not transform '%1' into an absolute path").
        arg(dir);
    }
    QString fp = directory->absolutePath();
    if(! directory->exists(".")) {
      QDir rt = QDir::root();
      if(! rt.mkpath(fp)) {
        closeDirectory();
        throw RuntimeError("Could not create directory '%1'").
          arg(fp);
      }
    }
    else {
      QFileInfo info(fp);
      if(! info.isDir()) {
        closeDirectory();
        throw RuntimeError("'%1' exists and is not a directory").
          arg(fp);
      }
    }
  }

  outputDevice = new QFile(directory->absoluteFilePath("debug.log"));
  Utils::open(outputDevice, QIODevice::WriteOnly);
  /// @bug leaks memory
  output = new QTextStream(outputDevice);
  level = 1;
}



void Debug::startCommand(const QStringList & cmdline)
{
  if(level > 0 && directory) {
    timeStamp();
    (*this) << "Starting command: '" << cmdline.join("'")
            << "'\nRotating stack file" << endl;
    Utils::rotateFile(directory->absoluteFilePath("stack.bin"), 9);
  }
}

void Debug::endCommand(const QStringList & cmdline)
{
  if(level > 0 && directory) {
    timeStamp();
    (*this) << "Finishing command: '" << cmdline.join("'")
            << "'" << endl;
    saveStack();
  }
}

void Debug::saveStack()
{
  if(level > 0 && directory) {
    timeStamp();
    (*this) << "Saving stack" << endl;
    QFile file(directory->absoluteFilePath("stack.bin"));
    Utils::open(&file, QIODevice::WriteOnly);
    QDataStream out(&file);
    out << soas().stack();
  }
}


void Debug::timeStamp()
{
  QDateTime dt = QDateTime::currentDateTime();
  (*this) << "[" << dt.toString() << "] ";
}
