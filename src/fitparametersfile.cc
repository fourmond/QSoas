/*
  fitparametersfile.cc: implementation of the FitParametersFile class
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014, 2015, 2017 by CNRS/AMU

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
#include <fitparametersfile.hh>
#include <fitparameter.hh>
#include <dataset.hh>
#include <terminal.hh>


void FitParametersFile::Parameter::replaceParameter(FitParameter * & parameter,
                                                    double * tg, 
                                                    int idx, int ds) const
{
  FitParameter * npm = 
    FitParameter::loadFromString(value, tg, idx, ds);
  delete parameter;
  parameter = npm;
}


void FitParametersFile::readFromStream(QTextStream & in)
{
  // The \\S really shouldn't be necessary, but it looks like a Qt
  // regexp bug ?
  QRegExp paramRE("^([^\t []+)\\s*(?:\\[#(\\d+)\\])?\t\\s*(\\S.*)");
  QRegExp commentRE("^\\s*#\\s*(.*)$");
  QRegExp bufferCommentRE("^\\s*Buffer\\s*#(\\d+)\\s*:\\s(.*)");
  QRegExp blankLineRE("^\\s*$");

  QString line;

  int nb = 0;

  int nbFailed = 0;
    
  while(true) {
    line = in.readLine();
    if(line.isNull())
      break;                    // EOF
    nb++;
    if(paramRE.indexIn(line) == 0) {
      int ds = -1;
      if(! paramRE.cap(2).isEmpty())
        ds = paramRE.cap(2).toInt();
      parameters << Parameter(paramRE.cap(1), ds, paramRE.cap(3));
      QString pn = paramRE.cap(1);
      if(! definedParameters.contains(pn)) {
        parametersOrder << pn;
        definedParameters += pn;
      }
      if(pn == "buffer_name" && ds >= 0) {
        bufferByName[paramRE.cap(3)] = ds;
        bufferNames[ds] = paramRE.cap(3);
      }
      if(pn == "Z" && ds >= 0) {
        bufferByZ[paramRE.cap(3)] = ds;
        bufferZValues[ds] = paramRE.cap(3).toDouble();
      }
    }
    else if(commentRE.indexIn(line) == 0) {
      QString cmt = commentRE.cap(1);
      comments << cmt;
      if(bufferCommentRE.indexIn(cmt) == 0) {
        // We found a comment describing the buffers
        int idx = bufferCommentRE.cap(1).toInt();
        bufferByName[bufferCommentRE.cap(2)] = idx;
      }

      // Parse comments, if possible.
    }
    else if(blankLineRE.indexIn(line) == 0) {
      continue;
    }
    else {
      Terminal::out << "Line #" << nb << " not understood: '" 
                    << line.trimmed() << "'" << endl;
      if(++nbFailed > 500)
        throw RuntimeError("Too many errors trying to read the parameters file, aborting");
    }
  }

  // Now we output the list of files we detected:
  // QList<int> lst = datasetNames.keys();
  // for(int i = 0; i < lst.size(); i++) {
  //   int idx = lst[i];
  //   o << "Buffer #" << idx << " was " << datasetNames[idx] << endl;
  // }
}

QHash<QString,DataSet> FitParametersFile::parameterValuesAsfZ(bool makeupZ) const
{
  /// @todo That's quite high-level, and not very optimized, but it
  /// shouldn't really matter so much.
  // First, preprocess the parameters as hashes index -> value
  QHash<QString, DataSet> rv;
  if(! hasPerpendicularCoordinates() && !makeupZ)
    return rv;

  QHash<int, double> perp;
  QHash<QString, QHash<int, double> > values;
  for(int i = 0; i < parameters.size(); i++) {
    const Parameter & p = parameters[i];
    bool ok = false;
    double val = p.toDouble(&ok);
    if(ok) {
      values[p.name][p.datasetIndex] = val;
      perp[p.datasetIndex] = p.datasetIndex;
    }
  }

  if(hasPerpendicularCoordinates()) 
    perp = values["Z"];
  
  QSet<int> zidx = perp.keys().toSet();
  zidx -= -1;

  if(zidx.isEmpty())            // This is the case for a
                                // single-buffer fit.
    zidx.insert(0);
  
  values.take("Z");
  values.take("buffer_weight");
  QStringList names = values.keys();
  for(int i = 0; i < names.size(); i++) {
    const QHash<int, double> & v = values[names[i]];
    QSet<int> vv = v.keys().toSet();
    QSet<int> vls;
    if(vv.contains(-1))
      vls = zidx;
    else
      vls = vv.intersect(zidx);
    QList<int> vls2 = vls.toList();
    qSort(vls2);
    Vector xv, yv;
    double def = v.value(-1, 0); // the 0 should never be needed.
    for(int j = 0; j < vls2.size(); j++) {
      int idx = vls2[j];
      xv << perp[idx];
      yv << v.value(idx, def);
    }
    rv[names[i]] = DataSet(xv,yv);
  }

  return rv;
}



void FitParametersFile::dump(QTextStream & o) const
{
  for(const Parameter & p : parameters)
    o << p.name << " -- " << p.datasetIndex << " -- " << p.value << endl;
}
