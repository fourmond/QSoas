/**
   \file mruby.hh
   Embedded mruby interpreter
   Copyright 2017 by CNRS/AMU

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
#ifndef __MRUBY_HH
#define __MRUBY_HH

/// This class embeds a mruby interpreter.
///
/// Most of the class's functions come in two variants:
/// * an "unprotected" version calling the raw code, ending with up
/// * a "protected" version, calling the unprotected one through protect()
class MRuby {

  static MRuby * globalInterpreter;

  /// Generates the code
  struct RProc * generateCode(const QByteArray & code,
                              const QString & fileName = "(eval)");

public:
  mrb_state *mrb;


  MRuby();
  ~MRuby();

  /// Evaluate the given code.
  mrb_value eval(const QByteArray & code);
  mrb_value eval(const char * code);
  mrb_value eval(const QString& code);

  /// Evaluate the given file
  mrb_value eval(QIODevice * device);

  /// Returns a representation of the object as a string
  QString inspect(mrb_value object);

  /// Runs the function with proper exception catching.
  mrb_value protect(const std::function<mrb_value ()> & function);

  /// Returns the static interpreter
  static MRuby * ruby();


  /// Defines a toplevel module
  struct RClass * defineModule(const char * name);

  /// Defines a module function
  void defineModuleFunction(struct RClass* cls, const char* name,
                            mrb_func_t func, mrb_aspec aspec);

  /// Detects the "external parameters" for the given code.
  QStringList detectParameters(const QByteArray & code);

  /// Defines a module function
  void defineGlobalConstant(const char *name, mrb_value val);

};


#endif
