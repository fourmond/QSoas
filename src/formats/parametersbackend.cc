/**
   @file parametersbackend.cc: backend to load parameter files as data
   Copyright 2017 by CNRS/AMU

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

/// A class to read QSoas saved parameters file
class ParametersBackend : public DataBackend {
protected:

  virtual int couldBeMine(const QByteArray & peek, 
                          const QString & fileName) const override {
    QString s(peek);
    int p = 0;
    if(peek.contains("# The following information are comments, but Soas"))
      p += 450;
    if(peek.contains("# QSoas saved parameters"))
      p += 450;
    QRegExp re1("^# Fit used:");
    if(re1.indexIn(s) >= 0)
      p += 20;
    
    QRegExp re2("^[][#\\w]+\t[\\d.e+-]+\t!");
    if(re2.indexIn(s) >= 0)
      p += 150;
    return p;

  };

  // virtual ArgumentList * loadOptions() const {
  //   return NULL;
  // };

  virtual QList<DataSet *> readFromStream(QIODevice * stream,
                                          const QString & fileName,
                                          const CommandOptions & opts) const override {
    QList<DataSet *> rv;
    FitParametersFile pms;
    QTextStream in(stream);
    pms.readFromStream(in);
    QHash<QString,DataSet> vals = pms.parameterValuesAsfZ(true);
    QList<Vector> cols;
    QStringList cns;
    cns << "Z";
    for(const QString & n : pms.parametersOrder) {
      const DataSet & ds = vals[n];
      if(ds.nbColumns() == 0)//  {
        // QTextStream o(stdout);
        // o << "Missing values for " << pms.parametersOrder[i] << endl;
        continue;
      // }
      if(cols.size() == 0)
        cols << ds.x();
      cols << ds.y();
      cns << n;
    }
    DataSet * nds = new DataSet(cols);
    nds->name = QDir::cleanPath(fileName);
    nds->setMetaData("fit_name", pms.fitName);
    nds->columnNames << cns;
    for(int k : pms.bufferNames.keys())
      nds->setRowName(k, pms.bufferNames[k]);

    QRegExp resRE("^\\s*Final residuals:\\s+(\\S+)");
    for(const QString & c : pms.comments) {
      if(resRE.indexIn(c) == 0) {
        nds->setMetaData("residuals", resRE.cap(1).toDouble());
        break;
      }
    }
    
    setMetaDataForFile(nds, fileName);
    rv << nds;
    
    return rv;
  };

public:
  ParametersBackend() : DataBackend("parameters", "QSoas parameters",
                                    "Parameter files saved in fits") { 
  };
};

static ParametersBackend pb;
