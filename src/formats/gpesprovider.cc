/*
  gpesprovider.cc: meta-data provider for GPES-based data files
  Copyright 2014, 2015 by CNRS/AMU

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

#include <terminal.hh>

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
    // Common stuff, date:

    try {
      if(lines.size() < 225)
        throw RuntimeError("Not enough lines in GPES meta-data file");
         
    
      // Try to parse dates...
      QTime time = QTime::fromString(lines[203]);
      ret["exp_time"] = time;

      QRegExp dre("(\\d+)/(\\d+)/(\\d+)");
      if(dre.indexIn(lines[202]) == 0) {
        QDate date = QDate(dre.cap(3).toInt(), dre.cap(2).toInt(),
                           dre.cap(1).toInt());
        QDateTime dt(date, time);
        ret["exp_date"] = dt;
      }
      ret["title"] = lines[210];
      ret["comments"] = lines[211] + lines[212];
      ret["gpes_file"] = lines[224];

      ret["t_cond"] = lines[11].toDouble();
      ret["t_eq"] = lines[10].toDouble();
      ret["E_cond"] = lines[12].toDouble();
      ret["E_stby"] = lines[20].toDouble();

      // Filter
      double flt = 0;
      switch(lines[55].toInt()) {
      case 2:
        flt = 0.1;
        break;
      case 3:
        flt = 1;
        break;
      case 4:
        flt = 5;
        break;
      }
      ret["filter_time"] = flt;
    
      switch(mf[mf.size()-2].toLatin1()) {
      case 'c': {                 // staircase voltammetry
        ret["method"] = "staircase voltammetry";
        ret["sr"] = lines[65].toDouble();
        ret["cycles"] = lines[8].toInt();
        ret["step"] = lines[64].toDouble();
        ret["E_start"] = lines[61].toDouble();
        ret["E_first"] = lines[62].toDouble();
        ret["E_second"] = lines[63].toDouble();

        QRegExp cre("(\\d{5})\\.ocw$");
        if(cre.indexIn(fileName) > 0)
          ret["cycle"] = cre.cap(1).toInt();

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
          double pot = lines[141 + i].toDouble();
          ret[QString("E_%1").arg(i)] = pot;
          pots << pot;
          double dt = lines[100 + i].toDouble();
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
    }
    catch (const Exception & e) {
      Terminal::out << "(meta-data file '" << mf
                    << "' could not be read by QSoas, it is probably corrupted)";
    }
    return ValueHash(); 
  };
  
  GPESProvider() : MetaDataProvider("GPES")
  {
  };

  
};


static GPESProvider provider;
