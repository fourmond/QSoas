/**
   \file statistics.hh
   The Statistics class, providing statistics (hey, that's true !)
   Copyright 2013 by CNRS/AMU

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
                     QList<ValueHash> * byColumn);

public:

  Statistics(const DataSet * s);
  ~Statistics();

  /// Returns the overall statistics
  ValueHash stats();

  /// Returns column-by-column statistics, and the overall statistics
  /// in the target value hash if not NULL.
  QList<ValueHash> statsByColumns(ValueHash * overall = NULL);

  /// Returns segment-by-segments statistics, and the overall
  /// statistics in the target value hash if not NULL.
  QList<ValueHash> statsBySegments(ValueHash * overall = NULL);
  
};

#endif
