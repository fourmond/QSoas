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
#include <contourlines.hh>
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
static void conrecContour(const QList<Vector> & data,
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

//////////////////////////////////////////////////////////////////////

/// @todo merge with Utils::fuzzyCompare ?
bool epsilonCompare(double a, double b, int fact = 10)
{
  if(a == b)
    return true;
  if( fabs(a - b) < fact * std::numeric_limits<double>::epsilon()
      * (fabs(a) + fabs(b)))
    return true;
  return false;
}



ContourLines::Path::Path()
{
}

ContourLines::Path::Path(double x1, double y1, double x2, double y2)
{
  xvalues << x1 << x2;
  yvalues << y1 << y2;
}

int ContourLines::Path::isEnd(double x, double y) const
{
  if(::epsilonCompare(x,xvalues.first()) &&
     ::epsilonCompare(y,yvalues.first()))
    return -1;
  if(::epsilonCompare(x,xvalues.last()) &&
     ::epsilonCompare(y,yvalues.last()))
    return 1;
  return 0;
}

bool ContourLines::Path::isConnected(const Path & other) const
{
  if(isEnd(other.xvalues.first(), other.yvalues.first()) != 0)
    return true;
  if(isEnd(other.xvalues.last(), other.yvalues.last()) != 0)
    return true;
  return false;
}

    
void ContourLines::Path::merge(const Path & other)
{
  /// Appends the given vector to the beginning
  auto insertBeg = [](Vector &a, const Vector & b, bool swapB) {
                     Vector t = b;
                     if(swapB) {
                       t.reverse();
                     }
                     t.takeLast();
                     t.append(a);
                     a = t;
                   };
      
  auto insertEnd = [](Vector &a, const Vector & b, bool swapB) {
                     Vector t = b;
                     if(swapB) {
                       t.reverse();
                     }
                     t.takeFirst();
                     a.append(t);
                   };

  int v = isEnd(other.xvalues.first(), other.yvalues.first());
  // QTextStream o(stdout);
  // o << " * v(first) = " << v << endl;
  // o << " bsz = " << yvalues.size() << endl;


  if(v < 0) {
    insertBeg(xvalues, other.xvalues, true);
    insertBeg(yvalues, other.yvalues, true);
    // o << " sz = " << yvalues.size() << endl;
    return;
  }
  if(v > 0) {
    insertEnd(xvalues, other.xvalues, false);
    insertEnd(yvalues, other.yvalues, false);
    // o << " sz = " << yvalues.size() << endl;
    return;
  }
  v = isEnd(other.xvalues.last(), other.yvalues.last());
  // o << " * v(last) = " << v << endl;
  if(v < 0) {
    insertBeg(xvalues, other.xvalues, false);
    insertBeg(yvalues, other.yvalues, false);
    // o << " sz = " << yvalues.size() << endl;
    return;
  }
  if(v > 0) {
    insertEnd(xvalues, other.xvalues, true);
    insertEnd(yvalues, other.yvalues, true);
    // o << " sz = " << yvalues.size() << endl;
    return;
  }
}


//////////////////////////////////////////////////////////////////////

void ContourLines::addSegment(double x1, double y1, double x2, double y2)
{
  Path np(x1, y1, x2, y2);
  // QTextStream o(stdout);
  // o.setRealNumberPrecision(16);
  // o << "Adding " << ": " << np.xvalues.first()
  //   << "," << np.yvalues.first()
  //   << " to " << np.xvalues.last()
  //   << "," << np.yvalues.last() << endl;

  int fnd = -1;
  for(int i = 0; i < paths.size(); i++) {
    const Path & p = paths[i];
    if(p.isConnected(np)) {
      fnd = i;
      break;
    }
  }
  // o << "Fnd: " <<  fnd << endl;
  if(fnd >= 0) {
    // Now merge into fnd
    // o << " - " << paths[fnd].xvalues.size();
    paths[fnd].merge(np);
    // o << " -> " << paths[fnd].xvalues.size() << endl;
    // Add the possibility that the newly added path makes two
    // pre-existing paths merge.
    for(int i = fnd+1; i < paths.size(); i++) {
      if(paths[fnd].isConnected(paths[i])) {
        // o << " -> merging " << fnd << " and " << i << endl;
        paths[fnd].merge(paths.takeAt(i));
        break;
      }
    }
  }
  else
    paths << np;

    
  // for(int i = 0; i < paths.size(); i++) {
  //   o << "Path " << i << ": " << paths[i].xvalues.first()
  //     << "," << paths[i].yvalues.first()
  //     << " to " << paths[i].xvalues.last()
  //     << "," << paths[i].yvalues.last() << endl;
  // }
}


QList<ContourLines> ContourLines::conrecContour(const QList<Vector> & data,
                                                const Vector & xValues,
                                                const Vector & yValues,
                                                const Vector & levels)
{
  QList<ContourLines> allPaths;
  for(int i = 0; i < levels.size(); i++)
    allPaths << ContourLines();

  auto addLine = [&allPaths](double x1, double y1,
                             double x2, double y2, int k) {
    allPaths[k].addSegment(x1, y1, x2, y2);
  };

  ::conrecContour(data, xValues, yValues, levels, addLine);
  return allPaths;
}



#include <command.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>
#include <soas.hh>
#include <datastackhelper.hh>
#include <terminal.hh>

#include <limits>


static void contourCommand(const QString & /*name*/,
                           QList<double> values,
                           const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();
  QList<Vector> data = ds->allColumns();
  if(data.size() < 3)
    throw RuntimeError("Need at least two Y columns");
  Vector x = data.takeFirst();
  Vector y = ds->perpendicularCoordinates();

  Vector lvls;
  for(double & v : values)
    lvls << v;
  std::sort(lvls.begin(), lvls.end());


  QList<ContourLines> allPaths =
    ContourLines::conrecContour(data, x, y, lvls);

  DataStackHelper pusher(opts);

  for(int k = 0; k < allPaths.size(); k++) {
    double lvl = lvls[k];
    int idx = 0;
    for(const ContourLines::Path & p : allPaths[k].paths) {
      QList<Vector> ncls;
      ncls << p.xvalues << p.yvalues;
      DataSet * nds = ds->derivedDataSet(ncls, "_cont.dat");
      nds->setMetaData("contour_level", lvl);
      nds->setMetaData("contour_idx", idx++);
      pusher << nds;
    }
    Terminal::out << "Found " << idx << " contours for level " << lvl << endl;
  }
}


static ArgumentList 
cntA(QList<Argument *>()
      << new SeveralNumbersArgument("levels",
                                    "Levels", "levels at which to contour",
                                    true)
      );

static ArgumentList 
cntO(QList<Argument *>()
     << DataStackHelper::helperOptions()
     );


static Command 
cntC("contour", // command name
     effector(contourCommand), // action
     "buffer",  // group name
     &cntA, // arguments
     &cntO, // options
     "Contours");
