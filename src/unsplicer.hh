/**
   \file unsplicer.hh
   A class to "unsplice" multiple trends within a X,Y dataset
   Copyright 2019 by CNRS/AMU

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
#ifndef __UNSPLICER_HH
#define __UNSPLICER_HH

#include <vector.hh>

class DataSet;


/// An ordered dataset used for Unsplicer
class UnsplicedData {
public:
  /// We assume the xv vector is always sorted.
  Vector xv, yv;

  Vector dy;

  /// Returns the size
  int size() const;
                    
  UnsplicedData(double x, double y);
  
  /// Returns the index at which the given point would be inserted,
  /// i.e. the index of the first X value greater than @a x.
  ///
  /// @todo for now naive search
  int indexOfX(double x) const;
  
  /// Magnitudes of the y and derivative values around the given X
  /// value.
  ///
  /// Returns the position at which x would get inserted.
  int magnitudesAround(double x, double * ym,
                       double * dym, int window = 3) const;

  
  /// Returns true if this data point could be part of this data
  bool isMine(double x, double y, double tolerance, double dtol) const;

  
  /// Inserts the given point. Returns the index of the new point.
  int insertPoint(double x, double y);};


/// This class takes a X,Y series and 
class Unsplicer {
  Vector xv, yv;

  QList<UnsplicedData> trnds;

  Vector lx,ly;

  bool unspliced;
public:

  Unsplicer(const Vector & x, const Vector & y);

  /// Does the unsplicing.
  ///
  /// @todo Maybe this should be controlled by various
  void unsplice();

  /// Returns the trends. unsplice() should have been run first.
  QList<UnsplicedData> trends() const;

  /// Returns the "leftover points", i.e. the points that were not
  /// assigned to a trend. unsplice() should have been run first.
  QList<Vector> leftovers() const;

  /// Annotates the original data points with the number of the trend
  /// it was found to belong to, or -1 if it is part of the leftovers.
  /// unsplice() should have been run first.
  QList<int> annotate() const;
  
};

#endif
