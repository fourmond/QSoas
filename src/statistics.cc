/*
  statistics.cc: implementation of the Statistics class
  Copyright 2013 by Vincent Fourmond

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
#include <statistics.hh>

#include <dataset.hh>

Statistics::Statistics(const DataSet * ds) : source(ds)
{
}

Statistics::~Statistics()
{
  for(int i = 0; i < segs.size(); i++)
    delete segs[i];
}

void Statistics::internalStats(ValueHash * overall, 
                               QList<ValueHash> * byColumn)
{
  // This is the real job.
  if(overall) {
    (*overall) << "buffer" << source->name;
    (*overall) << "rows" << source->nbRows();
    (*overall) << "columns" << source->nbColumns();
  }


  QStringList names = source->columnNames();
  for(int i = 0; i < source->nbColumns(); i++) {
    const QString & n = names[i];
    const Vector & c = source->column(i);
    double a,v;
    c.stats(&a, &v);

    ValueHash stats;
    stats << QString("%1_first").arg(n) << c.first()
          << QString("%1_last").arg(n) << c.last()
          << QString("%1_min").arg(n) << c.min()
          << QString("%1_max").arg(n) << c.max()
          << QString("%1_average").arg(n) << a
          << QString("%1_med").arg(n) << c.median() /// @todo get rid
                                                    /// if too slow ?
          << QString("%1_var").arg(n) << v
          << QString("%1_norm").arg(n) << c.norm();

    if(i > 0)                   // Integrate
      stats << QString("%1_int").arg(n) 
            << Vector::integrate(source->x(), c);
    if(byColumn)
      *byColumn << stats;
    if(overall)
      overall->merge(stats);
  }
}

ValueHash Statistics::stats()
{
  ValueHash ret;
  internalStats(&ret, NULL);
  return ret;
}


QList<ValueHash> Statistics::statsByColumns(ValueHash * overall)
{
  QList<ValueHash> ret;
  internalStats(overall, &ret);
  return ret;
}

QList<ValueHash> Statistics::statsBySegments(ValueHash * overall)
{
  if(segs.size() == 0)
    segs = source->chopIntoSegments();
  QList<ValueHash> ret;

  if(overall)
    internalStats(overall, NULL);

  for(int i = 0; i < segs.size(); i++) {
    Statistics s(segs[i]);
    ret << s.stats();
  }
  return ret;
}
