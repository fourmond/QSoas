/**
   \file general-arguments.hh
   Various types of general Argument children
   Copyright 2011, 2012, 2013 by Vincent Fourmond

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
#ifndef __GENERAL_ARGUMENTS_HH
#define __GENERAL_ARGUMENTS_HH

#include <argument.hh>

/// A simple string argument
class StringArgument : public Argument {
public:

  StringArgument(const char * cn, const char * pn,
                 const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  }; 
  
  /// Returns a wrapped QString
  virtual ArgumentMarshaller * fromString(const QString & str) const;


  // Direct use of a line edit
  virtual QWidget * createEditor(QWidget * parent = NULL) const;
  virtual void setEditorValue(QWidget * editor, 
                              ArgumentMarshaller * value) const;
};

/// Several strings
///
/// @todo implement automatic splitting when necessary.
class SeveralStringsArgument : public Argument {
public:

  SeveralStringsArgument(const char * cn, const char * pn,
                         const char * d = "", bool g = true, 
                         bool def = false) : 
    Argument(cn, pn, d, g, def) {
  }; 


  /// Returns a wrapped QStringList
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;

};


/// A boolean argument
class BoolArgument : public Argument {
public:

  BoolArgument(const char * cn, const char * pn,
               const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  }; 
  
  /// Returns a wrapped bool
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  /// 
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;

  virtual QWidget * createEditor(QWidget * parent = NULL) const;
  virtual void setEditorValue(QWidget * editor, 
                              ArgumentMarshaller * value) const;


  virtual QStringList proposeCompletion(const QString & starter) const;
};

/// A choice between several fixed strings
class ChoiceArgument : public Argument {
  QStringList fixedChoices;

  QStringList (*provider)();

  QStringList choices() const;
public:

  ChoiceArgument(const QStringList & c,
                 const char * cn, const char * pn,
                 const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def), 
    fixedChoices(c), provider(NULL) {
  }; 

  ChoiceArgument(QStringList (*p)(),
                 const char * cn, const char * pn,
                 const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def), provider(p) {
  }; 
  
  /// Returns a wrapped QString
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;

  /// a rather easy one.
  virtual QStringList proposeCompletion(const QString & starter) const;
};


/// A choice between several fixed strings
class SeveralChoicesArgument : public Argument {
  QStringList fixedChoices;

  QStringList (*provider)();

  QStringList choices() const;
public:

  SeveralChoicesArgument(const QStringList & c,
                         const char * cn, const char * pn,
                         const char * d = "", bool g = true, 
                         bool def = false) : 
    Argument(cn, pn, d, g, def), 
    fixedChoices(c), provider(NULL) {
  }; 

  SeveralChoicesArgument(QStringList (*p)(),
                         const char * cn, const char * pn,
                         const char * d = "", bool g = true, 
                         bool def = false) : 
    Argument(cn, pn, d, g, def), provider(p) {
  }; 
  
  /// Returns a wrapped QString
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  /// a rather easy one.
  virtual QStringList proposeCompletion(const QString & starter) const;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;

};


/// A dataset from the stack
///
/// @todo Add prompting, but that will be fun.
class DataSetArgument : public Argument {
public:

  DataSetArgument(const char * cn, const char * pn,
                  const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  }; 
  
  /// Returns a wrapped DataSet *
  virtual ArgumentMarshaller * fromString(const QString & str) const;
};

/// Several datasets from the stack
///
/// @todo Add prompting, but that will be fun.
class SeveralDataSetArgument : public Argument {
public:

  SeveralDataSetArgument(const char * cn, const char * pn,
                         const char * d = "", bool g = true, 
                         bool def = false) : 
    Argument(cn, pn, d, g, def) {
  }; 
  
  /// Returns a wrapped QList<const DataSet *>
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;
};


/// A number
class NumberArgument : public Argument {
public:

  NumberArgument(const char * cn, const char * pn,
                 const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  }; 
  
  /// Returns a wrapped double
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;


  // Use of a line edit
  virtual QWidget * createEditor(QWidget * parent = NULL) const;
  virtual void setEditorValue(QWidget * editor, 
                              ArgumentMarshaller * value) const;

};

/// Several numbers
class SeveralNumbersArgument : public Argument {
protected:

  QString delim;
public:

  SeveralNumbersArgument(const char * cn, const char * pn,
                         const char * d = "", bool sl = true, 
                         bool def = false,
                         const char * del = ",") : 
    Argument(cn, pn, d, sl, def), delim(del) {
  }; 
  
  /// Returns a wrapped QList<double>
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;

};

/// A integer
class IntegerArgument : public Argument {
public:

  IntegerArgument(const char * cn, const char * pn,
                  const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  }; 
  
  /// Returns a wrapped double
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;
};

/// Several integers
class SeveralIntegersArgument : public Argument {
public:

  SeveralIntegersArgument(const char * cn, const char * pn,
                          const char * d = "", bool sl = true, 
                          bool def = false) : 
    Argument(cn, pn, d, sl, def) {
  }; 
  
  /// Returns a wrapped QList<double>
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;

};


/// A parameters hash (QString -> double)
///
/// @todo add automatic completion for keys ? Doesn't sound like a
/// clever thing to do...
class ParametersHashArgument : public Argument {
  QRegExp delims;
public:

  ParametersHashArgument(const char * cn, const char * pn,
                         const char * d = "", bool sl = true, 
                         bool def = false, 
                         const char * dels = "\\s*[=:]\\s*") : 
    Argument(cn, pn, d, sl, def), delims(dels) {
    
  }; 
  
  /// Returns a wrapped QList<double>
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;
  
};

/// A command.
class CommandArgument : public ChoiceArgument {

public:

  CommandArgument(const char * cn, const char * pn,
                  const char * d = "", 
                  bool def = false);  
  /// Returns a wrapped Command*
  virtual ArgumentMarshaller * fromString(const QString & str) const;

};

/// A style -- returned as a QString though, to avoid the problem of
/// spurious allocation.
class StyleGeneratorArgument : public ChoiceArgument {
public:

  StyleGeneratorArgument(const char * cn, const char * pn,
                         const char * d = "", 
                         bool def = false);  

  virtual ArgumentMarshaller * fromString(const QString & str) const;

};


/// A Regex
class RegexArgument : public Argument {
public:

  RegexArgument(const char * cn, const char * pn,
                const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  }; 
  
  /// Returns a wrapped Regex
  virtual ArgumentMarshaller * fromString(const QString & str) const;
};


#endif
