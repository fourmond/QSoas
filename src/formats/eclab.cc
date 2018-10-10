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
class ECLabASCII : public TextBackend {
protected:
  virtual int couldBeMine(const QByteArray & peek, 
                          const QString & fileName) const override;

  virtual QList<DataSet *> readFromStream(QIODevice * stream,
                                          const QString & fileName,
                                          const CommandOptions & opts) const override;

public:
  ECLabASCII();
};

QList<DataSet *> ECLabASCII::readFromStream(QIODevice * stream,
                                            const QString & fileName,
                                            const CommandOptions & opts) const
{
  ValueHash meta;
  int pks = 1000;
  QByteArray peek = stream->peek(pks); 
  QString s(peek);
  QRegExp hd("Nb header lines : (\\d+)");

  CommandOptions o(opts);

  // If no
  if(hd.indexIn(s) >= 0) {
    int nbl = hd.cap(1).toInt();
    peek = stream->peek(nbl * 100); 
    s = peek;
    QTextStream txt(&s);
    LineReader in(&txt);

    int nbr = 0;
    // QTextStream o(stdout);
    while(! in.atEnd() && nbr < nbl) {
      ++nbr;
      QString line = in.readLine();
      QRegExp re("\\s*([^:]+):\\s*(.*)");
      
      if(re.indexIn(line, 0) == 0) {
        QString key = re.cap(1);
        QString val = re.cap(2);

        // o << "Key: '" << key << "' -> '" << val <<  "'" << endl;

        if(key == "Acquisition started on ") {
          QRegExp red("(\\d+)/(\\d+)/(\\d+) (\\d+):(\\d+):(\\d+)");
          if(red.indexIn(val) >= 0) {
            QDateTime dt(QDate(red.cap(3).toInt(),
                               red.cap(1).toInt(),
                               red.cap(2).toInt()),
                         QTime(red.cap(4).toInt(),
                               red.cap(5).toInt(),
                               red.cap(6).toInt()));
            meta["exp-date"] = dt;
          }
        }
      }
      else {
        if(nbr == 4)
          meta["method"] = line;
      }
    }
  }

  
  QList<DataSet *> dss = TextBackend::readFromStream(stream, fileName, o);
  for(int i = 0; i < dss.size(); i++)
    dss[i]->addMetaData(meta);

  return dss;
}

int ECLabASCII::couldBeMine(const QByteArray & peek, 
                            const QString & /*fileName*/) const
{
  // We detect
  // EC-Lab ASCII FILE
  QString s(peek);
  QRegExp re1("EC-Lab ASCII FILE");
  QRegExp re2("Nb header lines");

  if(re1.indexIn(s) >= 0 && re2.indexIn(s) >= 0) {
    return 800;
  }
  return 0;
}


ECLabASCII::ECLabASCII() :
  TextBackend("/\\s+/", "eclab-ascii",
              "EC-Lab ASCII files",
              "Text files exported by EC-Labs potentiostats")
{
  
}

ECLabASCII eclab;



                
