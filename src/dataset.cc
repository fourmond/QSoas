/*
  dataset.cc: implementation of the DataSet class
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

#include <headers.hh>
#include <dataset.hh>

#include <math.h>

void DataSet::dump() const
{
  QTextStream o(stdout);
  for(int i = 0; i < x().size(); i++)
    for(int j = 0; j < columns.size(); j++)
      o << columns[j][i] << (j == columns.size() - 1 ? "\n" : "\t");      
}

int DataSet::size() const
{
  /// @todo use the cache ?
  int s = 0;
  for(int i = 0; i < columns.size(); i++)
    s += columns[i].size();
  s *= sizeof(double);
  return s;
}

QString DataSet::stringDescription() const
{
  return QObject::tr("'%1': %2 columns, %3 rows, %4 bytes").
    arg(name).arg(nbColumns()).arg(nbRows()).arg(size());
}

void DataSet::regenerateCache() const
{
  if(isCacheValid())
    return;

  int size = columns.size();
  cache.minima.resize(size);
  cache.maxima.resize(size);

  for(int i = 0; i < size; i++) {
    cache.minima[i] = columns[i].min();
    cache.maxima[i] = columns[i].max();
  }
  
  cache.valid = true;
}

QRectF DataSet::boundingBox() const
{
  updateCache();
  QRectF r;
  r.setCoords(cache.minima[0], cache.minima[1], 
              cache.maxima[0], cache.maxima[1]);
  return r;
}


QPair<double, int> DataSet::distanceTo(double x, double y) const
{
  const double * xvals = columns[0].data();
  const double * yvals = columns[1].data();
  int nb = columns[0].size();
  if(nb == 0)
    return QPair<double, int>(0.0/0.0, -1);

  double dx = x - xvals[0];
  double dy = y - yvals[0];
  double dist = dx*dx + dy*dy;
  int best = 0;
  for(int i = 1; i < nb; i++) {
    dx = x - xvals[i];
    dy = y - yvals[i];
    double d = dx * dx + dy * dy;
    if(d < dist || dist != dist) {
      dist = d;
      best = i;
    }
  }

  return QPair<double, int>(sqrt(dist), best);
}

QPair<double, int> DataSet::distanceTo(double x, double y, 
                                       double xs, double ys) const
{
  const double * xvals = columns[0].data();
  const double * yvals = columns[1].data();
  int nb = columns[0].size();
  if(nb == 0)
    return QPair<double, int>(0.0/0.0, -1);

  double dx = (x - xvals[0])*xs;
  double dy = (y - yvals[0])*ys;
  double dist = dx*dx + dy*dy;
  int best = 0;
  for(int i = 1; i < nb; i++) {
    dx = (x - xvals[i])*xs;
    dy = (y - yvals[i])*ys;
    double d = dx * dx + dy * dy;
    if(d < dist || dist != dist) {
      dist = d;
      best = i;
    }
  }

  return QPair<double, int>(sqrt(dist), best);
}


QString DataSet::cleanedName() const
{
  int idx = name.lastIndexOf('.');
  if(idx > 0)
    return name.left(idx);
  return name;
}

void DataSet::splitAt(int idx, DataSet ** first, DataSet ** second) const
{
  if(first) {
    QList<Vector> newcols;
    for(int i = 0; i < columns.size(); i++)
      newcols << Vector(columns[i].mid(0, idx+1));
    /// @todo Shall we handle the name change here ? Why not, after
    /// all ? Things still can change later on.
    *first = new DataSet(newcols);
    (*first)->name = cleanedName() + "_a.dat";
  }
  if(second) {
    if(idx >= nbRows())         // Ensure we don't go over.
      idx = nbRows() - 1;
    QList<Vector> newcols;
    for(int i = 0; i < columns.size(); i++)
      newcols << Vector(columns[i].mid(idx));
    *second = new DataSet(newcols);
    (*second)->name = cleanedName() + "_b.dat";
  }
}

const double * DataSet::getValues(int col, int * size) const
{
  if(size)
    *size = columns[col].size();
  return columns[col].data();
}

int DataSet::deltaSignChange(int col) const
{
  int size;
  const double *val = getValues(col, &size);
  if(size < 3)
    return  -1;
  double dv0 = val[1] - val[0];
  for(int i = 1; i < size - 1; i++) {
    double dv = val[i+1] - val[i];
    if(dv * dv0 < 0)
      return i;
  }
  return -1;
}
