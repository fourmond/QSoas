/**
   \file textbackend.hh
   Base class for text backends
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014, 2015 by CNRS/AMU

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
#include <databackend.hh>
#include <regex.hh>
#include <valuehash.hh>


class Vector;

/// A data structure that represents the header of a text file, which
/// can contain meta-data, the name of the columns, but also
/// @todo whether some columns are text or dates... (something else ?)
class TextFileHeader {
public:
  /// The meta-data
  ValueHash meta;

  /// The column names
  QStringList columnNames;
  
};

/// A general-purpose text files reader.
class TextBackend : public DataBackend {
protected:

  /// The column separator
  Regex separator;

  /// The comments lines
  Regex comments;
  virtual int couldBeMine(const QByteArray & peek, 
                          const QString & fileName) const override;

  virtual ArgumentList loadOptions() const override;

  virtual QList<DataSet *> readFromStream(QIODevice * stream,
                                          const QString & fileName,
                                          const CommandOptions & opts) const override;

  virtual TextFileHeader parseComments(const QStringList & cmts) const;


  virtual QList<QList<Vector> > readColumns(QTextStream & s,
                                            const CommandOptions & opts,
                                            QStringList * comments,
                                            const QList<int> & textColumns,
                                            QList<QList<QStringList> > * savedTexts) const;
public:
  TextBackend(const QString & sep,
              const char * n, const char * pn, const char * d = "");
};
