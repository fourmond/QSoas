/**
   \file ruby.hh
   Utility functions to communicate with Ruby
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
#ifndef __RUBY_HH
#define __RUBY_HH

/// Namespace holding various utility functions to run embedded Ruby
namespace Ruby {

  /// Inits the Ruby interpreter. Must be called <b>only once ! </b>
  void initRuby();

  /// Calls the given C \a function with the given args in an
  /// exception-safe way, ie with proper handling of the exceptions...
  VALUE exceptionSafeCall(VALUE (*function)(...), void * args);

  /// The global rescue function, whose role is to convert Ruby
  /// exceptions into proper C++ exceptions.
  VALUE globalRescueFunction(VALUE, VALUE exception);

  /// Evaluates the given string, while trying to avoid segfaults.
  VALUE eval(QByteArray code);

  /// Makes a block from this code using the given variables.
  ///
  /// The \a variables gets updated with the actual variables that
  /// were necessary to compile the block
  VALUE makeBlock(QStringList * variables, const QByteArray & code);

  /// Read a file and execute it
  VALUE loadFile(const QString & file);

  /// Converts to a double in a Ruby-exception safe way: Ruby
  /// exceptions will result in proper C++ exceptions.
  ///
  /// Don't use NUM2DBL directly, unless you know it's is a Float.
  double toDouble(VALUE val);


  /// Makes a textual representation of any Ruby object (kinda rp_p)
  ///
  /// This function is already ruby-exception safe, ie no need to wrap
  /// it in a Ruby::run() call.
  QString inspect(VALUE val);

  /// Creates a Ruby string from a QString. No exception handling, but
  /// it should never fail !
  VALUE fromQString(const QString & str);

  /// Transforms the Ruby value into a string
  QString toQString(VALUE val);

  /// Returns the version string of Ruby
  QString versionString();

  /// The main object.
  extern VALUE main;

  /// @name Exception-catching routines
  ///
  /// A series of functions to wrap calls to ruby functions and ensure
  /// exceptions are caught.
  ///  
  /// @{
  VALUE run(VALUE (*f)());
  template<typename A1> VALUE run(VALUE (*f)(A1), A1); 
  template<typename A1, typename A2> VALUE run(VALUE (*f)(A1, A2), A1, A2); 


  template<typename A1, typename A2, 
           typename A3, typename A4> VALUE run(VALUE (*f)(A1, A2, A3, A4), 
                                               A1, A2, A3, A4); 

  /// @}
};


#endif
