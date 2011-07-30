/**
   \file dataset.hh
   The DataSet class, representing a data set (most of the time one data file)
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

#ifndef __DATASET_HH
#define __DATASET_HH

#include <vector.hh>

/// A class representing a data set (formerly Soas buffers).
///
/// A DataSet represents a single data set, ie most of the time the
/// data contained in a single file. It contains a X column, a Y
/// column, and possibly additional columns whose use is left for
/// later.
///
/// @todo Add support for meta-data.
class DataSet {

  /// The columns
  QList<Vector> columns;

  /// A private cache
  class Cache {
  public:
    bool valid;
    Cache() : valid(false) {;};

    QVector<double> minima;
    QVector<double> maxima;
  };

  /// An internal cache to speed up various computations.
  mutable Cache cache;

  void invalidateCache() {
    cache.valid = false;
  };

  bool isCacheValid() const {
    return cache.valid;
  };

  /// Regenerate the cache
  void regenerateCache() const;

  /// Ensures that the cache is up-to-date
  void updateCache() const {
    if(! isCacheValid())
      regenerateCache();
  };
public:

  /// The name of the dataset, usually the name of the file.
  QString name;

  DataSet() {;};
  DataSet(const QList<Vector> & cols) : columns(cols) {;};


  /// Adds a new column to the data set.
  DataSet & operator<<(const Vector & column);

  Vector & x() {
    invalidateCache();
    return columns[0];
  };

  const Vector & x() const{
    return columns[0];
  };

  Vector & y() {
    invalidateCache();
    return columns[1];
  };

  const Vector & y() const{
    return columns[1];
  };

  /// Dump the data to standard output.
  ///
  /// @todo This may grow into something interesting later on, but not
  /// now.
  void dump() const;

  /// Return the number of rows.
  int nbRows() const {
    return x().size();
  };

  /// Returns the number of columns
  int nbColumns() const {
    return columns.size();
  };

  /// Returns a small text representing the data set.
  QString stringDescription() const;

  /// Returns the overall size used by the DataSet (not counting the
  /// QList overhead, probably much smaller than the rest anyway).
  int size() const;

  /// @name Data inspection facilites
  ///
  /// Functions that return various information about the data
  /// contained in the Vector.
  ///
  /// @{

  /// Returns the XY bounding box of the DataSet.
  QRectF boundingBox() const;


  /// Returns the distance of the \a x, \a y point to the curve, along
  /// with the index of the closest point
  QPair<double, int> distanceTo(double x, double y) const;
  

  /// Returns the distance of the \a point to the curve.
  QPair<double, int> distanceTo(const QPointF & p) const {
    return distanceTo(p.x(), p.y());
  };

  /// Returns the distance of the \a x, \a y point to the curve, along
  /// with the index of the closest point
  QPair<double, int> distanceTo(double x, double y, 
                                double xscale, double yscale) const;
  

  /// Returns the distance of the \a point to the curve.
  QPair<double, int> distanceTo(const QPointF & p, 
                                double xscale, double yscale) const {
    return distanceTo(p.x(), p.y(), xscale, yscale);
  };

  /// Returns the numbered point
  QPointF pointAt(int i) const {
    return QPointF(columns[0][i], columns[1][i]);
  };


  /// @}
  
};

#endif
