/**
   \file exceptions.hh
   Exceptions handling in Soas
   Copyright 2011 by Vincent Fourmond
             2012, 2013 by CNRS/AMU

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
#ifndef __EXCEPTIONS_HH
#define __EXCEPTIONS_HH

class Exception : public std::exception {
protected:
  QString msg;
  QStringList backtrace;
  QByteArray full;

  /// Updates full, the cache of the message 
  void updateCache();
public:
  explicit Exception(const QString & msg) throw();
  virtual const char * what() const throw();
  virtual QString message() const throw();
  virtual ~Exception() throw() {;};

  /// Adds a message to the current message
  virtual void appendMessage(const QString & msg);

  /// Setup the qt message handler to catch problems
  static void setupQtMessageHandler();

  /// Returns the backtrace
  const QStringList & exceptionBacktrace() const throw() {
    return backtrace;
  };

};

class RuntimeError : public Exception {
public:
  explicit RuntimeError(const QString & msg) throw() : Exception(msg) {
  };
  virtual ~RuntimeError() throw() {;};

  // we need a redefition to avoid throwing Exception rather than
  template<typename T> RuntimeError & arg(T a) {
    msg = msg.arg(a);
    updateCache();
    return *this;
  }; 

};


class GSLError : public RuntimeError {
public:
  explicit GSLError(const QString & msg) throw() : RuntimeError(msg) {
  };
  virtual ~GSLError() throw() {;};

  /// Setup the GSL handler to intercept GSL problems
  static void setupGSLHandler();
};


/// A specific error for "out of range" parameters in fits.
class RangeError : public RuntimeError {
public:
  explicit RangeError(const QString & msg) throw() : RuntimeError(msg) {
  };
  virtual ~RangeError() throw() {;};

  // we need a redefition to avoid throwing Exception rather than
  template<typename T> RangeError & arg(T a) {
    msg = msg.arg(a);
    updateCache();
    return *this;
  }; 

};

class InternalError : public Exception {
public:
  explicit InternalError(const QString & msg) throw();
  virtual ~InternalError() throw() {;};
  virtual QString message() const throw() override;

  template<typename T> InternalError & arg(T a) {
    msg = msg.arg(a);
    updateCache();
    return *this;
  }; 

};

/// Exception raised specifically when used in headless mode and
/// shouldn't be
class HeadlessError : public Exception {
public:
  explicit HeadlessError(const QString & msg) throw() : Exception(msg) {
  };
  virtual ~HeadlessError() throw() {;};

  // we need a redefition to avoid throwing Exception rather than
  template<typename T> HeadlessError & arg(T a) {
    msg = msg.arg(a);
    updateCache();
    return *this;
  }; 

};

#define NOT_IMPLEMENTED InternalError("Function %1 is not implemented").arg(Q_FUNC_INFO)

/// This exception in general isn't an error, but just an
/// implementation of control flow...
class ControlFlowException : public Exception {
public:
  explicit ControlFlowException(const QString & msg) throw() :
    Exception(msg) {
  };
  virtual ~ControlFlowException() throw() {;};
};


/// This exception is a pendant to the ControlFlowException and allows
/// one to break out of the current stack of commands back to the main
/// QSoas command-line.
///
/// @question should we add the interrupted commands ?
class CommandInterruptException : public Exception {
public:
  explicit CommandInterruptException(const QString & msg) throw() :
    Exception(msg) {
  };
  virtual ~CommandInterruptException() throw() {;};
};

#endif
