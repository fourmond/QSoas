/*
  origalys.cc: the Origalys potentiostat file format
  Copyright 2024 by CNRS/AMU

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

/// Class to load text files from Origalys potentiostat
class OrigalysBackend : public TextBackend {
protected:
  virtual int couldBeMine(const QByteArray & peek, 
                          const QString & /*fileName*/) const override {
    if(peek.size() > 3 && (unsigned char) peek[0] == 0xFE
       && (unsigned char) peek[1] == 0xAB &&
       peek[2] >= 0x30 && peek[2] <= 0x40)
      return 800;
    return 0;
  };

  virtual QList<DataSet *> readFromStream(QIODevice * stream,
                                          const QString & fileName,
                                          const CommandOptions & opts) const override {
    ValueHash meta;

    QByteArray bt = stream->peek(1024);
    QStringList lst;
    int nbl = 0;
    int cnt = 2;
    int last = 2;
    while(nbl < 9  && cnt < bt.size()) {
      if(bt[cnt] == 0x0A) {
        nbl += 1;
        lst << bt.mid(last, cnt-1-last);
        last = cnt + 1;
      }
      cnt += 1;
    }
    if(nbl < 9)
      throw RuntimeError("Truncated origalys header for file %1").
        arg(fileName);
    stream->seek(cnt+1);
    QTextStream o(stdout);
    o << "Lines: " << lst.join("\n\t") << endl;

    QStringList date = lst[7].split("/");
    if(date.size() == 9) {
      QDateTime dt(QDate(date[1].toInt(),
                         date[2].toInt(),
                         date[4].toInt()),
                   QTime(date[5].toInt(),
                         date[6].toInt(),
                         date[7].toInt(),
                         date[8].toInt()));
      meta["exp_date"] = dt;
      meta["exp_time"] = dt.time();
    }
    
    

    QList<DataSet *> dss = TextBackend::readFromStream(stream, fileName, opts);
    for(int i = 0; i < dss.size(); i++)
      dss[i]->addMetaData(meta);

    return dss;
  };

public:
  OrigalysBackend() :
    TextBackend("\t", "origalys",
                "Origalys files",
                "Text files for Origalys potentiostats") {

  };
};


static OrigalysBackend origalys;



                
