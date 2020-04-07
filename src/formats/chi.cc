/*
  textbackend.cc: implementation of the TextBackend class
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
#include <textbackend.hh>
#include <dataset.hh>

#include <utils.hh>
#include <exceptions.hh>

#include <linereader.hh>
#include <valuehash.hh>

/// Class to load text files from CH instruments. At least for what I
/// understand.
class CHIText : public TextBackend {
protected:
  virtual int couldBeMine(const QByteArray & peek, 
                          const QString & /*fileName*/) const {
    // We detect
    // Data Source: Experiment
    // Instrument Model:  CHI420A
    QString s(peek);
    QRegExp re1("Instrument\\s+Model:");
    QRegExp re2("Data\\s+Source:");

    if(re1.indexIn(s) >= 0 && re2.indexIn(s) >= 0) {
      return 800;
    }
    return 0;
  };

  virtual TextFileHeader parseComments(const QStringList & comments)
    const override {
    TextFileHeader rv;

    QRegExp cmtval("^([^:=]+)\\s*[:=]\\s*(.*)");
    for(int i = 1; i < comments.size(); i++) {
      const QString & s = comments[i];
      if(cmtval.indexIn(s) >= 0) {
        QString key = cmtval.cap(1).trimmed();
        QString value = cmtval.cap(2);
        if(value.trimmed().size() == 0)
          continue;
        bool ok;
        double v = value.toDouble(&ok);
        if(ok)
          rv.meta[key] = v;
        else
          rv.meta[key] = value;
      }
    }

    // Extract scan rates.
    if(rv.meta.contains("Scan Rate (V/s)"))
      rv.meta["sr"] = rv.meta["Scan Rate (V/s)"];

    return rv;
    
  };
public:
  
  CHIText() : TextBackend("/\\s*,\\s*|\\s+/", "chi-txt", "CHI text",
                          "Text files exported by CH instruments potentiostats")
  {
    
  };
};



CHIText chi;



                
