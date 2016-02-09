/**
   \file ruby.hh
   Utility functions to communicate with Ruby
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
#ifndef __RUBY_HH
#define __RUBY_HH

#include <ruby-wrappers.h>

/// Namespace holding various utility functions to run embedded Ruby
namespace Ruby {

  /// Inits the Ruby interpreter. Must be called <b>only once ! </b>
  void initRuby();

  /// Prepares the interface between Ruby and QSoas.
  ///
  /// The interface implementation sits in another file.
  void initInterface();

  /// Calls the given C \a function with the given args in an
  /// exception-safe way, ie with proper handling of the exceptions...
  RUBY_VALUE exceptionSafeCall(RUBY_VALUE (*function)(...), void * args);

  /// The global rescue function, whose role is to convert Ruby
  /// exceptions into proper C++ exceptions.
  RUBY_VALUE globalRescueFunction(RUBY_VALUE, RUBY_VALUE exception);

  /// Evaluates the given string, while trying to avoid segfaults.
  RUBY_VALUE eval(QByteArray code);

  /// Evaluates the given code, with appropriate code error handling.
  RUBY_VALUE safeEval(const QString & s);

  /// Makes a block from this code using the given variables.
  ///
  /// The \a variables gets updated with the actual variables that
  /// were necessary to compile the block
  RUBY_VALUE makeBlock(QStringList * variables, const QByteArray & code);

  /// Read a file and execute it
  RUBY_VALUE loadFile(const QString & file);

  /// Converts to a double in a Ruby-exception safe way: Ruby
  /// exceptions will result in proper C++ exceptions.
  double toDouble(RUBY_VALUE val);

  /// Converts to an int in a Ruby-exception safe way: Ruby
  /// exceptions will result in proper C++ exceptions.
  long toInt(RUBY_VALUE val);


  /// Makes a textual representation of any Ruby object (kinda rp_p)
  ///
  /// This function is already ruby-exception safe, ie no need to wrap
  /// it in a Ruby::run() call.
  QString inspect(RUBY_VALUE val);

  /// Creates a Ruby string from a QString. No exception handling, but
  /// it should never fail !
  RUBY_VALUE fromQString(const QString & str);

  /// Transforms the Ruby value into a string
  QString toQString(RUBY_VALUE val);

  /// Returns the version string of Ruby
  QString versionString();

  /// Attempts to convert the given Ruby code into C code. Will not
  /// work unless the code is very simple.
  ///
  /// The vars array contains the list of variables that are defined
  /// in the code.
  QString ruby2C(const QString & code, QStringList * vars = NULL);

  /// The main object.
  extern RUBY_VALUE main;

  /// I don't think Ruby is too reentrant, so every call to ruby
  /// functions should be synchronized using this.
  ///
  /// For performance reasons, this is not done systematically in all
  /// the functions of the interface.
  extern QMutex rubyGlobalLock;

  /// @name Exception-catching routines
  ///
  /// A series of functions to wrap calls to ruby functions and ensure
  /// exceptions are caught.
  ///  
  /// @{
  RUBY_VALUE run(RUBY_VALUE (*f)());
  template<typename A1> RUBY_VALUE run(RUBY_VALUE (*f)(A1), A1); 
  template<typename A1, typename A2> RUBY_VALUE run(RUBY_VALUE (*f)(A1, A2), A1, A2); 

  template<typename A1, typename A2, 
           typename A3> RUBY_VALUE run(RUBY_VALUE (*f)(A1, A2, A3), 
                                  A1, A2, A3); 

  template<typename A1, typename A2, 
           typename A3, typename A4> RUBY_VALUE run(RUBY_VALUE (*f)(A1, A2, A3, A4), 
                                               A1, A2, A3, A4); 

  /// @}


  /// Conversion of array to QList
  template <typename A> RUBY_VALUE ary2ListHelper(RUBY_VALUE v,
                                                  QList<A> * rv,
                                                  std::function<A (RUBY_VALUE)> fn);
    
  template <typename A> QList<A> rubyArrayToList(RUBY_VALUE v,
                                                 std::function<A (RUBY_VALUE)> converter);


};


#endif
