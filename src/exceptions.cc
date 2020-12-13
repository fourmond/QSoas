/*
  exceptions.cc: exceptions definitions
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
#include <exceptions.hh>

#include <terminal.hh>
#include <utils.hh>

Exception::Exception(const QString & m) throw() : 
  msg(m) {
  backtrace = Utils::backtrace();
  updateCache();
}


void Exception::updateCache()
{
  full = (msg + "\n" + backtrace.join("\n")).toLocal8Bit();
}

const char * Exception::what() const throw()
{
  return full.constData();
}

QString Exception::message() const throw()
{
  return msg;
}

void Exception::appendMessage(const QString & s)
{
  msg += s;
  updateCache();
}

// for now, we brutally remove message handling in Qt5
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
static void qtMessageHandler(QtMsgType type, const char *msg)
{
  switch (type) {
  case QtDebugMsg:
    fprintf(stderr, "Debug: %s\n", msg);
    break;
  case QtWarningMsg:
    fprintf(stderr, "Warning: %s\n", msg);
    break;
  case QtCriticalMsg:
    fprintf(stderr, "Critical: %s\n", msg);
    Terminal::out << "Critical error: " << msg << endl;
    break;
  case QtFatalMsg:
    throw InternalError(QString("Fatal error: %1").arg(msg));
  }
}

#else

void qtMessageHandler(QtMsgType type,
                      const QMessageLogContext &context,
                      const QString &msg)
{
  QString m = QString("%1 (%2:%3, %4)").arg(msg).
    arg(context.file).arg(context.line).arg(context.function);
  QByteArray bt = m.toLocal8Bit();
  switch (type) {
  case QtDebugMsg:
    fprintf(stderr, "Debug: %s\n", bt.constData());
    break;
  case QtInfoMsg:
    fprintf(stderr, "Info: %s\n", bt.constData());
    break;
  case QtWarningMsg:
    fprintf(stderr, "Warning: %s\n", bt.constData());
    break;
  case QtCriticalMsg:
    fprintf(stderr, "Critical: %s\n", bt.constData());
    Terminal::out << "Critical error: " << m << endl;
    break;
  case QtFatalMsg:
    throw InternalError(QString("Fatal error: %1").arg(m));
    break;
  }
}

#endif

void Exception::setupQtMessageHandler()
{
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
  qInstallMsgHandler(&qtMessageHandler);
#else
  qInstallMessageHandler(&qtMessageHandler);
#endif
}

//////////////////////////////////////////////////////////////////////

static void my_error_handler(const char * reason,
                             const char * file,
                             int line,
                             int gsl_errno)
{
  QString error = QString("GSL error: %1 (in %2:%3) -- error code %4").
    arg(reason).arg(file).arg(line).arg(gsl_errno);
  QString fl = file;
  // These errors are necessarily internal errors, while other GSL
  // errors could be legitimate user errors
  if(fl.contains("vector") || fl.contains("matrix"))
    throw InternalError(error);
  throw GSLError(error);
}

void GSLError::setupGSLHandler()
{
  gsl_set_error_handler(&my_error_handler);
}

//////////////////////////////////////////////////////////////////////

InternalError::InternalError(const QString & ms) throw() :
  Exception(ms)
{
}


QString InternalError::message() const throw()
{
  if(backtrace.size() > 0)
    return msg + "\nBacktrace:\n\t" + backtrace.join("\n\t");
  else
    return msg + "[no backtrace available]";
}
