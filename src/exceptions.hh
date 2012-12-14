/**
   \file exceptions.hh
   Exceptions handling in Soas
   Copyright 2011 by Vincent Fourmond

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
public:
  Exception(const QString & msg) throw();
  virtual const char * what() const throw();
  virtual QString message() const throw();
  virtual ~Exception() throw() {;};

  /// Setup the qt message handler to catch problems
  static void setupQtMessageHandler();

  template<typename T> Exception & arg(T a) {
    msg.arg(a);
    return *this;
  }; 

};

class RuntimeError : public Exception {
public:
  RuntimeError(const QString & msg) throw() : Exception(msg) {
  };
  virtual ~RuntimeError() throw() {;};
};

class GSLError : public RuntimeError {
public:
  GSLError(const QString & msg) throw() : RuntimeError(msg) {
  };
  virtual ~GSLError() throw() {;};

  /// Setup the GSL handler to intercept GSL problems
  static void setupGSLHandler();
};


/// A specific error for "out of range" parameters in fits.
class RangeError : public RuntimeError {
public:
  RangeError(const QString & msg) throw() : RuntimeError(msg) {
  };
  virtual ~RangeError() throw() {;};

};

class InternalError : public Exception {
public:
  InternalError(const QString & msg) throw();
  virtual ~InternalError() throw() {;};
};

/// This exception in general isn't an error, but just an
/// implementation of control flow...
class ControlFlowException : public Exception {
public:
  ControlFlowException(const QString & msg) throw() : Exception(msg) {
  };
  virtual ~ControlFlowException() throw() {;};
};


#endif
