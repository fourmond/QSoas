/*
  statistics.cc: implementation of the Statistics class
  Copyright 2013, 2014 by CNRS/AMU

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

// First, implementation of the StatisticsValue class and subclasses.

QList<StatisticsValue *> * StatisticsValue::allStats = NULL;

void StatisticsValue::registerSelf()
{
  if(! allStats)
    allStats = new QList<StatisticsValue *>;
  *allStats << this;
}

QStringList StatisticsValue::statsAvailable(const DataSet * ds)
{
  QStringList ret;
  if(! allStats)
    return ret;
  QStringList colnames = ds->columnNames();
  for(int i = 0; i < allStats->size(); i++) {
    const StatisticsValue * s = allStats->value(i);
    if(s->global() && s->available(ds, -1))
      ret += s->suffixes();
    else {
      QStringList cls = colnames;
      for(int i = cls.size()-1; i >= 0; i--) {
        if(! s->available(ds, i))
          cls.takeAt(i);
      }
      QStringList vrs = s->suffixes();
      for(int j = 0; j < cls.size(); j++) {
        for(int k = 0; k < vrs.size(); k++)
          ret << QString("%1_%2").arg(cls[j]).arg(vrs[k]);
      }
    }
  }
  return ret;
}

//////////////////////////////////////////////////////////////////////

/// A class representing a single stat, with a lambda
class SingleLambdaStat : public StatisticsValue {
protected:
  /// The name
  QString name;

  /// Global
  bool isGlobal;

  /// Whether it needs an X column (i.e. only for Y columns)
  bool needX;

  typedef std::function<QVariant (const DataSet *, int) > Lambda;

  Lambda function;
  
public:
  SingleLambdaStat(const QString & n,
                   bool glb, bool nx, Lambda fn) :
    name(n), isGlobal(glb), needX(nx), function(fn)
  {
  }

  virtual bool available(const DataSet * /*ds*/, int col) const override {
    return isGlobal || col > 0 || (col == 0 && (!needX));
  };

  virtual QStringList suffixes() const override {
    QStringList re;
    re << name;
    return re;
  };

  virtual bool global() const override {
    return isGlobal;
  };

  virtual QList<QVariant> values(const DataSet * ds, int col) const override {
    QList<QVariant> var;
    var << function(ds, col);
    return var;
  };
};


//////////////////////////////////////////////////////////////////////

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
    (*overall) << "segments" << source->segments.size();
  }


  QStringList names = source->columnNames();
  for(int i = 0; i < source->nbColumns(); i++) {
    const QString & n = names[i];
    const Vector & c = source->column(i);
    double a,v;
    c.stats(&a, &v);
    double dmin, dmax;
    c.deltaStats(&dmin, &dmax);

    ValueHash stats;
    stats << QString("%1_first").arg(n) << c.first()
          << QString("%1_last").arg(n) << c.last()
          << QString("%1_min").arg(n) << c.min()
          << QString("%1_max").arg(n) << c.max()
          << QString("%1_average").arg(n) << a
          << QString("%1_med").arg(n) << c.median() /// @todo get rid
                                                    /// if too slow ?
          << QString("%1_var").arg(n) << v
          << QString("%1_norm").arg(n) << c.norm()
          << QString("%1_delta_min").arg(n) << dmin
          << QString("%1_delta_max").arg(n) << dmax
      ;

    if(i > 0) {                   // Integrate
      stats << QString("%1_int").arg(n) 
            << Vector::integrate(source->x(), c);
      stats << QString("%1_min_pos").arg(n)
            << source->x()[c.whereMin()]
            << QString("%1_max_pos").arg(n)
            << source->x()[c.whereMax()];
    }
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

RUBY_VALUE Statistics::toRuby()
{
  ValueHash s;
  QList<ValueHash> sstats = statsBySegments(&s);
  RUBY_VALUE hsh = s.toRuby();
  for(int i = 0; i < sstats.size(); i++) {
    RUBY_VALUE v = sstats[i].toRuby();
    rbw_hash_aset(hsh, rbw_long2num(i), v);
    rbw_hash_aset(hsh, rbw_float_new(i), v);
  }
  rbw_gv_set("$stats", hsh);
  return hsh;
}
