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
 
  /// Return the raw values of the given column, setting \a size to
  /// the correct value when not NULL.
  const double * getValues(int col, int * size) const;

  /// Applies a binary operation to all Y columns while trying to keep
  /// X matching. Returns a brand new DataSet.
  ///
  /// The operation takes quadratic time, as all X of a are matched
  /// against all X of b for each value (which is bad,
  /// admittedly). There are probably ways to be much more clever than
  /// that.
  ///
  /// Performance shouldn't
  static DataSet * applyBinaryOperation(const DataSet * a,
                                        const DataSet * b,
                                        double (*op)(double, double),
                                        const QString & cat = "_op_",
                                        bool naive = false);


  friend QDataStream & operator<<(QDataStream & out, const DataSet & ds);
  friend QDataStream & operator>>(QDataStream & in, DataSet & ds);

public:

  /// The name of the dataset, usually the name of the file.
  QString name;

  DataSet() {;};
  DataSet(const QList<Vector> & cols) : columns(cols) {;};

  /// Returns a version of name without the extension, that can be
  /// used for making up new buffer names from the old ones.
  QString cleanedName() const;

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

  
  /// Returns the numbered column
  const Vector & column(int i) const {
    return columns[i];
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
  int byteSize() const;

  /// @name Data inspection facilites
  ///
  /// Functions that return various information about the data
  /// contained in the Vector. Some of them are cached, others are
  /// not.
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

  /// Returns the first index at which the delta between two
  /// consecutive elements of column \a i change. -1 if there is no
  /// such change.
  int deltaSignChange(int i) const;


  /// Performs a linear regression on XY, taking only into account the
  /// data points within the given indices (-1 for end means the last
  /// element)
  QPair<double, double> reglin(int begin = 0, int end = -1) const;

  /// Performs a linear regression on XY, taking into account only the
  /// data whose X value is within the given range.
  QPair<double, double> reglin(double xmin, double xmax) const;

  /// Returns a "smoothed" point at the given index, looking at the
  /// given range of points.
  QPointF smoothPick(int idx, int range = -1) const;

  /// Returns the Y value the closest to the given X value
  double yValueAt(double x) const;

  /// Performs a B-splines smoothing of the dataset, using the given X
  /// values as knots (xmin and xmax are automatically added if they
  /// are missing)
  ///
  /// Returns the smoothed Y values.
  Vector bSplinesSmooth(int order, const Vector & xvalues, 
                        double * residuals = NULL, 
                        Vector * derivative = NULL) const;

  /// Finds steps in the Y data, based on the following heuristic: at
  /// each point, estimate the projection of the Y value between point
  /// i - 1 and point i by linear regression from the left and right
  /// side over \a nb points. If they disagree by more than \a
  /// threshold relatively to the overall Y range of the dataset,
  /// split (well, split at the point where they disagree the most
  /// over a range of \a nb).
  QList<int> findSteps(int nb, double threshold) const;

  /// @}

  /// @name Operations
  ///
  /// Operations that either modify the given DataSet or return a new
  /// one.
  ///
  /// @{

  /// Split the DataSet at the given index, and stores the resulting
  /// datasets at the locations pointed too by \a first and \a second.
  ///
  /// The point at idx is included in \b both datasets.
  void splitAt(int idx, DataSet ** first, DataSet ** second = NULL) const;

  /// Splits a DataSet in multiple subdatasets of the given X lengths
  /// (computed in absolute value)
  QList<DataSet *> chop(const QList<double> & lengths) const;

  /// Splits a DataSet in multiple subdatasets at the indices given
  QList<DataSet *> chop(const QList<int> & indices) const;

  /// Returns a sorted copy of the dataset.
  DataSet * sort(bool reverse = false) const;

  /// Subtracts \a dataset from this DataSet and returns the result.
  ///
  /// If \a naive is true, only indices are matched, while a more
  /// complex algorithm is used to match X values in the other case.
  DataSet * subtract(const DataSet * dataset, bool naive = false) const;

  /// Dives by \a dataset and returns the result. \sa subtract.
  DataSet * divide(const DataSet * dataset, bool naive = false) const;

  /// Returns the subset of the dataset contained either within the
  /// indices or outside of them
  DataSet * subset(int beg, int end, bool within = true) const;

  /// Returns a new dataset with spikes on X and Y columns removed.
  /// @todo Other columns ?<
  ///
  /// @warning The returned dataset can be NULL if no spikes were
  /// removed/
  DataSet * removeSpikes(int nb = 3, double extra = 200) const;



  /// @}
  
  /// Writes the contents of the dataset in tab-separated format to \a
  /// target.
  void write(QIODevice * target) const;

  /// Writes to the named file, or use the name of the dataset.
  void write(const QString & fileName = QString()) const;

  
};

QDataStream & operator<<(QDataStream & out, const DataSet & ds);
QDataStream & operator>>(QDataStream & in, DataSet & ds);


#endif
