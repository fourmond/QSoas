/**
   \file statistics.hh
   The Statistics class, providing statistics (hey, that's true !)
   Copyright 2013, 2016 by CNRS/AMU

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
#ifndef __STATISTICS_HH
#define __STATISTICS_HH


#include <valuehash.hh>

class DataSet;

/// One or several statistics values applying to one buffer.
/// 
class StatisticsValue {
protected:
  static QList<StatisticsValue *> * allStats;

  void registerSelf();

  friend class Statistics;

public:

  StatisticsValue();
  
  virtual ~StatisticsValue() {
  };
  /// Whether the statistics are available for the given dataset and column.
  ///
  /// The column is ignored for global stats.
  virtual bool available(const DataSet * ds, int col) const = 0;

  /// Whether the stat is global or pertaining to a column.
  virtual bool global() const = 0;

  /// The suffixes of the stats, or the name of the stats for global
  /// stats.
  virtual QStringList suffixes() const = 0; 

  /// Returns the statistics for the given column, in the order in
  /// which they are returned in the suffixes() function.
  virtual QList<QVariant> values(const DataSet * ds, int col) const = 0;

  /// Returns a descriptive "bullet point" text for the stats handled
  /// by the object.
  virtual QString description() const = 0;

  /// Returns all the statistics available for the given dataset.
  ///
  /// @warning Not necessarily in the same order as the stats
  static QStringList statsAvailable(const DataSet * ds, bool useNames = false);

  /// Returns the suffixes of all the available stats.
  static QStringList allSuffixes();

  /// Returns a document string made from the description() texts
  static QString docString();
};

/// Provides various statistics about a dataset.
class Statistics {
  /// The original dataset
  const DataSet * source;

  /// When applicable, holds the segment decomposition of the \a
  /// source.
  QList<DataSet *> segs;

  /// Computes the statistics and store them in the given pointers
  /// (that may be NULL)
  void internalStats(ValueHash * overall, 
                     QList<ValueHash> * byColumn,
                     bool useNames = false);

public:

  explicit Statistics(const DataSet * s);
  ~Statistics();

  /// Returns the overall statistics
  ValueHash stats(bool useNames = false);

  /// Returns column-by-column statistics, and the overall statistics
  /// in the target value hash if not NULL.
  QList<ValueHash> statsByColumns(ValueHash * overall = NULL,
                                  bool useNames = false);

  /// Returns segment-by-segments statistics, and the overall
  /// statistics in the target value hash if not NULL.
  QList<ValueHash> statsBySegments(ValueHash * overall = NULL,
                                   bool useNames = false);


  /// Returns a Ruby hash containing the statistics (including
  /// sub-hashes containing stats by segments when applicable)
  mrb_value toRuby(bool useNames = false);
  
};

#endif
