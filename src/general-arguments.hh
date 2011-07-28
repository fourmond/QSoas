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


/// An argument representing a unique file name.
/// @todo Restrict the returned files to existing/readable files ?
///
/// @todo Provide filters for the dialog
///
/// @todo Make the distinction between save and load ?
class FileArgument : public Argument {
public:

  FileArgument(const char * cn, const char * pn,
               const char * d = "") : Argument(cn, pn, d) {
  }; 
  
  /// Returns a wrapped QString for now.
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  /// Prompting uses a QFileDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;
};

/// An argument representing a unique file name.
/// @todo Provide filters for the dialog
///
/// @todo Make the distinction between save and load ?
///
/// @todo is there any code to share with FileArgument ?
class SeveralFilesArgument : public Argument {
public:

  SeveralFilesArgument(const char * cn, const char * pn,
                       const char * d = "", bool g = false) : 
    Argument(cn, pn, d, g) {
  }; 
  
  /// Returns a wrapped QStringList
  virtual ArgumentMarshaller * fromString(const QString & str) const;
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;
  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;
};


#endif
