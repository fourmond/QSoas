/**
   \file vector.hh
   The Vector class, representing one data column
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014, 2020 by CNRS/AMU

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
#ifndef __VECTOR_HH
#define __VECTOR_HH

/// A class representing a data column.
///
/// It is but a QVector of double with additional functionalities.
class Vector : public QVector<double> {
protected:

  /// A private vector view. The mutable attribute is nice but not
  /// necessary, as I'm const-casting anyway.
  mutable gsl_vector_view view;
  
public:

  Vector() {;};
  Vector(const QVector<double> & d) : QVector<double>(d) {;};
  Vector(int nb, double value) : QVector<double>(nb, value) {;};

  /// Creates vector from raw data: @a nb doubles given in @a init
  /// This should seldom be ambiguous, I think.
  Vector(const double * init, int nb);

  /// @name Arithmetic operations
  /// 
  /// The arithmetic operations are redefined to better suit the needs
  /// of the arithmetics behind. The original meaning of + is still
  /// accessible with <<.
  ///
  /// Care was taken to ensure that the implementations are fast
  /// enough, but, well...
  ///
  /// @{

  Vector & operator+=(const Vector & a);
  Vector & operator+=(double a);

  Vector operator+(const Vector & a) const;
  Vector operator+(double a) const;

  Vector & operator-=(const Vector & a);
  Vector & operator-=(double a);

  Vector operator-(const Vector & a) const;
  Vector operator-(double a) const;

  Vector & operator*=(const Vector & a);
  Vector & operator*=(double a);

  Vector operator*(const Vector & a) const;
  Vector operator*(double a) const;

  Vector & operator/=(const Vector & a);
  Vector & operator/=(double a);

  Vector operator/(const Vector & a) const;
  Vector operator/(double a) const;

  /// @}

  /// @name Data inspection facilities
  ///
  /// Various functions returning information about the vector.
  ///
  /// @todo Maybe some cacheing would be nice ? OTOH, I doubt we're
  /// losing a lot of processing power here.
  ///  
  /// @{

  /// The minimum value
  double min() const;


  /// The index of the last minimum value
  int whereMin() const;

  /// The minimum value, excluding all funny numbers
  double finiteMin() const;

  /// The maximum value
  double max() const;
  
  /// The index of the last maximum value
  int whereMax() const;

  /// The maximum  value, excluding all funny numbers
  double finiteMax() const;

  /// The value most distant from 0
  double magnitude() const;

  /// The square root of the sum of the squares
  double norm() const;

  /// Returns true if all elements are finite
  bool allFinite() const;

  /// Fills the target values with the average and variance of the set
  void stats(double * average, double * variance, double * sum = NULL) const;

  /// Returns the median value. Uses a sorted copy, ie nlogn time.
  double median() const;

  /// Fills deltamin and deltamax with respectively the smallest (in
  /// absolute value) delta between successive points and the largest
  /// one.
  void deltaStats(double * deltamin, double * deltamax = NULL) const;

  /// Returns the total distance (ie sum of absolute value of deltas).
  double deltaSum() const;


  /// Returns true if the sum of the variations of the vector exceed
  /// threshold times the min to max distance.
  /// @sa DatasetOptions::isJaggy
  bool isJaggy(double threshold) const;


  /// Looks for the indice of the first point crossing the given
  /// value, starting from the given index and going in the direction
  /// given by di (only the sign matters). It returns the index of the
  /// first point after the crossing, or the last point taken into
  /// consideration if not found.
  int findCrossing(int seed, double thresh, int di = 1, bool * found = NULL) const;

  

  /// Finds local extrema and returns:
  /// \li 1 + index if it is a maximum
  /// \li -(1 + index) if it is a minimum
  ///
  /// The extrema has to be an extrema for over \a window
  QList<int> extrema(int window = 8) const; 

  /// Find the index of the value the closest to the one given here
  int closestPoint(double value) const;

  /// Wether the data consists only of NaN (such as when one loads
  /// text files)
  bool hasOnlyNaN() const;

  /// Returns true if there is one ore more NaNs or Inf
  bool hasNotFinite() const;

  /// Returns a vector containing all the values of the vector, sorted
  /// and all distinct, within the given tolerance.
  Vector values(double tolerance = 0) const;

  /// Performs a binary search for @a value. It returns the index at
  /// which the value is <b>or where it should be</b>. In particular,
  /// if the number is higher than the highest, the index returned
  /// will be size().
  ///
  /// @warning The vector needs to be @b sorted !
  ///
  /// It can handle NaNs -- they will be stored at the end.
  int bfind(double value) const;

  /// @}

  /// @name Other operations
  ///
  /// @{

  /// If @a centered is false, returns a vector of length n-1 containing the deltas between
  /// successive values (x[i+1]-x[i]).
  ///
  /// If @a centered is true, returns a vector of the same length with 0.5 *(x[i+1] - x[i-1]) (with provisions for borders).
  Vector deltas(bool centered = false) const;

  /// Returns a new vector with spikes removed.
  ///
  /// The \a nbFound pointer, if not NULL, is filled with the number
  /// of spikes found...
  Vector removeSpikes(int nb = 3, double extra = 200, 
                      int *nbFound = NULL) const;


  /// Returns a uniformly spaced vector spanning the same range, with
  /// possibly a different number of points.
  Vector uniformlySpaced(int nb = -1) const;

  /// Resamples the vector, ie returns a vector with factor times less
  /// points.
  ///
  /// It is not designed to be very precise.
  Vector downSample(int factor) const;


  /// Bins the vector into the given number of boxes (or the log of
  /// the vector). Returns two vectors, one with the mid points of the
  /// ranges, the other with the numbers.
  QList<Vector> bin(int boxes, bool log = false,
                    const Vector & weight = Vector(),
                    bool norm = false,
                    double min = std::nan("NaN"),
                    double max = std::nan("NaN")
                    ) const;

  /// Flips the contents of the vector from back to front
  void reverse();

  /// Apply a function double to double to the vector and returns the
  /// newly created values.
  void applyFunction(const std::function<double (double)> & func);

  /// Apply a random scaling factor to all the components of the
  /// vector. The scaling factor is between @a low and @a high.
  ///
  /// @todo Do that with a custom random number generator.
  void randomize(double low, double high);

  /// Rotates the contents of the vector, i.e. shifts the position of
  /// the elements from @a i to @a i + @a delta, wrapping around.
  void rotate(int delta);

  /// This function is a low-level convolution function in which the
  /// vector @a vector containing @a nb elements assumed to be equally
  /// spaced between @a xmin and @a xmax are convolved by the function
  /// @a function.
  ///
  /// That function is either symmetric (defined for positive and
  /// negative values of x) or not (defined only for positive values
  /// of x)
  ///
  /// The @a buffer is a buffer for storing values it must be large
  /// enough to contain 4*nb values.
  static void convolve(const double * vector,
                       int nb,
                       double * target,
                       double xmin,
                       double xmax,
                       std::function<double (double)> function,
                       bool symmetric,
                       double * buffer);


  /// @}


  /// @name GSL-related utility functions
  ///
  /// A few functions to ease interoperability with GSL
  /// @{

  /// Returns a gsl_vector_view object suitable to represent this dataset.
  gsl_vector_view vectorView();

  /// Returns a gsl_vector_view object suitable to represent this dataset.
  gsl_vector_const_view vectorView() const;

  /// Returns a gsl_vector suitable to modify this Vector
  gsl_vector * toGSLVector();

  operator gsl_vector *() {
    return toGSLVector();
  };

  operator const gsl_vector *() const {
    return toGSLVector();
  };

  /// Returns a gsl_vector suitable to access this Vector
  const gsl_vector * toGSLVector() const;

  /// Constructs from a GSL vector
  /// @todo make a constructor ?
  static Vector fromGSLVector(const gsl_vector * vect);
  
  /// @}

  /// Returns the list of formatted texts.
  QStringList asText(int fieldWidth = 0, char format = 'g',
                     int precision = -1, const QChar & fillChar = QLatin1Char( ' ' ))const;

  /// Reads textual data from a file.
  /// \li the splitting of each line into fields is done by the
  /// splitter function. The splitter function stores
  /// the index of the beginning and of the end of each 
  /// \li comment lines are those that match \p commentRE. They are stored
  /// in \p comments when not NULL.
  /// \li if splitOnBlank is true, then a new list of vectors is built for
  /// every time a blank line is found.
  ///
  /// @todo Quite a lot of things to improve here.
  ///
  /// A note about the splitter function: it should take the string,
  /// and store the indices and length of each element. The size
  /// should reflect the exact number of elements found.
  static QList<QList<Vector> > readFromStream(QTextStream * source,
                                              std::function<void (const QString &, QVector<int> * indices)> splitter,
                                              const QRegExp & commentRE,
                                              bool splitOnBlank = false,
                                              const QString & decimalSep = QString(),
                                              const QRegExp & blankRE = QRegExp("^\\s*$"),
                                              QStringList * comments = NULL,
                                              int skip = 0,
                                              QList<int> textColumns = QList<int>(),
                                              QList<QList<QStringList> > * savedTexts = NULL
                                              );

  /// Convenience overload in which the splitting is done by the
  /// separator regexp
  static QList<QList<Vector> > readFromStream(QTextStream * source,
                                              const QRegExp & separatorRE,
                                              const QRegExp & commentRE,
                                              bool splitOnBlank = false,
                                              const QString & decimalSep = QString(),
                                              const QRegExp & blankRE = QRegExp("^\\s*$"),
                                              QStringList * comments = NULL,
                                              int skip = 0,
                                              QList<int> textColumns = QList<int>(),
                                              QList<QList<QStringList> > * savedTexts = NULL,
                                              bool trim = true);


  /// Convenience overload
  static QList<QList<Vector> > readFromStream(QTextStream * source,
                                              const QString & separatorRE = "\\s+",
                                              const QString & commentRE = "^\\s*#",
                                              bool splitOnBlank = false,
                                              const QString & blankRE = "^\\s*$",
                                              QStringList * comments = NULL);

  /// Returns a vector with nb values spaced uniformly on the
  /// [min:max] segment.
  ///
  /// @todo Write the same function with log transform ?
  /// With *any* Bijection, for that matter ?
  static Vector uniformlySpaced(double min, double max, int nb);

  /// Returns a vector containing the indices, from 0 to nb-1.
  static Vector indexVector(int nb);

  /// Returns a vector with nb values spaced logarithmically on the
  /// [min:max] segment.
  static Vector logarithmicallySpaced(double min, double max, int nb);


  /// Integrates the given X,Y data into a single sum.
  static double integrate(const Vector & x, const Vector & y);

  /// Integrates the given vectors from the given index, and returns
  /// the corresponding y vector.
  static Vector integrateVector(const Vector & x, const Vector & y, int idx = 0);

  /// Returns true if the difference of the vectors is within the @a
  /// tol factor. Returns false otherwise.
  static bool withinTolerance(const Vector & x, const Vector & y, double tol);



  /// Returns a list of data for which the log of the Y values are
  /// (more-or-less) less than @a tolerance apart from the average of
  /// logs.
  static QList<QList<Vector> > orderOfMagnitudeClassify(const Vector & xv,
                                                        const Vector & yv,
                                                        double tolerance);

  /// Returns the correlation coefficient of the two vectors, i.e.
  /// @f$\rho_{X,Y} =  \frac{{E}(XY)-{E}(X){E}(Y)}{\sqrt{{E}(X^2)-{E}(X)^2}\cdot \sqrt{{E}(Y^2)-{E}(Y)^2}}@f$
  static double correlation(const Vector & x, const Vector & y);
};

#endif
