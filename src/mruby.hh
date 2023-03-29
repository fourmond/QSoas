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
                              const QString & fileName = "(eval)",
                              int line = -1);

  struct RClass * cQSoasInterface;


  /// The base class of exceptions.
  struct RClass * cException;

  mrb_value soasInstance;

public:
  /// The "Cplx" class
  struct RClass * cCplx;
private:

  friend mrb_value qs_interface(mrb_state *, mrb_value);

  /// initialize the ruby -> QSoas interface.
  void initializeInterface();

  /// initialize the Qt-based Ruby regular expressions
  void initializeRegexp();

  /// initialize the support for complex algebra
  void initializeComplex();

  mrb_value cTime;

  mrb_value cFancyHash;

  mrb_sym sNew;
  mrb_sym sToS;

  mrb_sym sBrackets;

public:
  mrb_state *mrb;


  MRuby();
  ~MRuby();

  /// Evaluate the given code.
  mrb_value eval(const QByteArray & code,
                 const QString & fileName = "(eval)",
                 int line = -1);
  mrb_value eval(const char * code,
                 const QString & fileName = "(eval)",
                 int line = -1);
  mrb_value eval(const QString& code,
                 const QString & fileName = "(eval)",
                 int line = -1);

  /// Evaluate the given file
  mrb_value eval(QIODevice * device);

  /// Returns a representation of the object as a string
  QString inspect(mrb_value object);

  /// Runs the function with proper exception catching.
  mrb_value protect(const std::function<mrb_value ()> & function);

  /// If the object obj is an exception, throws it.
  void throwIfException(mrb_value obj);

  /// Returns the static interpreter
  static MRuby * ruby();

  /// Returns a block, i.e wraps the following code into a
  /// proc do |parameters...|
  ///   ...
  /// end
  mrb_value makeBlock(const QString & code, const QStringList & parameters);


  void gcRegister(mrb_value obj);
  void gcUnregister(mrb_value obj);

  /// Starts the garbage collection
  void startGC();

  /// @name Number-related functions
  /// @{

  /// Returns a new float with the given value.
  mrb_value newFloat(double value);

  /// Returns a new complex value
  mrb_value newComplex(const gsl_complex & value);

  /// Returns true if the given value is a Cplx 
  bool isComplex(mrb_value value);

  /// Returns the complex value
  gsl_complex complexValue(mrb_value value);

  /// Returns the complex value. This is the unprotected version
  gsl_complex complexValue_up(mrb_value value);

  /// Returns the value of the object as a double
  double floatValue(mrb_value fv);

  /// Returns the value of the object as a double.
  /// This is the unprotected version
  double floatValue_up(mrb_value fv);

  /// Returns a new int with the given value
  mrb_value newInt(int value);

  /// @}

  /// Defines a toplevel module
  struct RClass * defineModule(const char * name);

  /// Defines a module function
  void defineModuleFunction(struct RClass* cls, const char* name,
                            mrb_func_t func, mrb_aspec aspec);





  /// Detects the "external parameters" for the given code, using a
  /// native detection mode, i.e. asking mruby what it thinks the
  /// parameters are.
  QStringList detectParametersNative(const QByteArray & code,
                                     QStringList * localVariables = NULL);

  /// Detects the "external parameters" for the given code.  Unlike
  /// the detectParameters, this does not rely on a deep understanding
  /// of the code, but rather a rough parsing of it. It seems good
  /// enough in fact.
  static QStringList detectParametersApprox(const QByteArray & code,
                                            QStringList * localVariables = NULL);

  static QStringList detectParametersApprox(const QString & code,
                                            QStringList * localVariables = NULL);

  /// Returns a QByteArray with a size matching that of code, with the
  /// "quoting status" of each character in the return value.
  ///  status & 0x03 is 0 for unquoted, 1 for single and 2 for double
  ///  status >> 2 is the level of nesting.
  static QByteArray annotateQuotes(const QString & code);

  /// Detects the "external parameters" for the given code. This may
  /// either be detectParametersNative or detectParametersApprox
  QStringList detectParameters(const QByteArray & code,
                               QStringList * localVariables = NULL);

  /// Defines a module function
  void defineGlobalConstant(const char *name, mrb_value val);

  /// Gets the value of a constant
  mrb_value getConstant(const char *name);

  /// Returns the symbol for the given name
  mrb_sym intern(const char * symbol);

  /// Calls the given function of func.
  mrb_value funcall(mrb_value self, mrb_sym func, mrb_int nb,
                    const mrb_value * params);


  mrb_value funcall_up(mrb_value self, mrb_sym func, mrb_int nb,
                       const mrb_value * params);

  /// @name Array functions
  /// @{

  /// Returns true if the object is an array
  bool isArray(mrb_value obj);

  /// Creates a new array
  mrb_value newArray();

  /// Returns the array element at index
  mrb_value arrayRef(mrb_value array, int index);

  /// Pushes to the array
  void arrayPush(mrb_value array, mrb_value obj);

  /// Returns the length of the array
  int arrayLength(mrb_value array);

  /// Iterates over the array, running the function.
  void arrayIterate(mrb_value array,
                    const std::function <void (mrb_value)> & func);

  /// Creates an array of strings from a Qt string list.
  mrb_value arrayFromStringList(const QStringList & lst);

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
  
  /// Creates a mruby symbol corresponding to the string
  mrb_value symbolFromQString(const QString & str);

  /// This series of functions calls the printf of ruby to format a
  /// string. This formatting prevents security issues, unlike
  /// asprintf.
  QString safeAsprintf(const QString & format, int value);

  /// @overload
  QString safeAsprintf(const QString & format, double value);

  /// @}

  /// @name Hash functions
  /// @{

  /// Returns true if the value is a hash
  bool isHash(mrb_value obj);

  /// Creates a hash
  mrb_value newHash();


  /// Sets a hash element
  void hashSet(mrb_value hash, mrb_value key, mrb_value elem);

  /// Gets a hash element
  mrb_value hashRef(mrb_value hash, mrb_value key);

  /// Iterates over the hash, running the function.
  void hashIterate(mrb_value hash,
                   const std::function <void (mrb_value key, mrb_value value)> & func);

  /// Creates a new FancyHash object
  mrb_value newFancyHash();

  /// Sets a hash element
  void fancyHashSet(mrb_value hash, mrb_value key, mrb_value elem);


  /// @}


  /// @name Time-related functions
  /// @{
  mrb_value newTime(int year = 0, int month = 1, int day = 1,
                    int hour = 0, int min = 0, int sec = 0, int usec = 0);

  /// @}

  /// Returns true if a < b
  bool isInferior(mrb_value a, mrb_value b);

  /// Returns a string that represents the current memory use of the
  /// mrb object
  QString memoryUse();

  /// Strictly debug-only function. To be used with the DUMP_MRUBY define.
  static void dumpValue(const char * t, int n, const char * s, mrb_value v);

#define DUMP_MRUBY(v) MRuby::dumpValue(__PRETTY_FUNCTION__, __LINE__, #v, v)
};

/// A small helper class that saves the GC arena and restores it at
/// the end of the function.
class MRubyArenaContext {
  int idx;

  MRuby * mr;
public:
  explicit MRubyArenaContext(MRuby * m) : mr(m) {
    idx = mrb_gc_arena_save(mr->mrb);
  }

  ~MRubyArenaContext() {
    mrb_gc_arena_restore(mr->mrb, idx);
  }
};


#endif
