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


#ifndef __RUBY_HH
#define __RUBY_HH

/// Namespace holding various utility functions to run embedded Ruby
namespace Ruby {

  /// Inits the Ruby interpreter. Must be called <b>only once ! </b>
  void initRuby();

  /// The global rescue function, whose role is to convert Ruby
  /// exceptions into proper C++ exceptions.
  VALUE globalRescueFunction(VALUE, VALUE exception);

  /// @name Exception-catching routines
  ///
  /// A series of functions to wrap calls to ruby functions and ensure
  /// exceptions are caught.
  ///  
  /// @{
  VALUE run(VALUE (*f)());
  template<typename A1> VALUE run(VALUE (*f)(A1), A1); 


  template<typename A1, typename A2, 
           typename A3, typename A4> VALUE run(VALUE (*f)(A1, A2, A3, A4), 
                                               A1, A2, A3, A4); 
};


#endif
