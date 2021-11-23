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
void conrecContour(const QList<Vector> & data,
                   const Vector & xValues,
                   const Vector & yValues,
                   const Vector & levels,
                   std::function<void (double x1, double y1,
                                       double x2, double y2,
                                       int lvl)> addLine)
{
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

        // Returns the x value of the secant of a,b
        auto mixX = [xvs, yvs, values, lvl](int a, int b) -> double {
          double al = (values[b] - lvl)/(values[b]-values[a]);
          return xvs[a]*al + xvs[b]*(1-al);
        };
        auto mixY = [xvs, yvs, values, lvl](int a, int b) -> double {
          double al = (values[b] - lvl)/(values[b]-values[a]);
          return yvs[a]*al + yvs[b]*(1-al);
        };

        // Now loop over the 4 triangles:
        for(int t = 0; t < 4; t++) {
          int v1 = t, v2 = t+1, v3 = 4;
          if(v2 == 4)
            v2 = 0;
          int disc = cmp[v1] + cmp[v2] * 4 + cmp[v3] * 16;
          // Here are the various cases (27 in total). We represent
          // them as: A, E, B (above, equal, below)
          switch(disc) {
            
          case 16+4+1: // All above
          case -16-4-1: // All below
            // Above plus one edge equal
          case 16+4: 
          case 16+1: 
          case 4+1: 
            // Below plus one edge equal
          case -16-4: 
          case -16-1: 
          case -4-1:
            // Nothing to do
            break;
            
            // Edge v1--v2:
            // We also use this when the value is uniform
          case 16:
          case -16:
          case 0:
            addLine(xvs[v1], yvs[v1], xvs[v2], yvs[v2], k);
            break;
            
            // Edge v1--v3:
          case 4:
          case -4:
            addLine(xvs[v1], yvs[v1], xvs[v3], yvs[v3], k);
            break;
            
            // Edge v2--v3:
          case 1:
          case -1:
            addLine(xvs[v2], yvs[v2], xvs[v3], yvs[v3], k);
            break;
            
            // v1 to point of v2--v3:
          case 16-4:
          case -16+4:
            addLine(xvs[v1], yvs[v1], mixX(v2, v3), mixY(v2, v3), k);
            break;

            // v2 to point of v1--v3:
          case 16-1:
          case -16+1:
            addLine(xvs[v2], yvs[v2], mixX(v1, v3), mixY(v1, v3), k);
            break;

            // v3 to point of v1--v2:
          case 4-1:
          case -4+1:
            addLine(xvs[v3], yvs[v3], mixX(v1, v2), mixY(v1, v2), k);
            break;

            // v1--v2 to v1--v3
          case -16-4+1:
          case 16+4-1:
            addLine(mixX(v1, v2), mixY(v1, v2),
                    mixX(v1, v3), mixY(v1, v3),
                    k);
            break;

            // v2--v1 to v2--v3
          case -16-1+4:
          case 16+1-4:
            addLine(mixX(v2, v1), mixY(v2, v1),
                    mixX(v2, v3), mixY(v2, v3),
                    k);
            break;
            
            // v3--v1 to v3--v2
          case 16-1-4:
          case -16+1+4:
            addLine(mixX(v3, v1), mixY(v3, v1),
                    mixX(v3, v2), mixY(v3, v2),
                    k);
            break;

          default:
            throw InternalError("Unhandled case: %1").arg(disc);
          }
        }
      }
    }
  }

}

// Temporary code for testing
#include <command.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>
#include <soas.hh>


static void contourCommand(const QString & /*name*/,
                           double value,
                           const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  Vector lvl;
  lvl << value;
  QList<Vector> data = ds->allColumns();
  if(data.size() < 3)
    throw RuntimeError("Need at least two Y columns");
  Vector x = data.takeFirst();
  Vector y = ds->perpendicularCoordinates();

  Vector nx, ny;
  auto addLine = [&nx, &ny](double x1, double y1, double x2, double y2, int) {
    nx << x1 << x2 << std::nan("");
    ny << y1 << y2 << std::nan("");
  };

  conrecContour(data, x, y, lvl, addLine);
  QList<Vector> ncls;
  ncls << nx << ny;
  DataSet * nds = ds->derivedDataSet(ncls, "_cont.dat");
  soas().pushDataSet(nds);
}


static ArgumentList 
cntA(QList<Argument *>()
      << new NumberArgument("level",
                            "Levels")
      );


static Command 
cntC("contour", // command name
     effector(contourCommand), // action
     "buffer",  // group name
     &cntA, // arguments
     NULL, // options
     "Contours");
