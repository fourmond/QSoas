/**
   \file general-arguments.hh
   Various types of general Argument children
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
#ifndef __GENERAL_ARGUMENTS_HH
#define __GENERAL_ARGUMENTS_HH

#include <argument.hh>


/// Strings

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

  virtual QString typeName() const {
    return "text";
  };

  virtual QString typeDescription() const;
};

/// Several strings
class SeveralStringsArgument : public Argument {
  QRegExp separator;
public:

  SeveralStringsArgument(const char * cn, const char * pn,
                         const char * d = "", bool g = true, 
                         bool def = false) : 
    Argument(cn, pn, d, g, def) {
  }; 

  SeveralStringsArgument(const QRegExp & re, 
                         const char * cn, const char * pn,
                         const char * d = "", bool g = true, 
                         bool def = false) : 
    Argument(cn, pn, d, g, def), separator(re) {
  }; 


  /// Returns a wrapped QStringList
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;

  virtual QString typeName() const;

  virtual QString typeDescription() const;
};

//////////////////////////////////////////////////////////////////////

/// Time-dependent parameters
class TDPArgument : public SeveralStringsArgument {
public:

  TDPArgument(const char * cn, const char * pn,
              const char * d = "", bool g = true, 
              bool def = false) : 
    SeveralStringsArgument(QRegExp("\\s*;\\s*"), cn, pn, d, g, def) {
  }; 



  virtual QString typeName() const  {
    return "time-dependent parameters";
  };

  virtual QString typeDescription() const;

};

//////////////////////////////////////////////////////////////////////


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

  virtual QString typeName() const {
    return "yes-no";
  };

  virtual QString typeDescription() const;
};

/// A choice between several fixed strings
class ChoiceArgument : public Argument {
  QStringList fixedChoices;

  QStringList (*provider)();

  QStringList choices() const;

  QString choiceName;
public:

  ChoiceArgument(const QStringList & c,
                 const char * cn, const char * pn,
                 const char * d = "", bool def = false,
                 const char * chN = "") : 
    Argument(cn, pn, d, false, def), 
    fixedChoices(c), provider(NULL), choiceName(chN) {
  }; 

  ChoiceArgument(QStringList (*p)(),
                 const char * cn, const char * pn,
                 const char * d = "", bool def = false,
                 const char * chN = "") : 
    Argument(cn, pn, d, false, def), provider(p), choiceName(chN) {
  }; 
  
  /// Returns a wrapped QString
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;

  /// a rather easy one.
  virtual QStringList proposeCompletion(const QString & starter) const;

  virtual QString typeName() const;
  virtual QString typeDescription() const;

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


/// A choice between several fixed strings
class FitNameArgument : public ChoiceArgument {
public:

  FitNameArgument(const char * cn, const char * pn,
                  const char * d = "", bool def = false,
                  const char * chN = "");

  virtual QString typeDescription() const;

};

//////////////////////////////////////////////////////////////////////

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

  virtual QString typeName() const {
    return "buffer";
  };

  virtual QString typeDescription() const {
    return "The number of a buffer in the stack";
  };
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

  virtual QString typeName() const {
    return "buffers";
  };

  virtual QString typeDescription() const {
    return "comma-separated lists of buffers in the stack, "
      "see [buffers lists](#buffer-lists)";
  };

  virtual QStringList proposeCompletion(const QString & starter) const;
};


/// A number
class NumberArgument : public Argument {
  bool special;
public:

  NumberArgument(const char * cn, const char * pn,
                 const char * d = "", bool def = false,
                 bool spec = false) : 
    Argument(cn, pn, d, false, def), special(spec) {
  }; 
  
  /// Returns a wrapped double
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;


  // Use of a line edit
  virtual QWidget * createEditor(QWidget * parent = NULL) const;
  virtual void setEditorValue(QWidget * editor, 
                              ArgumentMarshaller * value) const;

  virtual QString typeName() const {
    return "number";
  };

  virtual QString typeDescription() const {
    return "A floating-point number";
  };

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

  virtual QString typeName() const {
    return "numbers";
  };

  virtual QString typeDescription() const {
    return QString("Several floating-point numbers, separated by %1").arg(delim);
  };

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

  virtual QString typeName() const {
    return "integer";
  };

  virtual QString typeDescription() const {
    return "An integer";
  };

  virtual QWidget * createEditor(QWidget * parent = NULL) const;
  virtual void setEditorValue(QWidget * editor, 
                              ArgumentMarshaller * value) const;

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

  virtual QString typeName() const {
    return "integers";
  };

  virtual QString typeDescription() const {
    return "A comma-separated list of integers";
  };

};

//////////////////////////////////////////////////////////////////////

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

  virtual QString typeName() const {
    return "command";
  };

  virtual QString typeDescription() const {
    return "The name of one of QSoas's commands";
  };

};

/// A style -- returned as a QString though, to avoid the problem of
/// spurious allocation.
class StyleGeneratorArgument : public ChoiceArgument {
public:

  StyleGeneratorArgument(const char * cn, const char * pn,
                         const char * d = "", 
                         bool def = false);  

  virtual ArgumentMarshaller * fromString(const QString & str) const;

  virtual QString typeName() const {
    return "style";
  };

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

  virtual QString typeName() const;
  virtual QString typeDescription() const;

};


/// The column of a dataset
class ColumnArgument : public Argument {
public:

  /// Parses a column specification. Known formats:
  ///
  /// * just a number: 1-based index (1 = X, 2 = Y, etc...)
  /// * #number: 0-based index
  /// * X, Y, Z, Y2...YN
  static int parseFromText(const QString & str);

  ColumnArgument(const char * cn, const char * pn,
                 const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  }; 
  
  /// Returns a wrapped int
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  virtual QString typeName() const {
    return "column";
  };

  virtual QString typeDescription() const {
    return "The number/name of a column in a buffer";
  };

};

/// Several integers
class SeveralColumnsArgument : public Argument {
public:

  SeveralColumnsArgument(const char * cn, const char * pn,
                          const char * d = "", bool sl = true, 
                          bool def = false) : 
    Argument(cn, pn, d, sl, def) {
  }; 
  
  /// Returns a wrapped QList<int>
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;

  virtual QString typeName() const {
    return "columns";
  };

  virtual QString typeDescription() const {
    return "A comma-separated list of columns";
  };

};

#endif
