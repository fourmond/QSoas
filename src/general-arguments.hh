/**
   \file general-arguments.hh
   Various types of general Argument children
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

#ifndef __GENERAL_ARGUMENTS_HH
#define __GENERAL_ARGUMENTS_HH

#include <argument.hh>

/// A simple string argument
class StringArgument : public Argument {
public:

  StringArgument(const char * cn, const char * pn,
                 const char * d = "") : Argument(cn, pn, d) {
  }; 
  
  /// Returns a wrapped QString
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;
};

/// A choice between several fixed strings
class ChoiceArgument : public Argument {
  QStringList fixedChoices;

  QStringList (*provider)();

  QStringList choices() const;
public:

  ChoiceArgument(const QStringList & c,
                 const char * cn, const char * pn,
                 const char * d = "") : Argument(cn, pn, d), 
                                        fixedChoices(c), provider(NULL) {
  }; 

  ChoiceArgument(QStringList (*p)(),
                 const char * cn, const char * pn,
                 const char * d = "") : Argument(cn, pn, d), provider(p) {
  }; 
  
  /// Returns a wrapped QString
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;

  /// a rather easy one.
  virtual QStringList proposeCompletion(const QString & starter) const;
};

/// A dataset from the stack
///
/// @todo Add prompting, but that will be fun.
class DataSetArgument : public Argument {
public:

  DataSetArgument(const char * cn, const char * pn,
                  const char * d = "") : Argument(cn, pn, d) {
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
                         const char * d = "", bool g = true) : 
    Argument(cn, pn, d, g) {
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
                 const char * d = "") : Argument(cn, pn, d) {
  }; 
  
  /// Returns a wrapped double
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;
};

/// Several numbers
class SeveralNumbersArgument : public Argument {
public:

  SeveralNumbersArgument(const char * cn, const char * pn,
                         const char * d = "", bool sl = true) : 
    Argument(cn, pn, d, sl) {
  }; 
  
  /// Returns a wrapped QList<double>
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;

};


#endif
