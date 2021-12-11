/**
   \file contourlines.hh
   The ContourLines class, implementation of contouring routines 
   Copyright 2021 by CNRS/AMU

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

   Some parts of this file are under a different copyright/license,
   please see below
*/

#include <headers.hh>
#ifndef __CONTOURLINES_HH
#define __CONTOURLINES_HH

#include <vector.hh>

/// A class representing a set of contour lines. They can be
/// disconnected.
class ContourLines {
public:

  /// Sub-class representing a continuous path
  struct Path {
    Vector xvalues;
    Vector yvalues;

    Path();

    Path(double x1, double y1, double x2, double y2);

    /// Return 0 if x,y is neither the end nor the beginning of the
    /// path, -1 if the beginning, 1 if the end.
    int isEnd(double x, double y) const;
    bool isConnected(const Path & other) const;
    
    void merge(const Path & other);
  };


  QList<Path> paths;

  /// Adds a segment to the lines, merging into the currently
  /// available paths if possible.
  void addSegment(double x1, double y1, double x2, double y2);

  /// Draw contours at the given level around the data, using the
  /// given X and Y values. @b Note: assumes that the levels are
  /// sorted.
  static QList<ContourLines> conrecContour(const QList<Vector> & data,
                                           const Vector & xValues,
                                           const Vector & yValues,
                                           const Vector & levels);
                         
  
};

#endif
