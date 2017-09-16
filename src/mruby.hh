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

  /// Returns a block, i.e wraps the following code into a
  /// proc do |parameters...|
  ///   ...
  /// end
  mrb_value makeBlock(const QString & code, const QStringList & parameters);


  void gcRegister(mrb_value obj);
  void gcUnregister(mrb_value obj);

  /// @name Number-related functions
  /// @{

  /// Returns a new float with the given value.
  mrb_value newFloat(double value);

  /// Returns the value of the object as a double
  double floatValue(mrb_value fv);

  /// Returns the value of the object as a double
  double floatValue_up(mrb_value fv);

  /// Returns a new int with the given value
  mrb_value newInt(int value);

  /// @}

  /// Defines a toplevel module
  struct RClass * defineModule(const char * name);

  /// Defines a module function
  void defineModuleFunction(struct RClass* cls, const char* name,
                            mrb_func_t func, mrb_aspec aspec);

  /// Detects the "external parameters" for the given code.
  QStringList detectParameters(const QByteArray & code);

  /// Defines a module function
  void defineGlobalConstant(const char *name, mrb_value val);

  /// Returns the symbol for the given name
  mrb_sym intern(const char * symbol);

  /// Calls the given function of func.
  mrb_value funcall(mrb_value self, mrb_sym func, mrb_int nb,
                    const mrb_value * params);


  mrb_value funcall_up(mrb_value self, mrb_sym func, mrb_int nb,
                       const mrb_value * params);

  /// @name Array functions
  /// @{
  bool isArray(mrb_value array);
  mrb_value arrayRef(mrb_value array, int index);
  int arrayLength(mrb_value array);

  /// @}

  /// @name Global variables
  /// @{

  /// Sets a global variables
  void setGlobal(const char * name, mrb_value value);

  /// Gets the given global variable
  mrb_value getGlobal(const char * name);
  /// @}


  /// @name String functions
  /// @{

  /// Returns the string value of the object as a string.
  QString toQString(mrb_value value);

  /// Creates a mruby object corresponding to the string.
  mrb_value fromQString(const QString & str);

  /// @}

  /// @name Hash functions
  /// @{

  /// Creates a hash
  mrb_value newHash();

  /// Sets a hash element
  void hashSet(mrb_value hash, mrb_value key, mrb_value elem);

  /// @}
};


#endif
