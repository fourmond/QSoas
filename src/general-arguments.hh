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

class DataSet;

/// Strings

/// A simple string argument
class StringArgument : public Argument {
public:

  StringArgument(const char * cn, const char * pn,
                 const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  }; 
  
  /// Returns a wrapped QString
  virtual ArgumentMarshaller * fromString(const QString & str) const override;

  QStringList toString(const ArgumentMarshaller * arg) const override;


  // Direct use of a line edit
  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              ArgumentMarshaller * value) const override;

  virtual QString typeName() const override {
    return "text";
  };

  virtual QString typeDescription() const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

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
  virtual ArgumentMarshaller * fromString(const QString & str) const override;
  QStringList toString(const ArgumentMarshaller * arg) const override;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const override;

  virtual QString typeName() const override;

  virtual QString typeDescription() const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;
};

/// A series of x=y strings converted to QHash<QString, QVariant>.
///
/// No separator, will cause more harm than good ? Speficying several
/// times should work.
class MetaHashArgument : public Argument {
public:

  MetaHashArgument(const char * cn, const char * pn,
                         const char * d = "", bool g = true, 
                         bool def = false) : 
    Argument(cn, pn, d, g, def) {
  }; 



  /// Returns a wrapped QStringList
  virtual ArgumentMarshaller * fromString(const QString & str) const override;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const override;

  QStringList toString(const ArgumentMarshaller * arg) const override;

  virtual QString typeName() const override;

  virtual QString typeDescription() const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;
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



  virtual QString typeName() const  override {
    return "time-dependent parameters";
  };

  virtual QString typeDescription() const override;
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
  virtual ArgumentMarshaller * fromString(const QString & str) const override;

  QStringList toString(const ArgumentMarshaller * arg) const override;

  /// 
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const override;

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              ArgumentMarshaller * value) const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

  virtual QStringList proposeCompletion(const QString & starter) const override;

  virtual QString typeName() const override {
    return "yes-no";
  };

  virtual QString typeDescription() const override;
};

/// A choice between several fixed strings
class ChoiceArgument : public Argument {
  QStringList fixedChoices;

  std::function<QStringList ()> provider;

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

  ChoiceArgument(const std::function<QStringList ()> & p,
                 const char * cn, const char * pn,
                 const char * d = "", bool def = false,
                 const char * chN = "") : 
    Argument(cn, pn, d, false, def), provider(p), choiceName(chN) {
  }; 
  
  /// Returns a wrapped QString
  virtual ArgumentMarshaller * fromString(const QString & str) const override;

  QStringList toString(const ArgumentMarshaller * arg) const override;

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

  /// a rather easy one.
  virtual QStringList proposeCompletion(const QString & starter) const override;

  virtual QString typeName() const override;
  virtual QString typeDescription() const override;

};


/// A choice between several fixed strings
class SeveralChoicesArgument : public Argument {
  QStringList fixedChoices;

  QStringList (*provider)();

  QStringList choices() const;

  /// The separator, one char for now
  QChar sep;
public:

  SeveralChoicesArgument(const QStringList & c,
                         QChar s,
                         const char * cn, const char * pn,
                         const char * d = "", bool g = true, 
                         bool def = false) : 
    Argument(cn, pn, d, g, def), 
    fixedChoices(c), provider(NULL), sep(s) {
  }; 

  SeveralChoicesArgument(QStringList (*p)(),
                         QChar s,
                         const char * cn, const char * pn,
                         const char * d = "", bool g = true, 
                         bool def = false) : 
    Argument(cn, pn, d, g, def), provider(p), sep(s) {
  }; 
  
  /// Returns a wrapped QStringList
  virtual ArgumentMarshaller * fromString(const QString & str) const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

  QStringList toString(const ArgumentMarshaller * arg) const override;


  virtual QStringList proposeCompletion(const QString & starter) const override;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const override;

};


/// A choice between several fixed strings
class FitNameArgument : public ChoiceArgument {
public:

  FitNameArgument(const char * cn, const char * pn,
                  const char * d = "", bool def = false,
                  const char * chN = "");

  virtual QString typeDescription() const override;

};

/// A choice between several fixed strings
class SeveralFitNamesArgument : public SeveralChoicesArgument {
public:

  SeveralFitNamesArgument(const char * cn, const char * pn,
                          const char * d = "", bool g = true,
                          bool def = false,
                          const char * chN = "");

  virtual QString typeDescription() const override;

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
  virtual ArgumentMarshaller * fromString(const QString & str) const override;
  QStringList toString(const ArgumentMarshaller * arg) const override;

