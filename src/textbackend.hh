/**
   \file textbackend.hh
   Backends for reading text files.
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

#ifndef __TEXTBACKEND_HH
#define __TEXTBACKEND_HH

#include <databackend.hh>


/// A general-purpose text files reader.
class TextBackend : public DataBackend {
protected:

  /// The column separator
  QRegExp separator;
  virtual int couldBeMine(const QByteArray & peek, 
                          const QString & fileName) const;

public:
  TextBackend(const QRegExp & sep,
              const char * n, const char * pn, const char * d = "");
  virtual DataSet * readFromStream(QIODevice * stream,
                                   const QString & fileName,
                                   const CommandOptions & opts) const;
};

#endif
