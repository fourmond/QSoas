/**
   \file dataset.hh
   The DataSet class, representing a data set (most of the time one data file)
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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
#ifndef __DATASET_HH
#define __DATASET_HH

#include <vector.hh>
#include <guarded.hh>
#include <valuehash.hh>
#include <datasetoptions.hh>

#include <ruby-wrappers.h>

/// A small helper class that maintains an ordered list of integers
/// (duplicates being possible)
///
/// @todo This should include ways to find the current segment
class OrderedList : public QList<int> {
public:
  /// Inserts the given index into the list
  void insert(int idx);

  /// Shifts all the indices above the given value.
  void shiftAbove(int idx, int delta = -1); 

  OrderedList & operator=(const QList<int> & lst);

private:
  // Here, we disable a few detrimental operations
  OrderedList & operator<<(int);
  void append(int idx);
};

/// A class representing a data set (formerly Soas buffers).
///
/// A DataSet represents a single data set, ie most of the time the
/// data contained in a single file. It contains a X column, a Y
/// column, and possibly additional columns whose use is left for
/// later.
///
/// @todo Add support for meta-data.
class DataSet : public Guardable {

  /// The columns
  QList<Vector> columns;

  /// The coordinate in the perpendicular direction: [0] would be the
  /// Y coordinate for column 1, [1] for column 2 and so on.
  ///
  /// This makes sense only for datasets that are intrinsically 3D.
  Vector perpCoords;

  /// A set of meta-data. Basically anything !
  ///
  /// @todo Have the data handled by that
  ValueHash metaData;

  
  /// The set of flags attached to this dataset
  QSet<QString> flags;
  

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
  /// X matching. Returns a brand new DataSet. If \a naive is on, it
  /// works index by index. If \a useSteps is on, each step on each
  /// side is compared to each on the other. For that to work, there
  /// must be at least as many steps in \a b than in \a a.
  ///
  /// By default, operations are applied between matching Y columns,
  /// ie Y1 of a with Y1 of b, Y2 of a with Y2 of b and so
  /// on. However, if useACol is non-negative, operations are applied
  /// to all Y columns of b with the correspond Y column of a.
  ///
  /// 
  ///
  /// The operation takes quadratic time, as all X of a are matched
  /// against all X of b for each value (which is bad,
  /// admittedly). There are probably ways to be much more clever than
  /// that.
  ///
  static DataSet * applyBinaryOperation(const DataSet * a,
                                        const DataSet * b,
                                        double (*op)(double, double),
                                        const QString & cat = "_op_",
                                        bool naive = false, 
                                        bool useSteps = false,
                                        int useACol = -1);


  friend QDataStream & operator<<(QDataStream & out, const DataSet & ds);
  friend QDataStream & operator>>(QDataStream & in, DataSet & ds);

public:


  /// A small helper function for creating new datasets with the same
  /// X, the given Y, and a name based on the original one with a
  /// given suffix. X can be changed using the optional \a x argument.
  ///
  /// It keeps other columns unchanged.
  DataSet * derivedDataSet(const Vector &y, const QString & suffix,
                           const Vector &x = Vector()) const;

  /// A derived dataset with no modification. You can alway modify it
  /// later...
  DataSet * derivedDataSet(const QString & suffix) const;

  /// A small helper function for creating new datasets that keep as
  /// much information as possible from the old dataset while changing
  /// all the columns.
  ///
  /// This function (or its sister) should be used systematically
  /// whenever one creates a new dataset based on an old one.
  DataSet * derivedDataSet(const QList<Vector> &newCols, 
                           const QString & suffix) const;

  /// Creates a derived dataset based on a list of Qt points.
  DataSet * derivedDataSet(const QList<QPointF> &newCols, 
                           const QString & suffix) const;

  /// The name of the dataset, usually the name of the file.
  QString name;

  /// The date and time of the (original ?) data
  QDateTime date;

  /// @name Flag-related functions
  ///
  /// @{

  /// Whether the dataset has one or more flags attached to it.
  bool flagged() const;

  /// Whether the dataset has the given flag attached to it.
  bool flagged(const QString & str) const;

  /// Returns all the flags
  QSet<QString> allFlags() const;

  /// Sets the given flag
  void setFlag(const QString & str);

  /// Sets the given flags
  void setFlags(const QStringList & lst);

  /// Sets the given flags
  void setFlags(const QSet<QString> & lst);

  /// Removes the given flag
  void clearFlag(const QString & str);

  /// Removes the given flags;
  void clearFlags(const QStringList & lst);

  /// Removes all the flags
  void clearFlags();

  /// @}


  /// @name Perpendicular coordinates handling
  ///
  /// @{

  /// Returns the perpendicular coordinates
  const Vector & perpendicularCoordinates() const;

  /// Sets perpendicular coordinates
  void setPerpendicularCoordinates(const Vector & vect);

  /// Set perpendicular coordinates
  void setPerpendicularCoordinates(double val);

  /// @}

  /// @name Options and option-related functions.
  ///
  /// @{

  /// The options attached to this dataset
  DatasetOptions options;

  /// The Y error for the given index
  double yError(int idx) const {
    return options.yError(this, idx);
  };

  /// @}


  /// @name Segment-related functions
  ///
  /// @{

  /// A list of indices delimiting segments in the dataset (each index
  /// is the start point of a new segment). Segments correspond to
  /// different phases of a single experiment, generally with a
  /// corresponding variation of experimental conditions.
  ///
  /// By default, there are no segments.
  ///
  /// They will be used for:
  /// @li segment-by-segment dataset operations
  /// @li segment-by-segment film loss correction
  /// @li other ideas ?
  OrderedList segments;

  /// Returns the X positions of the changes in segments.
  Vector segmentPositions() const;

  /// Like chop with indices, but uses the saved segments (and changes a
  /// bit the dataset_names...)
  QList<DataSet *> chopIntoSegments() const;

  /// @}

  DataSet() {;};
  DataSet(const QList<Vector> & cols) : columns(cols) {;};
  DataSet(const Vector & x, const Vector & y) {
    columns << x << y;
  };
  DataSet(const Vector & x, const Vector & y, const Vector & z) {
    columns << x << y << z;
  };

  /// Returns a version of name without the extension, that can be
  /// used for making up new buffer names from the old ones.
  QString cleanedName() const;

  /// Inserts a column at the given position.
  void insertColumn(int idx, const Vector & col) {
    invalidateCache();
    columns.insert(idx, col);
  };

  /// Inserts a columns at the end
  void appendColumn(const Vector & col) {
    insertColumn(columns.size(), col);
  }

  /// Strips all the columns that contain only NaNs
  void stripNaNColumns();

  /// Whether there are any NaNs or Infs in the x() or y() columns
  bool hasNotFinite() const;

  /// Strip rows containing NaNs or Infs in the x() or y() column
  void stripNotFinite();
  
  /// Flip all columns back to front, so that the first
  /// X,Y1,Y2... become the last one and vice versa.
  void reverse();

  /// Inserts a column at the given position.
  Vector takeColumn(int idx) {
    invalidateCache();
    return columns.takeAt(idx);
  };

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

  /// All the columns
  const QList<Vector> & allColumns() const {
    return columns;
  };

  
  /// Returns the numbered column
  const Vector & column(int i) const {
    return columns[i];
  };

  /// Returns the numbered column
  Vector & column(int i) {
    invalidateCache();
    return columns[i];
  };

  /// Dump the data to standard output.
  ///
  /// @todo This may grow into something interesting later on, but not
  /// now.
  void dump() const;

  /// Return the number of rows.
  int nbRows() const {
    if(columns.size() > 0)
      return x().size();
    return 0;
  };

  /// Returns the number of columns
  int nbColumns() const {
    return columns.size();
  };

  /// Returns a small text representing the data set (or a long one,
  /// if longDescription is on).
  QString stringDescription(bool longDescription = false) const;

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

  /// Returns the distance of the \a point to the curve.
  QPair<double, int> distanceTo(const QPointF & p, 
                                const QPointF & scales) const {
    return distanceTo(p.x(), p.y(), scales.x(), scales.y());
  };

  /// Returns the numbered point
  QPointF pointAt(int i) const {
    return QPointF(columns[0][i], columns[1][i]);
  };

  /// Removes the point at the given index.
  void removePoint(int i);

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

  /// Returns the Y value the closest to the given X value.
  ///
  /// If interpolate is true, interpolates linearly (of course when it
  /// is located between two points)
  double yValueAt(double x, bool interpolate = false) const;

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
  /// 
  /// If @a isLength is false, cut at the given X values.
  ///
  /// If @a indices isn't NULL, the datasets are not created, but
  /// rather the indices of the stops are pushed into the list.
  QList<DataSet *> chop(const QList<double> & lengths, 
                        bool isLength = true,
                        QList<int> * indices = NULL) const;

  /// Splits a DataSet in multiple subdatasets at the indices given
  QList<DataSet *> chop(const QList<int> & indices) const;

  /// Returns a sorted copy of the dataset.
  DataSet * sort(bool reverse = false) const;

  /// Subtracts \a dataset from this DataSet and returns the result.
  ///
  /// If \a naive is true, only indices are matched, while a more
  /// complex algorithm is used to match X values in the other case.
  DataSet * subtract(const DataSet * dataset, bool naive = false, 
                     bool useSteps = false) const;

  /// Adds \a dataset to this DataSet and returns the result.
  ///
  /// If \a naive is true, only indices are matched, while a more
  /// complex algorithm is used to match X values in the other case.
  DataSet * add(const DataSet * dataset, bool naive = false, 
                     bool useSteps = false) const;

  /// Multiplies this DataSet by \a dataset and returns the result.
  ///
  /// If \a naive is true, only indices are matched, while a more
  /// complex algorithm is used to match X values in the other case.
  DataSet * multiply(const DataSet * dataset, bool naive = false, 
                     bool useSteps = false) const;

  /// Divides by \a dataset and returns the result. \sa subtract.
  DataSet * divide(const DataSet * dataset, bool naive = false, 
                   bool useSteps = false) const;

  /// Returns a dataset containing \a dataset's Y and further
  /// columns as a function of Y of this dataset.
  DataSet * merge(const DataSet * dataset, bool naive = false, 
                  bool useSteps = false) const;

  /// Does the reverse of expand
  ///
  /// If \a useColumns is present, only the given columns are
  /// contracted. Careful, columns are 0-based !
  DataSet * contract(const DataSet * dataset, bool naive = false, 
                     bool useSteps = false, const QList<int> & useColumns = 
                     QList<int>() ) const;

  /// Returns the subset of the dataset contained either within the
  /// indices or outside of them
  DataSet * subset(int beg, int end, bool within = true) const;

  /// Returns a new dataset with spikes on X and Y columns removed.
  /// @todo Other columns ?
  ///
  /// @warning The returned dataset can be NULL if no spikes were
  /// removed
  DataSet * removeSpikes(int nb = 3, double extra = 200) const;

  /// Computes a 4-th order accurate first derivative.
  ///
  /// @warning This function fails if there are less than 5 data
  /// points.
  DataSet * firstDerivative() const;

  /// Computes a 4-th order accurate second derivative.
  ///
  /// @warning This function fails if there are less than 5 data
  /// points.
  DataSet * secondDerivative() const;

  /// Concatenates all datasets into a single one.
  ///
  /// If \a setSegments is true, then new segments are added at the
  /// boundary points. In any case, the original segments are
  /// preserved.
  static DataSet * concatenateDataSets(QList<const DataSet *> datasets, 
                                       bool setSegments = true);


  /// @}

  /// Transpose a dataset, ie creates a new dataset where the
  /// perpendicular coordinate becomes the X coordinate and vice versa
  /// (ie swap columns and rows, somehow)
  DataSet * transpose() const;
  
  /// Writes the contents of the dataset in tab-separated format to \a
  /// target.
  void write(QIODevice * target) const;

  /// Writes to the named file, or use the name of the dataset.
  void write(const QString & fileName = QString()) const;

  /// A utility function to compute the first derivative of a given
  /// series of points (not to be used on noisy -- or even simply
  /// experimental -- data !)
  static void firstDerivative(const double *x, int xstride, 
                              const double *y, int ystride, 
                              double * target, int tstride,
                              int nb);

  /// Splits into a series of disconnected buffers whose X values (or
  /// the values of the given column) are monotonic.
  ///
  /// If group is more than one, it is the number of monotonic
  /// segments pushed in each of the datasets.
  QList<DataSet * > splitIntoMonotonic(int col = 0, int group = 1) const;


  /// Returns the names of the columns.
  ///
  /// @todo find a way to customize that later on.
  QStringList columnNames() const;


  /// @name Meta-data related functions
  ///
  /// @{

  /// Sets the given meta-data
  void setMetaData(const QString & name, const QVariant & value);

  /// Returns all the meta-data
  const ValueHash & getMetaData() const;

  /// Clears the given meta-data
  void clearMetaData(const QString & name);

  /// Returns the given metadata
  QVariant getMetaData(const QString & val) const;

  /// Add a whole new set of meta-data
  void addMetaData(const ValueHash & val, bool override = true);


  /// Evaluates the given expression, setting the $stats and $meta
  /// variables as necessary
  RUBY_VALUE evaluateWithMeta(const QString & expression, bool useStats = false) const;

  /// Runs the given expression feeding it the values of the meta-data
  /// and the statistics, and returns wether the expression is true or
  /// false. It returns false also if the expression fails for some
  /// reason.
  bool matches(const QString & expression) const;


  /// This function splits the dataset into a series of datasets based
  /// on the values of the given columns: each dataset in the final
  /// set corresponds to a unique set of values. They are added as
  /// meta-data to the final datasets.
  QList<DataSet *> autoSplit(const QHash<int, QString> & cols, double tolerance = 0) const;

  /// @}
  
};

QDataStream & operator<<(QDataStream & out, const DataSet & ds);
QDataStream & operator>>(QDataStream & in, DataSet & ds);


#endif
