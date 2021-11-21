/*
  dataset-contour.cc: implementation of the contouring routines for DataSet
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
#include <dataset.hh>

#include <exceptions.hh>
#include <credits.hh>


// This code is my implementation of the CONREC algorithm by Paul
// Bourke, see there: http://paulbourke.net/papers/conrec/

static int compareNumbers(double a, double b)
{
  if(a == b)
    return 0;
  if(a < b)
    return -1;
  return 1;
}

/// Assumes that the levels are sorted
QList<QPair<Vector, Vector> > conrecContour(const QList<Vector> & data,
                                            const Vector & xValues,
                                            const Vector & yValues,
                                            const Vector & levels)
{
  QList<QPair<Vector, Vector> > rv;
  for(int i = 0; i < levels.size(); i++)
    rv << QPair<Vector, Vector>();
  
  if(data.size() != yValues.size())
    throw InternalError("Mismatch between number of columns (%1) and number of Y values (%2)").arg(data.size()).arg(yValues.size());
  if(data.size() < 2)
    throw RuntimeError("Need at least two Y values for making contours");

  if(xValues.size() < 2)
    throw RuntimeError("Need at least two X values for making contours");
  for(int i = 0; i < data.size(); i++) {
    if(data[i].size() != xValues.size())
      throw InternalError("Mismatch between number of row of column %1 (%2) and number of X values (%3)").arg(i).arg(data[i].size()).arg(xValues.size());
  }

  int xSz = xValues.size(), ySz = yValues.size(), lSz = levels.size();

  for(int i = 0; i < xSz - 1; i++) {
    for(int j = 0; j < ySz - 1; j++) {
      // Values of the vertices, in the order [top left, top right,
      // bottom right, bottom left, middle]
      double values[5] =
        {
         data[j][i],
         data[j+1][i],
         data[j+1][i+1],
         data[j][i+1],
         0
        },
        xvs[5] =
        {
         xValues[i],
         xValues[i],
         xValues[i+1],
         xValues[i+1],
         0.5*(xValues[i] + xValues[i+1])
        },
        yvs[5] = {
         yValues[j],
         yValues[j+1],
         yValues[j+1],
         yValues[j],
         0.5*(yValues[j] + yValues[j+1])
        };
      for(int w = 0; w < 4; w++)
        values[4] += 0.25*values[w];
      for(int k = 0; k < lSz; k++) {
        double lvl = levels[k];
        // comparison between the level and the vertices
        int cmp[5];
        for(int w = 0; w < 5; w++)
          cmp[w] = compareNumbers(values[w], lvl);
      }
    }
  }

  return rv;
}