  virtual QString typeName() const override {
    return "buffer";
  };

  virtual QString typeDescription() const override {
    return "The number of a buffer in the stack";
  };



  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

  /// Returns the name of a dataset -- or, more precisely, its number.
  ///
  /// Raises an exception if the dataset is absent from the stacK.
  static QString dataSetName(const DataSet * ds);
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
  virtual ArgumentMarshaller * fromString(const QString & str) const override;
  QStringList toString(const ArgumentMarshaller * arg) const override;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const override;

  virtual QString typeName() const override {
    return "buffers";
  };

  virtual QString typeDescription() const override {
    return "comma-separated lists of buffers in the stack, "
      "see [buffers lists](#buffer-lists)";
  };

  virtual QStringList proposeCompletion(const QString & starter) const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;
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
  virtual ArgumentMarshaller * fromString(const QString & str) const override;

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const override;

  QStringList toString(const ArgumentMarshaller * arg) const override;


  // Use of a line edit
  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              ArgumentMarshaller * value) const override;

  virtual QString typeName() const override {
    return "number";
  };

  virtual QString typeDescription() const override {
    return "A floating-point number";
  };

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;
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
  virtual ArgumentMarshaller * fromString(const QString & str) const override;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const override;

  QStringList toString(const ArgumentMarshaller * arg) const override;

  virtual QString typeName() const override {
    return "numbers";
  };

  virtual QString typeDescription() const override {
    return QString("Several floating-point numbers, separated by %1").arg(delim);
  };

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;
};

/// A integer
class IntegerArgument : public Argument {
public:

  IntegerArgument(const char * cn, const char * pn,
                  const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  }; 

  QStringList toString(const ArgumentMarshaller * arg) const override;

  /// Returns a wrapped double
  virtual ArgumentMarshaller * fromString(const QString & str) const override;

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const override;

  virtual QString typeName() const override {
    return "integer";
  };

  virtual QString typeDescription() const {
    return "An integer";
  };

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              ArgumentMarshaller * value) const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

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
  virtual ArgumentMarshaller * fromString(const QString & str) const override;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const override; 

  QStringList toString(const ArgumentMarshaller * arg) const override;
  
  virtual QString typeName() const override {
    return "integers";
  };

  virtual QString typeDescription() const override {
    return "A comma-separated list of integers";
  };

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

};

//////////////////////////////////////////////////////////////////////

// /// A parameters hash (QString -> double)
// ///
// /// @todo add automatic completion for keys ? Doesn't sound like a
// /// clever thing to do...
// class ParametersHashArgument : public Argument {
//   QRegExp delims;
// public:

//   ParametersHashArgument(const char * cn, const char * pn,
//                          const char * d = "", bool sl = true, 
//                          bool def = false, 
//                          const char * dels = "\\s*[=:]\\s*") : 
//     Argument(cn, pn, d, sl, def), delims(dels) {
    
//   }; 
  
//   /// Returns a wrapped QList<double>
//   virtual ArgumentMarshaller * fromString(const QString & str) const override;

//   QStringList toString(const ArgumentMarshaller * arg) const override;

//   virtual void concatenateArguments(ArgumentMarshaller * a, 
//                                     const ArgumentMarshaller * b) const override;

//   virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

// };

/// A command.
class CommandArgument : public ChoiceArgument {

public:

  CommandArgument(const char * cn, const char * pn,
                  const char * d = "", 
                  bool def = false);  
  /// Returns a wrapped Command*
  virtual ArgumentMarshaller * fromString(const QString & str) const override;

  QStringList toString(const ArgumentMarshaller * arg) const override;

  virtual QString typeName() const override {
    return "command";
  };

  virtual QString typeDescription() const override {
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

  virtual ArgumentMarshaller * fromString(const QString & str) const override;

  virtual QString typeName() const override {
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
  virtual ArgumentMarshaller * fromString(const QString & str) const override;
  QStringList toString(const ArgumentMarshaller * arg) const override;

  virtual QString typeName() const override;
  virtual QString typeDescription() const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

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
  virtual ArgumentMarshaller * fromString(const QString & str) const override;
  QStringList toString(const ArgumentMarshaller * arg) const override;

  virtual QString typeName() const override {
    return "column";
  };

  virtual QString typeDescription() const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

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
  virtual ArgumentMarshaller * fromString(const QString & str) const override;
  QStringList toString(const ArgumentMarshaller * arg) const override;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const override;

  virtual QString typeName() const override {
    return "columns";
  };

  virtual QString typeDescription() const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;
};

#endif
