/**
   \file file-arguments.hh
   Argument children for handling files
   Copyright 2011 by Vincent Fourmond
             2012, 2013 by CNRS/AMU

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
#ifndef __FILE_ARGUMENTS_HH
#define __FILE_ARGUMENTS_HH

#include <argument.hh>


/// An argument representing a unique file name.
/// @todo Restrict the returned files to existing/readable files ?
///
/// @todo Provide filters for the dialog
class FileArgument : public Argument {
  
  /// can it match directories ?
  bool isDir;
public:

  FileArgument(const char * cn, const char * pn,
               const char * d = "", bool dir = false,
               bool def = false) : Argument(cn, pn, d, false, def),
                                                        isDir(dir) {
  }; 
  
  /// Returns a wrapped QString
  virtual ArgumentMarshaller * fromString(const QString & str) const;

  /// Prompting uses a QFileDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;
  virtual QStringList proposeCompletion(const QString & starter) const;

  virtual QString typeName() const {
    return isDir ? "directory" : "file";
  };

  virtual QString typeDescription() const;

};

/// An argument representing a unique file name, for saving.
/// @todo Provide filters for the dialog
class FileSaveArgument : public FileArgument {

  /// A default name for the save dialog box.
  QString defaultName;

  /// A way to acquire the default name
  QString (*provider)();

  /// Whether or not we ask confirmation before overwriting
  bool askOverwrite;
public:

  FileSaveArgument(const char * cn, const char * pn,
                   const char * d = "", 
                   const QString & def = QString(),
                   bool asko = true) : 
    FileArgument(cn, pn, d), defaultName(def),
    provider(NULL),  askOverwrite(asko) {
  }; 

  FileSaveArgument(const char * cn, const char * pn,
                   const char * d, QString (*pr)(),
                   bool asko = true) : FileArgument(cn, pn, d),
                                       provider(pr), askOverwrite(asko) {
  }; 

  /// Returns a wrapped QString. Check that the target file does not
  /// exist if askOverwrite is on.
  virtual ArgumentMarshaller * fromString(const QString & str) const;
  
  /// Prompting uses a QFileDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;

};



/// An argument representing a unique file name.
/// @todo Provide filters for the dialog
class SeveralFilesArgument : public Argument {
  bool expandGlobs;
public:

  SeveralFilesArgument(const char * cn, const char * pn,
                       const char * d = "", bool g = true, bool glob = true) : 
    Argument(cn, pn, d, g), expandGlobs(glob) {
  }; 
  
  /// Returns a wrapped QStringList
  virtual ArgumentMarshaller * fromString(const QString & str) const;
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;
  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;
  virtual QStringList proposeCompletion(const QString & starter) const;


  virtual QString typeName() const {
    return "files";
  };

  virtual QString typeDescription() const {
     return "One or more files. Can include wildcards such as *, `[0-4]`, etc...";
  };
};

#endif
