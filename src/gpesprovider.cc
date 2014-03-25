/*
  conditionsprovider.cc: meta-data provider based on "condition files"
  Copyright 2014 by Vincent Fourmond

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
#include <metadataprovider.hh>
#include <utils.hh>

/// Provider 
class GPESProvider : public MetaDataProvider {
protected:

  QString metaFile(const QString & fileName) const {
    QString s = fileName;
    // That's what I call ugly ;-)...
    if(s[s.size() - 4] == '.' && s[s.size() - 3] == 'o') {
      s[s.size() - 3] = 'i';
      return s;
    }
    return QString();
  }

public:

  /// Whether or not the provider has meta-data for the give file.
  virtual bool handlesFile(const QString & fileName) const {
    QString s = metaFile(fileName);
    if(! s.isEmpty() &&  QFile::exists(s))
      return true;
    return false;
  };

  /// Returns the meta-data for the given file
  virtual ValueHash metaDataForFile(const QString & fileName) const {
    QString mf = metaFile(fileName);

    QStringList lines;
    ValueHash ret;
    try {
      QFile f(mf);
      Utils::open(&f, QIODevice::ReadOnly);
      lines = Utils::readAllLines(&f);
    }
    catch(const RuntimeError & re) {
      return ret;
    }
    
    switch(mf[mf.size()-2].toLatin1()) {
    case 'c': {                 // staircase voltammetry
      ret["method"] = "staircase voltammetry";
      ret["sr"] = lines[65].toDouble();
      ret["cycles"] = lines[8].toInt();
      ret["step"] = lines[64].toDouble();
      ret["E_start"] = lines[61].toDouble();
      ret["E_first"] = lines[62].toDouble();
      ret["E_second"] = lines[63].toDouble();
      return ret;
    }
    case 'x': {                  // chronoamperometry
      /// @todo equilibration time and the like
      ret["method"] = "chronoamperometry";
      int nbsteps = lines[61].toInt();
      ret["steps"] = nbsteps;
      double t = 0;
      QList<QVariant> pots, times, dts;
      for(int i = 0; i < nbsteps; i++) {
        double pot = lines[142 + i].toDouble();
        ret[QString("E_%1").arg(i)] = pot;
        pots << pot;
        double dt = lines[101 + i].toDouble();
        dts << dt;
        ret[QString("delta_t_%1").arg(i)] = dt;
        ret[QString("t_%1").arg(i)] = t;
        times << t;
        t += dt;
      }
      ret["potentials"] = pots;
      ret["times"] = times;
      ret["lengths"] = dts;
      return ret;
    }
    }
    return ret;
  };
  
  GPESProvider() : MetaDataProvider("GPES")
  {
  };

  
};


static GPESProvider provider;