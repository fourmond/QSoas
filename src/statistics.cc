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
#include <mruby.hh>

// First, implementation of the StatisticsValue class and subclasses.

QList<StatisticsValue *> * StatisticsValue::allStats = NULL;

StatisticsValue::StatisticsValue()
{
  registerSelf();
}

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

QStringList StatisticsValue::allSuffixes()
{
  QStringList ret;
  if(allStats)
    for(int i = 0; i < allStats->size(); i++)
      ret += allStats->value(i)->suffixes();
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

/// A class representing a series of stats, with a lambda
class MultiLambdaStat : public StatisticsValue {
protected:
  /// The names
  QStringList names;

  /// Global
  bool isGlobal;

  /// Whether it needs an X column (i.e. only for Y columns)
  bool needX;

  typedef std::function<QList<QVariant> (const DataSet *, int) > Lambda;

  Lambda function;
  
public:
  MultiLambdaStat(const QStringList & n,
                   bool glb, bool nx, Lambda fn) :
    names(n), isGlobal(glb), needX(nx), function(fn)
  {
  }

  virtual bool available(const DataSet * /*ds*/, int col) const override {
    return isGlobal || col > 0 || (col == 0 && (!needX));
  };

  virtual QStringList suffixes() const override {
    return names;
  };

  virtual bool global() const override {
    return isGlobal;
  };

  virtual QList<QVariant> values(const DataSet * ds, int col) const override {
    return function(ds, col);
  };
};


//////////////////////////////////////////////////////////////////////

static MultiLambdaStat gen(QStringList()
                           << "buffer"
                           << "rows"
                           << "columns"
                           << "segments", true, false,
                           [](const DataSet * ds, int /*c*/) -> QList<QVariant>
                           {
                             QList<QVariant> rv;
                             rv << ds->name
                                << ds->nbRows()
                                << ds->nbColumns()
                                << ds->segments.size();
                             return rv;
                           });

static MultiLambdaStat avg(QStringList()
                           << "sum"
                           << "average"
                           << "var"
                           << "stddev", false, false,
                           [](const DataSet * ds, int c) -> QList<QVariant>
                           {
                             QList<QVariant> rv;
                             double a,v,s;
                             ds->column(c).stats(&a, &v, &s);
                             rv << s
                                << a
                                << v
                                << sqrt(v);
                             return rv;
                           });




    //   stats << QString("%1_min_pos").arg(n)
    //         << source->x()[c.whereMin()]
    //         << QString("%1_max_pos").arg(n)
    //         << source->x()[c.whereMax()];
    // }

static SingleLambdaStat fst("first", false, false,
                            [](const DataSet * ds, int c) -> QVariant
                            {
                              return ds->column(c).first();
                            });

static SingleLambdaStat lst("last", false, false,
                            [](const DataSet * ds, int c) -> QVariant
                            {
                              return ds->column(c).last();
                            });

static SingleLambdaStat mn("min", false, false,
                            [](const DataSet * ds, int c) -> QVariant
                            {
                              return ds->column(c).min();
                            });

static SingleLambdaStat mx("max", false, false,
                           [](const DataSet * ds, int c) -> QVariant
                           {
                             return ds->column(c).max();
                           });

// static SingleLambdaStat med("med", false, false,
//                             [](const DataSet * ds, int c) -> QVariant
//                             {
//                               return ds->column(c).median();
//                             });

static SingleLambdaStat nrm("norm", false, false,
                            [](const DataSet * ds, int c) -> QVariant
                            {
                              return ds->column(c).norm();
                            });


static SingleLambdaStat inte("int", false, true,
                             [](const DataSet * ds, int c) -> QVariant
                               {
                                 return Vector::integrate(ds->x(), ds->column(c));
                               });

static MultiLambdaStat med(QStringList()
                           << "med"
                           << "q10"
                           << "q25"
                           << "q75"
                           << "q90", false, false,
                           [](const DataSet * ds, int c)  -> QList<QVariant>
                           {
                             QList<QVariant> rv;
                             Vector v = ds->column(c);
                             qSort(v);
                             int nb = v.size();
                             rv << v[0.5 * nb]
                                << v[0.1 * nb]
                                << v[0.25 * nb]
                                << v[0.75 * nb]
                                << v[0.9 * nb];
                             return rv;
                           });

static MultiLambdaStat delta(QStringList()
                             << "delta_min"
                             << "delta_max", false, false,
                             [](const DataSet * ds, int c) -> QList<QVariant>
                             {
                               QList<QVariant> rv;
                               double dmin, dmax;
                               ds->column(c).deltaStats(&dmin, &dmax);
                               rv << dmin
                                  << dmax;
                               return rv;
                             });


static MultiLambdaStat reglin(QStringList()
                             << "a"
                             << "b", false, true,
                             [](const DataSet * ds, int c) -> QList<QVariant>
                             {
                               QList<QVariant> rv;
                               QPair<double, double> a;
                               a = ds->reglin(0, -1, c);
                               rv << a.first
                                  << a.second;
                               return rv;
                             });



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
  // We first sort the stats between global and not
  if(! StatisticsValue::allStats)
    return;
  QList<StatisticsValue*> globalStats;
  QList<StatisticsValue*> localStats;
  for(int i = 0; i < StatisticsValue::allStats->size(); i++) {
    StatisticsValue * v = StatisticsValue::allStats->value(i);
    if(v->global())
      globalStats << v;
    else
      localStats << v;
  }
  
  // This is the real job.
  if(overall) {
    for(int i = 0; i < globalStats.size(); i++) {
      StatisticsValue * v = globalStats[i];
      if(v->available(source, -1))
        overall->multiAdd(v->suffixes(), v->values(source, -1));
    }
  }

  QStringList names = source->columnNames();
  for(int i = 0; i < source->nbColumns(); i++) {
    const QString & n = names[i];
    ValueHash stats;

    for(int j = 0; j < localStats.size(); j++) {
      StatisticsValue * v = localStats[j];
      if(v->available(source, i)) {
        QStringList ns = v->suffixes();
        ns.replaceInStrings(QRegExp("^"), n + "_");
        stats.multiAdd(ns, v->values(source, i));
      }
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

mrb_value Statistics::toRuby()
{
  ValueHash s;
  QList<ValueHash> sstats = statsBySegments(&s);
  mrb_value hsh = s.toRuby();
  MRuby * mr = MRuby::ruby();

  for(int i = 0; i < sstats.size(); i++) {
    mrb_value v = sstats[i].toRuby();
    mr->hashSet(hsh, mr->newInt(i), v);
    mr->hashSet(hsh, mr->newFloat(i), v);
  }
  // rbw_gv_set("$stats", hsh);
  return hsh;
}
