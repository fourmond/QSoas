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
                              const ArgumentMarshaller * value) const override;

  virtual QString typeName() const override {
    return "text";
  };

  virtual QString typeDescription() const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

};

/// Several strings
class SeveralStringsArgument : public Argument {
  QRegExp separator;

  /// 
  QString type, description;
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

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;

  /// This function is here to help describe the type.
  /// Returns this so it can be easily chained
  SeveralStringsArgument * describe(const QString &desc, const QString & type = "");

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


  virtual QVariant toVariant(const ArgumentMarshaller * arg) const override;


  virtual QStringList toString(const ArgumentMarshaller * arg) const override;

  virtual QString typeName() const override;

  virtual QString typeDescription() const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;
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
                              const ArgumentMarshaller * value) const override;

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

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;

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

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;

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
                          bool def = false);

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

  virtual QString typeName() const override;

  virtual QString typeDescription() const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

  /// Returns the name of a dataset -- or, more precisely, its number.
  ///
  /// Raises an exception if the dataset is absent from the stacK.
  static QString dataSetName(const DataSet * ds);


  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;
  virtual QStringList proposeCompletion(const QString & starter) const override;

};

/// Several datasets from the stack. The specifications are
/// comma-separated.
///
/// @todo Add prompting, but that will be fun.
class SeveralDataSetArgument : public Argument {
  bool nullOK;
public:

  SeveralDataSetArgument(const char * cn, const char * pn,
                         const char * d = "", bool g = true, 
                         bool def = false, bool nOK = false) : 
    Argument(cn, pn, d, g, def), nullOK(nOK) {
  }; 
  
  /// Returns a wrapped QList<const DataSet *>
  virtual ArgumentMarshaller * fromString(const QString & str) const override;
  QStringList toString(const ArgumentMarshaller * arg) const override;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const override;

  virtual QString typeName() const override;

  virtual QString typeDescription() const override;

  virtual QStringList proposeCompletion(const QString & starter) const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;
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

  virtual QStringList toString(const ArgumentMarshaller * arg) const override;
  virtual QVariant toVariant(const ArgumentMarshaller * arg) const override;


  // Use of a line edit
  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;

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

  virtual QStringList toString(const ArgumentMarshaller * arg) const override;
  virtual QVariant toVariant(const ArgumentMarshaller * arg) const override;

  virtual QString typeName() const override {
    return "numbers";
  };

  virtual QString typeDescription() const override {
    return QString("Several floating-point numbers, separated by %1").arg(delim);
  };

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;

};

/// A integer
class IntegerArgument : public Argument {
public:

  IntegerArgument(const char * cn, const char * pn,
                  const char * d = "", bool def = false) : 
    Argument(cn, pn, d, false, def) {
  }; 

  virtual QStringList toString(const ArgumentMarshaller * arg) const override;
  virtual QVariant toVariant(const ArgumentMarshaller * arg) const override;

  /// Returns a wrapped double
  virtual ArgumentMarshaller * fromString(const QString & str) const override;

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const override;

  virtual QString typeName() const override {
    return "integer";
  };

  virtual QString typeDescription() const override {
    return "An integer";
  };

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;

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

  virtual QStringList toString(const ArgumentMarshaller * arg) const override;
  virtual QVariant toVariant(const ArgumentMarshaller * arg) const override;
  
  virtual QString typeName() const override {
    return "integers";
  };

  virtual QString typeDescription() const override {
    return "A comma-separated list of integers";
  };

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;

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

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;

};

class Vector;
/// This class simply wraps a column specification until the moment it
/// is actually used on a DataSet. It is essentially just a wrapped
/// QString
///
/// Known formats:
///
/// @li just a number: 1-based index (1 = X, 2 = Y, etc...), 0 is index when applicable
/// @li negative numbers: -1 is the last one, -2, the one before last...
/// @li #number: 0-based index
/// @li x, y, z, y2...yN
/// @li last
/// @li none: returns -1
/// @li named:(main column name)
/// @li $c.name (as will happen in apply-formula later on)
class ColumnSpecification {
  QString spec;

