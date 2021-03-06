/**
   @file ignorebackend.cc: backend that autodetects non-data file and claim them while refusing to load them
   Copyright 2021 by CNRS/AMU

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
#include <fitparametersfile.hh>
#include <dataset.hh>

#include <argumentlist.hh>
#include <argumentmarshaller.hh>
#include <general-arguments.hh>

#include <utils.hh>
#include <exceptions.hh>

/// A class that attempts to detect files which are not data, such as:
/// @li QSoas's .qsm files
/// @li PDF files
/// @li ...
/// They are detected but not loaded, so as to avoid spending too much time reading useless files.
class IgnoreBackend : public DataBackend {
protected:

  /// Returns a string containing the detected format, or just an
  /// empty string if the format was not detected.
  static QString detectFormat(const QByteArray & peek) {
    if(peek.startsWith("// QSoas JSON meta-data"))
       return "QSoas meta data";
    if(peek.startsWith("%PDF-1."))
      return "PDF";
    return QString();
  };

  virtual int couldBeMine(const QByteArray & peek, 
                          const QString & fileName) const override {
    QString f = detectFormat(peek);
    if(! f.isEmpty())
      return 900;
    return 0;
  };

  virtual QList<DataSet *> readFromStream(QIODevice * stream,
                                          const QString & fileName,
                                          const CommandOptions & opts) const override {
    QList<DataSet *> rv;
    QByteArray peek = stream->peek(4096);
    throw RuntimeError("File '%1' is not data, but a %2 file, ignoring.\n  -> if you think the detection is wrong, use the appropriate load-as-xxx command").
      arg(fileName).arg(detectFormat(peek));
    return rv;
  };

public:
  IgnoreBackend() : DataBackend("ignore", "Ignore files",
                                "recognizes files but does not load them") { 
  };
};

static IgnoreBackend pb;
