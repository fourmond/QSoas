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

QStringList StatisticsValue::statsAvailable(const DataSet * ds, bool useNames)
{
  QStringList ret;
  if(! allStats)
    return ret;
  QStringList colnames = useNames ?
    ds->mainColumnNames()
    : ds->standardColumnNames();
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

QString StatisticsValue::docString()
{
  QStringList strs;
  if(allStats) {
    for(const StatisticsValue * s : *allStats)
      strs << s->description();
  }
  return strs.join("\n");
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

  QString desc;
  
public:
  SingleLambdaStat(const QString & n,
                   bool glb, bool nx, Lambda fn,
                   const QString & d) :
    name(n), isGlobal(glb), needX(nx), function(fn), desc(d)
  {
  }

  virtual bool available(const DataSet * ds, int col) const override {
    if(isGlobal)
      return true;
    // Empty datasets don't have local stats, I think.
    if(ds->nbRows() == 0)
      return false;
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

  virtual QString description() const {
    QString pref = name;
    if(! isGlobal) {
      if(needX)
        pref = "y_" + pref;
      else
        pref = "_" + pref;
    }
    return QString("* `%1`: %2").arg(pref).arg(desc);
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

  QString desc;
  
public:
  MultiLambdaStat(const QStringList & n,
                  bool glb, bool nx, Lambda fn, const QString & d) :
    names(n), isGlobal(glb), needX(nx), function(fn), desc(d)
  {
  }

  virtual bool available(const DataSet * ds, int col) const override {
    if(isGlobal)
      return true;
    // Empty datasets don't have local stats, I think.
    if(ds->nbRows() == 0)
      return false;
    return col > 0 || (col == 0 && (!needX));
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

  virtual QString description() const {
    QStringList prefs;
    for(QString pref : names) {
      if(! isGlobal) {
        if(needX)
          pref = "y_" + pref;
        else
          pref = "_" + pref;
      }
      prefs << pref;
    }
    return QString("* `%1`: %2").arg(prefs.join("`, `")).arg(desc);
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
                           },
                           "the buffer name, and the row, column and segment counts.");

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
                           }, "the sum, the average, the variance and the standard deviation of the values of the column.");




    //   stats << QString("%1_min_pos").arg(n)
    //         << source->x()[c.whereMin()]
    //         << QString("%1_max_pos").arg(n)
    //         << source->x()[c.whereMax()];
    // }

static MultiLambdaStat fl(QStringList()
                          << "first"
                          << "last", false, false,
                          [](const DataSet * ds, int c) -> QList<QVariant>
                          {
                            QList<QVariant> rv;
                            rv << ds->column(c).first()
                               << ds->column(c).last();
                            return rv;
                          },
                          "the first and last values of the column.");

// static SingleLambdaStat lst("last", false, false,
//                             [](const DataSet * ds, int c) -> QVariant
//                             {
//                               return ;
//                             });

static MultiLambdaStat mnx(QStringList()
                           << "min"
                           << "max"
                           , false, false,
                           [](const DataSet * ds, int c) -> QList<QVariant>
                            {
                              QList<QVariant> rv;
                              rv << ds->column(c).min()
                                 << ds->column(c).max();
                              return rv;
                            }, "the minimum and maximum values of the column.");

static SingleLambdaStat nrm("norm", false, false,
                            [](const DataSet * ds, int c) -> QVariant
                            {
                              return ds->column(c).norm();
                            }, "the norm of the column, that is $$\\sqrt{\\sum {x_i}^2}$$.");


static SingleLambdaStat inte("int", false, true,
                             [](const DataSet * ds, int c) -> QVariant
                               {
                                 return Vector::integrate(ds->x(), ds->column(c));
                               }, "the integral of the Y values over the X values.");

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
                           },
                           "the median, and the 10th, 25th, 75th and 90th percentiles.");

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
                             },
                             "the min and max values of the difference between two successive values.");


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
                             },
                              "the linear regression coefficients of the Y column over X: `a` is the slope and `b` the value at 0.");



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
                               QList<ValueHash> * byColumn,
                               bool useNames)
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
  /// @todo Cache this ? 
  if(overall) {
    for(int i = 0; i < globalStats.size(); i++) {
      StatisticsValue * v = globalStats[i];
      if(v->available(source, -1))
        overall->multiAdd(v->suffixes(), v->values(source, -1));
    }
  }

  QStringList names = useNames ?
    source->mainColumnNames()
    : source->standardColumnNames();
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

ValueHash Statistics::stats(bool useNames)
{
  ValueHash ret;
  internalStats(&ret, NULL, useNames);
  return ret;
}


QList<ValueHash> Statistics::statsByColumns(ValueHash * overall, bool useNames)
{
  QList<ValueHash> ret;
  internalStats(overall, &ret, useNames);
  return ret;
}

QList<ValueHash> Statistics::statsBySegments(ValueHash * overall,
                                             bool useNames)
{
  QList<ValueHash> ret;
  if(source->segments.size() > 0) {
    if(segs.size() == 0)
      segs = source->chopIntoSegments();

    if(overall)
      internalStats(overall, NULL, useNames);

    for(int i = 0; i < segs.size(); i++) {
      Statistics s(segs[i]);
      ret << s.stats(useNames);
    }
  }
  else {
    /// @todo This will not work properly when overall isn't empty.
    if(overall) {
      internalStats(overall, NULL, useNames);
      ret << *overall;
    }
    else {
      ret << ValueHash();
      internalStats(&(ret[0]), NULL, useNames);
    }
  }
  return ret;
}

mrb_value Statistics::toRuby(bool useNames)
{
  ValueHash s;
  QList<ValueHash> sstats = statsBySegments(&s, useNames);
  mrb_value hsh = s.toRuby();
  MRuby * mr = MRuby::ruby();

  for(int i = 0; i < sstats.size(); i++) {
    mrb_value v = sstats[i].toRuby();
    mr->hashSet(hsh, mr->newInt(i), v);
    mr->hashSet(hsh, mr->newFloat(i), v);
  }
  return hsh;
}