  bool acceptNone;

  bool acceptIndex;
public:
  explicit ColumnSpecification(const QString & str,
                               bool acceptNone = false,
                               bool acceptIndex = false);
  /// This creates an invalid one, to be used in ranges.
  ColumnSpecification();

  /// Returns the value, for the specific dataset.
  ///
  /// If no specification is given, returns def
  ///
  /// Will return -1 for the index column when applicable.
  int getValue(const DataSet * ds, int def = -2) const;

  /// Returns the actual column for the given dataset.
  /// Returns an empty vector on invalid
  Vector getColumn(const DataSet * ds) const;

  /// Returns true if the string isn't empty
  bool isValid() const;

  /// Returns the text specification
  QString specification() const;

  /// Returns all the valid column names for the given dataset.
  static QStringList validNames(const DataSet * ds, bool acceptNone,
                                bool acceptIndex);

  /// Updates the target integer to be the givne option
  static void updateFromOptions(const CommandOptions & opts,
                                const QString & key,
                                int & target, const DataSet * ds);
};


/// The column of a dataset
class ColumnArgument : public Argument {

  bool acceptNone;

  bool acceptIndex;
public:


  ColumnArgument(const char * cn, const char * pn,
                 const char * d = "", bool def = false,
                 bool _acceptNone = false, bool ai = false) : 
    Argument(cn, pn, d, false, def),
    acceptNone(_acceptNone), acceptIndex(ai) {
  }; 
  
  /// Returns a wrapped int
  virtual ArgumentMarshaller * fromString(const QString & str) const override;
  QStringList toString(const ArgumentMarshaller * arg) const override;

  virtual QString typeName() const override;
  virtual QString typeDescription() const override;

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override;

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;

  /// Proposes a completion, based on the current dataset.
  virtual QStringList proposeCompletion(const QString & starter) const override;

};

/// A list of columns
class ColumnListSpecification {
  bool acceptIndex;
public:

  explicit ColumnListSpecification(bool ai = false) :
    acceptIndex(ai) {;};
  
  QList<QPair<ColumnSpecification, ColumnSpecification> > columns;

  /// Returns the value, for the specific dataset.
  QList<int> getValues(const DataSet * ds) const;

  /// Returns the vectors, for the given dataset
  QList<Vector> getColumns(const DataSet * ds) const;
};

/// Several integers
class SeveralColumnsArgument : public Argument {

  bool acceptIndex;
public:

  SeveralColumnsArgument(const char * cn, const char * pn,
                         const char * d = "", bool sl = true, 
                         bool def = false, bool _acceptIndex = false) : 
    Argument(cn, pn, d, sl, def),
    acceptIndex(_acceptIndex)
  {
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

  virtual QWidget * createEditor(QWidget * parent = NULL) const override;
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override;

  /// Proposes a completion, based on the current dataset.
  virtual QStringList proposeCompletion(const QString & starter) const override;

};

/// A piece of code. The only difference this provides to
/// StringArgument is that it provides limited completion on $stats
/// and $meta.
class CodeArgument : public StringArgument {
public:

  CodeArgument(const char * cn, const char * pn,
                 const char * d = "", bool def = false) : 
    StringArgument(cn, pn, d, def) {
  }; 
  
  virtual QString typeName() const override {
    return "code";
  };

  virtual QString typeDescription() const override;

  virtual QStringList proposeCompletion(const QString & starter) const override;

  /// Propose completions for code. Used as well for SeveralCodesArgument
  static QStringList completeCode(const QString & starter);

};

/// A piece of code. The only difference this provides to
/// StringArgument is that it provides limited completion on $stats
/// and $meta.
class SeveralCodesArgument : public SeveralStringsArgument {
public:

  SeveralCodesArgument(const char * cn, const char * pn,
                       const char * d = "", bool g = true, 
                       bool def = false) :
    SeveralStringsArgument(cn, pn, d, g, def) {
  }; 
  
  virtual QString typeName() const override {
    return "codes";
  };

  virtual QString typeDescription() const override;

  virtual QStringList proposeCompletion(const QString & starter) const override;

};

#endif
