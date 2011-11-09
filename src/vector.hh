/**
   \file vector.hh
   The Vector class, representing one data column
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

#ifndef __VECTOR_HH
#define __VECTOR_HH

/// A class representing a data column.
///
/// It is but a QVector of double with additional functionalities.
class Vector : public QVector<double> {
public:

  Vector() {;};
  Vector(const QVector<double> & d) : QVector<double>(d) {;};
  Vector(int nb, double value) : QVector<double>(nb, value) {;};

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
  /// @{

  /// The minimum value
  double min() const;

  /// The maximum value
  double max() const;

  /// Fills the target values with the average and variance of the set
  void stats(double * average, double * variance) const;

  /// @}

  /// @name Other operations
  ///
  /// @{

  /// Returns a vector of length n-1 containing the deltas between
  /// successive values (x[i+1]-x[i]).
  Vector deltas() const;

  /// Returns a new vector with spikes removed.
  ///
  /// The \a nbFound pointer, if not NULL, is filled with the number
  /// of spikes found...
  Vector removeSpikes(int nb = 3, double extra = 200, 
                      int *nbFound = NULL) const;

  /// @}


  /// @name GSL-related utility functions
  ///
  /// A few functions to ease interoperability with GSL
  /// @{

  /// Returns a gsl_vector object suitable to represent this dataset.
  ///
  /// \warning The return value should not be freed using
  /// gsl_vector_free !
  gsl_vector_view vectorView();

  /// Returns a gsl_vector object suitable to represent this dataset.
  ///
  /// \warning The return value should not be freed using
  /// gsl_vector_free !
  gsl_vector_const_view vectorView() const;

  /// Constructs from a GSL vector
  static Vector fromGSLVector(const gsl_vector * vect);
  
  /// @}

  /// Reads textual data from a file.
  /// \li separator is specified by 
  /// \li comment lines are those that match \p commentRE. They are stored
  /// in \p comments when not NULL.
  ///
  /// @todo Quite a lot of things to improve here.
  static QList<Vector> readFromStream(QIODevice * source,
                                      const QString & separatorRE = "\\s+",
                                      const QString & commentRE = "^\\s*#",
                                      QStringList * comments = NULL);
};

#endif
