/*
  dataset.cc: implementation of the DataSet class
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014, 2024 by CNRS/AMU

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
#include <dataset.hh>

#include <math.h>
#include <utils.hh>

#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>

#include <gsl/gsl_blas.h>

#include <exceptions.hh>

#include <terminal.hh>

#include <possessive-containers.hh>

// I don't like that so much, but...
#include <soas.hh>
#include <commandwidget.hh>

#include <datastack.hh>
#include <datasetexpression.hh>
#include <file.hh>

#include <mruby.hh>
#include <idioms.hh>
#include <statistics.hh>

#include <debug.hh>

OrderedList & OrderedList::operator=(const QList<int> & lst)
{
  QList<int>::operator=(lst);
  std::sort(begin(), end());
  return *this;
}

void OrderedList::insert(int idx)
{
  // The lazy-but-actually-smart-thing-to-do
  QList<int>::append(idx);
  std::sort(begin(), end());
}

void OrderedList::shiftAbove(int idx, int delta)
{
  int i = size();
  while(--i >= 0) {
    int v = value(i);
    if(v > idx)
      operator[](i) += delta;
    else
      break;
  }
}

//////////////////////////////////////////////////////////////////////

const Vector & DataSet::column(int i) const {
  if(i >= columns.size())
    throw RuntimeError("Trying to access the %1th column, when dataset has only %2").
      arg(i+1).arg(columns.size());
  return columns[i];
}

Vector & DataSet::column(int i) {
  if(i >= columns.size())
    throw RuntimeError("Trying to access the %1th column, when dataset has only %2").
      arg(i+1).arg(columns.size());
  invalidateCache();
  return columns[i];
}



void DataSet::dump() const
{
  for(int i = 0; i < x().size(); i++)
    for(int j = 0; j < columns.size(); j++)
      Debug::debug()
        << columns[j][i] << (j == columns.size() - 1 ? "\n" : "\t");      
}

int DataSet::byteSize() const
{
  /// @todo use the cache ?
  int s = 0;
  for(int i = 0; i < columns.size(); i++)
    s += columns[i].size();
  s *= sizeof(double);
  return s;
}

QString DataSet::stringDescription(bool longDesc) const
{
  /// @todo Possibly clean that up ?
  if(longDesc) {
    int idx;
    QString index;
    if(soas().stack().indexOf(this, &idx))
      index = QString(", #%1").arg(idx);
    
    QString val = QString("%1: %2 cols, %3 rows, %4 segments%5\n").
      arg(name).arg(nbColumns()).arg(nbRows()).
      arg(segments.size() + 1).arg(index);
    QStringList flgs = QStringList::fromSet(flags);
    std::sort(flgs.begin(), flgs.end());
    val += QString("Flags: %1\n").arg(flgs.join(", "));
    val += "Meta-data:";
    val += metaData.prettyPrint(3, "\t", ",", true);
    if(perpCoords.size() > 0) {
      val += "\nPerpendicular coordinates: " + perpCoords.asText().join(", ") + "\n";
    }
    bool mup = false;
    QStringList coln = mainColumnNames(&mup);
    val += "\nColumn names" + (mup ? QString(" (default)") : QString()) + ": " +
      coln.join(", ");
    return val;
  }
  else
    return QString("%6 %2\t%3\t%5\t'%1'").
      arg(name).arg(nbColumns()).arg(nbRows()).arg(segments.size() + 1).
      arg(flagged() ? "(*)" : "   ");
}

void DataSet::regenerateCache() const
{
  if(isCacheValid())
    return;

  int size = columns.size();
  cache.minima.resize(size);
  cache.maxima.resize(size);

  cache.finiteMinima.resize(size);
  cache.finiteMaxima.resize(size);

  for(int i = 0; i < size; i++) {
    cache.minima[i] = columns[i].min();
    cache.maxima[i] = columns[i].max();
    cache.finiteMinima[i] = columns[i].finiteMin();
    cache.finiteMaxima[i] = columns[i].finiteMax();
  }
  
  cache.valid = true;
}

QRectF DataSet::boundingBox() const
{
  updateCache();
  QRectF r;
  if(columns.size() >= 2)
    r.setCoords(cache.finiteMinima[0], cache.finiteMinima[1], 
                cache.finiteMaxima[0], cache.finiteMaxima[1]);
  return r;
}

QPair<double, double> DataSet::allYBoundaries() const
{
  updateCache();
  double min = 0, max = 0;
  int size = columns.size();
  for(int i = 1; i < size; i++) {
    if(i == 1) {
      min = cache.finiteMinima[i];
      max = cache.finiteMaxima[i];
    }
    else {
      min = std::min(min, cache.finiteMinima[i]);
      max = std::max(max, cache.finiteMaxima[i]);
    }
  }

  return QPair<double, double>(min, max);
}

void DataSet::insertColumn(int idx, const Vector & col)
{
  invalidateCache();
  for(int i = 0; i < columnNames.size(); i++) {
    QStringList & ls = columnNames[i];
    if(ls.size() >= idx) {
      if(i)                     // Empty column name by default
        ls.insert(idx, "");
      else                      // 
        ls.insert(idx, standardNameForColumn(idx));
    }
  }
  /// @question 0 as default perpendicular coordinate upon insert
  if(perpCoords.size() >= idx)
    perpCoords.insert(idx, 0);  
  columns.insert(idx, col);
};

void DataSet::insertRow(int index, double val)
{
  if(index > nbRows())
    throw InternalError("Out of bounds: %1 for %2").
      arg(index).arg(nbRows());
  
  for(int i = 0; i < columns.size(); i++)
    columns[i].insert(index, val);
  segments.shiftAbove(index, 1);

  for(QStringList & lst : rowNames) {
    if(lst.size() > index)
      lst.insert(index, "");
  }
  
  invalidateCache();            // important.
}


Vector DataSet::takeColumn(int idx)
{
  invalidateCache();
  for(QStringList & ls : columnNames) {
    if(ls.size() > idx)
      ls.takeAt(idx);
  }
  
  /// @question we drop the first coordinate to keep the other ones
  if(perpCoords.size() > 0 && perpCoords.size() >= idx)
    perpCoords.takeAt(std::max(idx-1, 0));
  
  return columns.takeAt(idx);
}

void DataSet::selectColumns(const QList<int> & cols)
{
  QList<Vector> nc;
  QList<QStringList> ncn;
  Vector np;

  bool cnok = checkColNames();
  bool perpok = perpCoords.size() == columns.size() -1;

  if(cnok) {
    ncn = columnNames;
    for(QStringList & l : ncn)
      l.clear();
  }
  
  bool first = true;
  for(int c : cols) {
    if(c < 0 || c >= columns.size())
      throw RuntimeError("Invalid column: #%1 (only %2 columns)").
        arg(c).arg(columns.size());
    nc << columns[c];
    if(cnok) {
      for(int i = 0; i < ncn.size(); i++) {
        ncn[i] << columnNames[i].value(c, QString()); // Pad with empty
      }
    }
    // Add perpendicular coordinates for the second data.
    if(!first && perpok && nc.size() > 0) {
      np << perpCoords.value(c-1, 0); // Default to 0
    }
    first = false;
  }
  invalidateCache();
  columns = nc;
  columnNames = ncn;
  perpCoords = np;
}

void DataSet::selectRows(const QList<int> & rows)
{
  QList<Vector> nc;
  nc = columns;
  for(Vector & c : nc)
    c.clear();
  
  bool rnok = checkRowNames();
  QList<QStringList> nrn;

  if(rnok) {
    nrn = rowNames;
    for(QStringList & l : nrn)
      l.clear();
  }

  for(int r : rows) {
    if(r < 0 || r >= nbRows())
      throw RuntimeError("Invalid row: #%1 (only %2 rows)").
        arg(r).arg(nbRows());
    for(int c = 0; c < columns.size(); c++)
      nc[c] << columns[c][r];

    if(rnok) {
      for(int i = 0; i < nrn.size(); i++) {
        nrn[i] << rowNames[i].value(r, QString()); // Pad with empty
      }
    }
  }
  invalidateCache();
  segments.clear();             // really doesn't make sense to keep
                                // them
  columns = nc;
  rowNames = nrn;
}



void DataSet::stripNaNColumns()
{
  for(int i = 0; i < columns.size(); i++) {
    if(columns[i].hasOnlyNaN()) {
      takeColumn(i);
      --i;
    }
  }
}

bool DataSet::hasNotFinite() const
{
  return x().hasNotFinite() || y().hasNotFinite();
}

void DataSet::stripNotFinite()
{
  for(int i = 0; i < x().size(); i++) {
    double xv = x()[i];
    double yv = y()[i];
    if(!std::isfinite(xv) || !std::isfinite(yv))
      removeRow(i--);
  }
}




QPair<double, int> DataSet::distanceTo(double x, double y) const
{
  const double * xvals = columns[0].data();
  const double * yvals = columns[1].data();
  int nb = columns[0].size();
  if(nb == 0)
    return QPair<double, int>(0.0/0.0, -1);

  double dx = x - xvals[0];
  double dy = y - yvals[0];
  double dist = dx*dx + dy*dy;
  int best = 0;
  for(int i = 1; i < nb; i++) {
    dx = x - xvals[i];
    dy = y - yvals[i];
    double d = dx * dx + dy * dy;
    if(d < dist || dist != dist) {
      dist = d;
      best = i;
    }
  }

  return QPair<double, int>(sqrt(dist), best);
}

/// @todo Probably this function should rather join Vector, as it
/// isn't really dataset-specific ? (on the other hand, it isn't too
/// expensive to create a dataset on-demand to use it)
QPair<double, int> DataSet::distanceTo(double x, double y, 
                                       double xs, double ys) const
{
  const double * xvals = columns[0].data();
  const double * yvals = columns[1].data();
  int nb = columns[0].size();
  if(nb == 0)
    return QPair<double, int>(0.0/0.0, -1);

  double dx = (x - xvals[0])*xs;
  double dy = (y - yvals[0])*ys;
  double dist = dx*dx + dy*dy;
  int best = 0;
  for(int i = 1; i < nb; i++) {
    dx = (x - xvals[i])*xs;
    dy = (y - yvals[i])*ys;
    double d = dx * dx + dy * dy;
    if(d < dist || dist != dist) {
      dist = d;
      best = i;
    }
  }

  return QPair<double, int>(sqrt(dist), best);
}


QString DataSet::cleanedName() const
{
  int idx = name.lastIndexOf('.');
  int sz = name.size();
  // Maximum size of an extension to remove this way: 6
  if(idx > 0 && (sz - idx < 6))
    return name.left(idx);
  return name;
}

void DataSet::splitAt(int idx, DataSet ** first, DataSet ** second) const
{
  if(first) {
    QList<Vector> newcols;
    for(int i = 0; i < columns.size(); i++)
      newcols << Vector(columns[i].mid(0, idx+1));
    /// @todo Shall we handle the name change here ? Why not, after
    /// all ? Things still can change later on.
    *first = derivedDataSet(newcols, "_a.dat");
  }
  if(second) {
    if(idx >= nbRows())         // Ensure we don't go over.
      idx = nbRows() - 1;
    QList<Vector> newcols;
    for(int i = 0; i < columns.size(); i++)
      newcols << Vector(columns[i].mid(idx));
    *second = derivedDataSet(newcols, "_b.dat");
  }
}

/// Small helper for chop
static void pushNext(const DataSet * ds,
                     QList<DataSet *> * target, QList<int> * indices,
                     int s, int e)
{
  if(indices) {
    if(e+1 < ds->nbRows())
      indices->append(e+1);     // No need to push the last one ! 
  }
  else
    target->append(ds->subset(s, e));
 }

QList<DataSet *> DataSet::chop(const QList<double> & lengths, 
                               bool isLength, QList<int> * indices) const
{
  QList<DataSet *> retvals;
  int lastidx = 0;
  double tdx = 0;
  int size;
  int curIdx = 0;
  const double *x = getValues(0, &size);
  if(isLength) {
    for(int i = 1; i < size; i++) {
      double dx = fabs(x[i] - x[i-1]);
      tdx += dx;
      if(tdx >= lengths[curIdx]) {
        pushNext(this, &retvals, indices, lastidx, i-1);
        lastidx = i;
        tdx = dx; // We restart as if the previous split had finished
        curIdx++;
        if(curIdx >= lengths.size())
          break;
      }
    }
  }
  else {
    for(int i = 0; i < size; i++) {
      if(x[i] >= lengths[curIdx]) {
        pushNext(this, &retvals, indices, lastidx, i-1);
        lastidx = i;
        curIdx++;
        if(curIdx >= lengths.size())
          break;
      }
    }
  }
  if(lastidx < size - 1)
    pushNext(this, &retvals, indices, lastidx, size-1);
  return retvals;
}

QList<DataSet *> DataSet::chop(const QList<int> & indices) const
{
  QList<DataSet *> retvals;
  for(int i = 0; i <= indices.size(); i++) {
    int s = indices.value(i-1, 0);
    int e = indices.value(i, x().size()) - 1;
    DataSet * ds = subset(s, e);
    ds->name = cleanedName() + QString("_@%1_to_@%2.dat").
      arg(s).arg(e);
    retvals << ds;
  }
  return retvals;
}


QList<DataSet *> DataSet::chopIntoSegments() const
{
  QList<DataSet *> retvals = chop(segments);
  for(int i = 0; i < retvals.size(); i++) {
    retvals[i]->name = cleanedName() + QString("_seg#%1.dat").
      arg(i);
    retvals[i]->segments.clear();
  }
  return retvals;
}


const double * DataSet::getValues(int col, int * size) const
{
  if(size)
    *size = columns[col].size();
  return columns[col].data();
}

int DataSet::deltaSignChange(int col) const
{
  /// @todo Most probably this function should join
  /// Vector.
  int size;
  const double *val = getValues(col, &size);
  if(size < 3)
    return  -1;
  double dv0 = val[1] - val[0];
  for(int i = 1; i < size - 1; i++) {
    double dv = val[i+1] - val[i];
    if(dv * dv0 < 0)
      return i;
  }
  return -1;
}


QList<DataSet *> DataSet::splitIntoMonotonic(int col, int group) const
{
  QList<DataSet *> ret;
  
  int size;
  const double *val = getValues(col, &size);
  int idx = 1;
  int curStart = 0;
  int nbcrossed = 0;
  if(size >= 3) {
    double dv0 = val[1] - val[0];
    bool last0 = false;
    for(idx = 1; idx < size - 1; idx++) {
      if(dv0 == 0) {
        dv0 = val[idx+1] - val[idx];
        continue;
      }
      double dv = val[idx+1] - val[idx];
      if(dv == 0) {
        last0 = true;
        continue;
      }
      if(dv * dv0 < 0) {
        nbcrossed += 1;
        if(nbcrossed % group == 0) {
          QList<Vector> cols;
          if(last0)
            idx--;
          for(int j = 0; j < columns.size(); j++)
            cols << columns[j].mid(curStart, idx - curStart + 1);
          ret << derivedDataSet(cols,
                                QString("_part%1.dat").arg(ret.size()));
          curStart = idx+1;
        }

        idx += 2;
        if(idx < size)
          dv0 = val[idx] - val[idx-1];
      }
      last0 = false;
    }
  }
  QList<Vector> cols;
  for(int j = 0; j < columns.size(); j++)
    cols << columns[j].mid(curStart, columns[j].size() - curStart);
  ret << derivedDataSet(cols,
                        QString("_part%1.dat").arg(ret.size()));
  return ret;
}


DataSet * DataSet::applyBinaryOperation(const DataSet * a,
                                        const DataSet * b,
                                        double (*op)(double, double),
                                        const QString & cat,
                                        DataSet::BinaryOperationMode mode,
                                        bool useSteps,
                                        int useACol)
{
  // only deal with the common columns
  int nbcols = useACol >= 0 ? b->nbColumns() : 
    std::min(a->nbColumns(), b->nbColumns());
  
  if(nbcols < 2)
    throw RuntimeError("Need at least a Y column for both datasets");

  QList<Vector> vects; 
  if(useSteps) {

    // We first split into segments, shift X values, apply and then
    // combine everything back
    
    // We use possessive lists to avoid memory leaks on exceptions.

    /// @todo This is rather memory-intensive and a waste of
    /// resources, it could be optimized by not allocating new things.
    PossessiveList<DataSet> ads(a->chopIntoSegments());
    PossessiveList<DataSet> bds(b->chopIntoSegments());

    if(bds.size() < ads.size())
      throw RuntimeError(QString("Dataset '%1' has less segments than '%2': "
                                 "cannot perform segment-by-segment "
                                 "operation !").
                         arg(b->name).arg(a->name));

    for(int j = 0; j < nbcols-1; j++)
      vects << Vector();
    for(int i = 0; i < ads.size(); i++) {
      ads[i]->x() -= ads[i]->x()[0];
      bds[i]->x() -= bds[i]->x()[0];
      DataSet * nds = applyBinaryOperation(ads[i], bds[i], op,
                                           cat, mode, false, useACol);
      for(int j = 0; j < nbcols-1; j++)
        vects[j] << nds->columns[j+1];

      delete nds;
    }
    
    vects.insert(0, a->x());
  }
  else {
    

    int size_a = a->nbRows();
    const double * xa = a->columns[0].data();

    int size_b = b->nbRows();
    const double * xb = b->columns[0].data();

    for(int i = 0; i < nbcols; i++)
      vects << Vector();
    switch(mode) {
    case Indices:
      if(size_a > size_b)
        throw RuntimeError("Not enough points in dataset '%1': %2 vs %3").
          arg(b->name).arg(size_b).arg(size_a);
      
      for(int i = 0; i < size_a; i++) {
        vects[0] << xa[i];
        for(int k = 1; k < nbcols; k++)
          vects[k] << op(a->columns[useACol >= 0 ? useACol : k][i], b->columns[k][i]);
      }
      break;
    case ClosestX:
    case Extend:
      {
        double xb_min = b->x().min(),
          xb_max = b->x().max();
        double maxDx = (xb_max - xb_min)/size_b * 2;
        for(int i = 0; i < size_a; i++) {
          if(mode == ClosestX && ((xa[i] < xb_min - maxDx) ||
                                  (xa[i] > xb_max + maxDx)))
            throw RuntimeError("Trying to extend dataset %1 too far: "
                               "%2 for ([%3,%4]), use /mode=extend").
              arg(b->name).arg(xa[i]).arg(xb_min).arg(xb_max);

          /* We first look for the closest point */
          double diff = fabs(xa[i] - xb[0]);
          int found = 0;
          // We do not assume that X values are varying 
          for(int j = 0; j < size_b; j++) {
            double d = fabs(xa[i] - xb[j]);
            if(d < diff) {
              diff  = d;
              found = j;
            }
          }
          vects[0] << xa[i];        // a is the master dataset
          for(int k = 1; k < nbcols; k++)
            vects[k] << op(a->columns[useACol >= 0 ? useACol : k][i], b->columns[k][found]);
        }
      }
      break;
    case Strict:
      {
        for(int i = 0; i < size_a; i++) {
          int found = -1;
          for(int j = 0; j < size_b; j++) {
            if(xb[j] == xa[i]) {
              found = j;
              break;
            }
          }
          vects[0] << xa[i];        // a is the master dataset
          for(int k = 1; k < nbcols; k++)
            vects[k] << op(a->columns[useACol >= 0 ? useACol : k][i], b->columns[k].value(found, std::nan("0")));
              
        }
      }
      break;
    default:
      throw InternalError("Unknown mode");
    }
  }

  DataSet * ds = a->derivedDataSet(vects, 
                                   cat + b->cleanedName() + ".dat");

  /// @todo Add meta-data to track which buffer was combined into this one ?

  // We put back the segments present in the first dataset
  ds->segments = a->segments;
  return ds;
}

static inline double sub(double a, double b)
{
  return a - b;
}

DataSet * DataSet::subtract(const DataSet * ds,
                            BinaryOperationMode mode,
                            bool useSteps) const
{
  return applyBinaryOperation(this, ds, sub, "-", mode, useSteps);
}

static inline double add(double a, double b)
{
  return a + b;
}

DataSet * DataSet::add(const DataSet * ds,
                       BinaryOperationMode mode,
                       bool useSteps) const
{
  return applyBinaryOperation(this, ds, ::add, "+", mode, useSteps);
}

static inline double mul(double a, double b)
{
  return a * b;
}

DataSet * DataSet::multiply(const DataSet * ds,
                            BinaryOperationMode mode,
                            bool useSteps) const
{
  return applyBinaryOperation(this, ds, ::mul, "*", mode, useSteps);
}

static inline double div(double a, double b)
{
  return a/b;
}

DataSet * DataSet::divide(const DataSet * ds,
                          BinaryOperationMode mode,
                          bool useSteps) const
{
  return applyBinaryOperation(this, ds, div, "_div_", mode, useSteps);
}

static inline double keep_second(double, double b)
{
  return b;
}

DataSet * DataSet::merge(const DataSet * ds,
                         BinaryOperationMode mode,
                         bool useSteps) const
{
  DataSet * nd = applyBinaryOperation(this, ds, keep_second, 
                                      "_merged_", mode, useSteps);
  nd->columns << nd->columns[0];
  nd->columns[0] = columns[1];
  return nd;
}

DataSet * DataSet::contract(const DataSet * ds,
                            BinaryOperationMode mode,
                            bool useSteps) const
{
  DataSet * nd = applyBinaryOperation(this, ds, keep_second, 
                                      "_cont_", mode, useSteps, 0);
 
  Vector pc = ds->perpCoords;
  for(int i = 1; i < columns.size(); i++)
    nd->columns.insert(i, columns[i]);
  nd->perpCoords << pc;

  // Now splice the column names
  nd->columnNames = columnNames;
  for(int i = 0; i < columnNames.size(); i++) {
    QStringList nnames;
    if(ds->columnNames.size() > i)
      nnames = ds->columnNames[i].mid(1, ds->nbColumns()-1);
    while(nnames.size() < ds->nbColumns()-1)
      nnames << "";             // Insert empty names
    nd->columnNames[i] += nnames;
  }

  return nd;
}




/// Performs a linear regression on the given data, storing the result in
/// \a a and \a b.
static void reglin(const double *x, const double *y, int nb, 
		   double *a, double *b)
{
  double sx = 0;
  double sy = 0;
  double sxx = 0;
  double sxy = 0;
  double det;

  for(int i = 0; i < nb; i++, x++, y++) {
    sx += *x;
    sy += *y;
    sxx += *x * *x;
    sxy += *x * *y;
  }
  det = nb*sxx - sx*sx;

  if(det == 0) {
    *a = 0;			/* Whichever, we only have one point */
    *b = sy/nb;
  }

  else {
    *a = (nb *sxy - sx*sy)/det; 
    *b = (sxx * sy - sx * sxy)/(det);
  }
}

QPair<double, double> DataSet::reglin(int begin, int end, int ycol) const
{
  int nb = nbRows();
  if(end < 0 || end > nb)
    end = nb;
  const double * x = columns[0].data();
  const double * y = columns[ycol].data();
  QPair<double, double> retval;
  ::reglin(x + begin, y + begin, (end - begin), 
           &retval.first, &retval.second);
  return retval;
}

QPair<double, double> DataSet::reglin(double xmin, double xmax) const
{
  int nb = nbRows();
  const double * x = columns[0].data();
  const double * y = columns[1].data();

  double sx = 0;
  double sy = 0;
  double sxx = 0;
  double sxy = 0;
  int total = 0;
  for(int i = 0; i < nb; i++, x++, y++) {
    if(! (*x >= xmin && *x <= xmax)) {
      continue;
    }
    sx += *x;
    sy += *y;
    sxx += *x * *x;
    sxy += *x * *y;
    total++;
  }
  double det = total*sxx - sx*sx;
  if(det == 0)
    return QPair<double, double>(0, sy/total);
  else
    return QPair<double, double>((total *sxy - sx*sy)/det,
                                 (sxx * sy - sx * sxy)/det);
}

void DataSet::reverse()
{
  invalidateCache();
  for(int i = 0; i < columns.size(); i++)
    columns[i].reverse();

  for(QStringList & rn : rowNames) {
    int sz = rn.size();
    for(int i = 0; i < sz/2; i++)
      std::swap(rn[i], rn[sz-i-1]);
  }

  // Update segments:
  int sz = nbRows();
  for(int j = 0; j < segments.size(); j++)
    segments[j] = sz - segments[j];
  std::sort(segments.begin(), segments.end());
}


DataSet * DataSet::subset(int beg, int end, bool within) const
{
  
  QList<Vector> cols;
  int nb = nbRows();
  if(columns.size() == 0 || nb == 0)
    return derivedDataSet("_empty.dat");
  // Sanity checking for indices
  if(beg < 0)
    beg = 0;
  if(beg >= nb)
    beg = nb - 1;
  if(end < beg)
    end = beg;
  if(end >= nb)
    end = nb-1;
    
  QList<int> newSegs;

  QList<QStringList> rn;
  if(within) {
    for(int i = 0; i < columns.size(); i++)
      cols << columns[i].mid(beg, 1+end-beg);

    for(const QStringList &  r: rowNames)
      rn << r.mid(beg, 1+end-beg);

    // Now adjust segments:
    for(int i = 0; i < segments.size(); i++) {
      int idx = segments[i];
      if(idx >= beg && idx <= end)
        newSegs << idx - beg;
    }
  }
  else {
    for(int i = 0; i < columns.size(); i++) {
      Vector v = columns[i].mid(0, beg);
      v << columns[i].mid(end);
      cols << v;
    }

    for(const QStringList &  r: rowNames) {
      QStringList nc = r.mid(0, beg);
      nc << r.mid(end);
      rn << nc;
    }

    for(int i = 0; i < segments.size(); i++) {
      int idx = segments[i];
      if(idx <= beg)
         newSegs << idx;
      else if(idx >= end)
        newSegs << idx - (end - beg);
    }
  }


  DataSet * ds = derivedDataSet(cols, QString("_%1from_%2_to_%3.dat").
                                arg(within ? "" : "excl_").
                                arg(columns[0][beg]).arg(columns[0][end]));
  ds->rowNames = rn;

  ds->segments = newSegs;
  return ds;
}

DataSet * DataSet::removeSpikes(int nbc, double extra) const
{
  int nb = 0, nb2;
  QList<Vector> cols;
  cols += columns[0].removeSpikes(nbc, extra, &nb2);
  if(nb2) {
    nb = nb2;
    Terminal::out << "Found " << nb2 << " spikes on X values" << endl;
  }

  cols += columns[1].removeSpikes(nbc, extra, &nb2);
  if(nb2) {
    nb += nb2;
    Terminal::out << "Found " << nb2 << " spikes on Y values" << endl;
  }

  if(! nb2)
    return NULL;

  return derivedDataSet(cols, "_spikes.dat");
}


/* Simply returns the sign */
static int signof(double x)
{
  if(x > 0)
    return 1;
  else if(x < 0)
    return -1;
  else
    return 0;
}

/// The smooth pick algorithm, see 10.1016/j.bioelechem.2009.02.010
///
/// @todo This implementation is absolutely not what I remember from
/// smooth picks
double smooth_pick(const double *x, const double *y, 
		   int nb, int idx, int range)
{
  int left, right,i,nb_same_sign = 0;
  double a,b;
  int last_sign;
  do {
    left = idx - range/2;
    if(left < 0) 
      left = 0;
    right = idx + range/2;
    if(right > nb)
      right = nb;
    reglin(x+left, y+left, right-left,&a,&b);
    if(range == 6)
      break; 			/* We stop here */
    range = right - left;
    last_sign = 0;
    for(i = left; i < right; i++) {
      double residual = y[i] - a * x[i] - b;
      if(! last_sign)
	last_sign = signof(residual);
      else if(last_sign == signof(residual))
	nb_same_sign ++;
      else {
	nb_same_sign = 1;
	last_sign = signof(residual);
      }
    }
    if(nb_same_sign * 4 <= right - left)
      break;
    range -= (nb_same_sign * 4 -range)/2 + 2;
    if(range < 6)
      range = 6;
  } while(1);
  /* Now, we have a and b for the last range measured. */
  return a*x[idx] + b;
}

QPointF DataSet::smoothPick(int idx, int range) const
{
  int nb = nbRows();
  const double * x = columns[0].data();
  const double * y = columns[1].data();
  if(range < 0)
    range = nb > 500 ? 50 : nb/10;
  QPointF ret = pointAt(idx);
  ret.setY(smooth_pick(x, y, nb, idx, range));
  return ret;
}

double DataSet::yValueAt(double x, bool interpolate) const
{
  const double * xvals = columns[0].data();
  const double * yvals = columns[1].data();
  int nb = columns[0].size();
  if(nb < 2)
    return yvals[0];

  double lastdx = xvals[0] - x;
  for(int i = 1; i < nb; i++) {
    double dx = xvals[i] - x;
    if(dx*lastdx <= 0) {        // sign change, we passed it !
      if(interpolate) {
        if(lastdx == dx)
          return 0.5*(yvals[i] + yvals[i-1]);
        // Take care of the case when two successive X values are identical
        return (lastdx * yvals[i] - dx * yvals[i-1])/(lastdx - dx);
      }
      else {
        if(fabs(dx) < fabs(lastdx))
          return yvals[i];
        else
          return yvals[i-1];
      }
    }
    lastdx = dx;
  }
  if(fabs(x-xvals[0]) > fabs(x - xvals[nb-1]))
    return yvals[nb-1];
  return yvals[0];
}


// void DataSet::write(QIODevice * target) const
// {
//   DataSetWriter writer;
//   writer.writeDataSet(target, this);
// }

// void DataSet::write(const QString & n) const
// {
//   QString fileName = n;
//   if(fileName.isEmpty())
//     fileName = cleanedName() + ".dat";

//   File file(fileName, File::TextOverwrite);
//   write(file);
// }

QList<int> DataSet::findSteps(int nb, double threshold) const
{
  QList<int> ret;
  
  // Threshold is relative.
  threshold *= (y().max() - y().min());

  const double *xv = x().data();
  const double *yv = y().data();
  int size = x().size();
  if(size < 2 * nb)
    return ret;

  /// The number of points left since we detected something, or 0 if
  /// there was not a step hinted recently.
  int nbLeft = 0;
  double maxdelta = 0;
  int posDelta = 0;// Position of the maximum delta

  for(int i = nb; i <= size - nb; i++) {
    double aleft, bleft;
    double aright, bright;
    // Left prediction
    ::reglin(xv + (i-nb), yv + (i-nb), nb, 
           &aleft, &bleft);
    // Right prediction
    ::reglin(xv + i, yv + i, nb, &aright, &bright);

    // We compute the point between i and i-1 as both regressions
    // would predict
    double yleft = aleft * 0.5*(xv[i] + xv[i-1]) + bleft;
    double yright = aright * 0.5*(xv[i] + xv[i-1]) + bright;

    double delta = fabs(yleft - yright);
    if(nbLeft > 0) {
      nbLeft--;
      if(delta > maxdelta) {
        maxdelta = delta;
        posDelta = i;
      }
      if(nbLeft == 0) {
        ret << posDelta;
        i = posDelta + nb + 1;
      }
    }
    else if(delta >= threshold) {
      nbLeft = nb;
      maxdelta = delta;
      posDelta = i;
    }
  }
  if(nbLeft > 0)
    ret << posDelta;
  return ret;
}

static bool lessThan(const QPair<double, int> &a, 
              const QPair<double, int> &b) {
  return a.first < b.first;
}

static bool greaterThan(const QPair<double, int> &a, 
                 const QPair<double, int> &b) {
  return a.first > b.first;
}

DataSet * DataSet::sort(bool reverse, int col) const
{
  /// @todo This algorithm isn't fast nor space-efficient, but it does
  /// work, and its simple. (well, it's still n ln(n) excepted in
  /// pathological cases)

  QList< QPair<double, int> > vals;

  const double *xv = columns[col].data();
  int size = x().size();
  vals.reserve(size);
  for(int i = 0; i < size; i++)
    vals << QPair<double, int>(xv[i], i);
  
  if(reverse)
    std::sort(vals.begin(), vals.end(), &greaterThan);
  else
    std::sort(vals.begin(), vals.end(), &lessThan);

  /// Hmm.  @perf Make sure select works with iterators rather than
  /// with lists.
  QList<int> rows;
  for(const QPair<double, int> & p : vals)
    rows << p.second;

  DataSet * rv = derivedDataSet("_sorted.dat");

  rv->selectRows(rows);
  return rv;
}

DataSet * DataSet::derivedDataSet(const QList<Vector> &newCols, 
                                  const QString & suffix) const
{
  DataSet * ds = new DataSet(newCols);
  ds->name = cleanedName() + suffix;
  ds->metaData = metaData;      // Copy !
  // add a "derived-from" attribute ?

  /// @todo Optionnally drop the segments (with an optional argument ?)
  ds->segments = segments;

  // We append the current command to the "commands" key
  ds->metaData.appendToList("commands", soas().currentCommandLine());

  // We copy the options !
  ds->options = options;

  // We copy the perpendicular coordinates
  ds->perpCoords = perpCoords;


  // Now dealing wih the columns

  // keeping the row names only when the number of rows hasn't changed.
  if(ds->nbRows() == nbRows()) {
    ds->rowNames = rowNames;
    // perpcoordnames is in fact the name of the row "above"
    ds->perpCoordNames = perpCoordNames;
  }
  /// @todo else log ?

  if(checkRowNames()) {
    // Copy, adding and removing if necessary
    for(int i = 0; i < columnNames.size(); i++) {
      ds->columnNames << columnNames[i];
      // padding with standard names
      while(ds->columnNames[i].size() < ds->nbColumns())
        ds->columnNames[i] << standardNameForColumn(ds->columnNames[i].size());
      // if too large, truncate.
      if(ds->columnNames[i].size() > ds->nbColumns())
        ds->columnNames[i] = ds->columnNames[i].mid(0, ds->nbColumns());
    }
  }


  /// @question Should we update the date too ?
  return ds;
}


DataSet * DataSet::derivedDataSet(const Vector &newy, 
                                  const QString & suffix,
                                  const Vector & newx) const
{
  QList<Vector> newCols = columns;
  newCols[1] = newy;
  if(newx.size() > 0)
    newCols[0] = newx;

  return derivedDataSet(newCols, suffix);
}

DataSet * DataSet::derivedDataSet(const QList<QPointF> &points, 
                                  const QString & suffix) const
{
  Vector nx;
  Vector ny;
  for(int i = 0; i < points.size(); i++) {
    const QPointF & pt = points[i];
    nx << pt.x();
    ny << pt.y();
  }
  QList<Vector> nc;
  nc << nx << ny;
  return derivedDataSet(nc, suffix);
}

DataSet * DataSet::derivedDataSet(const QString & suffix) const
{
  return derivedDataSet(columns, suffix);
}


void DataSet::firstDerivative(const double *x, int xstride, 
                              const double *y, int ystride, 
                              double * target, int tstride,
                              int size, bool silent)
{

  int i;
  double delta_1, delta_2, delta_3, delta_4;
  double alpha_1, alpha_2, alpha_3, alpha_4;
  double v0,v1,v2,v3,v4;

  if(size < 5)
    throw RuntimeError("Need at least 5 points");

  for(i = 0; i < size; i++) {
    /* First initialize values, though this is very suboptimal */
    v0 = y[i * ystride];
    if(i == 0) {
      delta_1 = x[1 * xstride] - x[0]; v1 = y[1 * ystride];
      delta_2 = x[2 * xstride] - x[0]; v2 = y[2 * ystride];
      delta_3 = x[3 * xstride] - x[0]; v3 = y[3 * ystride];
      delta_4 = x[4 * xstride] - x[0]; v4 = y[4 * ystride];
    } else if(i == 1) {
      delta_1 = x[0 * xstride] - x[1 * xstride]; v1 = y[0 * ystride];
      delta_2 = x[2 * xstride] - x[1 * xstride]; v2 = y[2 * ystride];
      delta_3 = x[3 * xstride] - x[1 * xstride]; v3 = y[3 * ystride];
      delta_4 = x[4 * xstride] - x[1 * xstride]; v4 = y[4 * ystride];
    } else if(i == size - 2) {
      delta_1 = x[(size-1) * xstride] - x[(size-2) * xstride]; v1 = y[(size-1) * ystride];
      delta_2 = x[(size-3) * xstride] - x[(size-2) * xstride]; v2 = y[(size-3) * ystride];
      delta_3 = x[(size-4) * xstride] - x[(size-2) * xstride]; v3 = y[(size-4) * ystride];
      delta_4 = x[(size-5) * xstride] - x[(size-2) * xstride]; v4 = y[(size-5) * ystride];
    } else if(i == size - 1) {
      delta_1 = x[(size-2) * xstride] - x[(size-1) * xstride]; v1 = y[(size-2) * ystride];
      delta_2 = x[(size-3) * xstride] - x[(size-1) * xstride]; v2 = y[(size-3) * ystride];
      delta_3 = x[(size-4) * xstride] - x[(size-1) * xstride]; v3 = y[(size-4) * ystride];
      delta_4 = x[(size-5) * xstride] - x[(size-1) * xstride]; v4 = y[(size-5) * ystride];
    } else {
      delta_1 = x[(i-2) * xstride] - x[i * xstride]; v1 = y[(i-2) * ystride];
      delta_2 = x[(i-1) * xstride] - x[i * xstride]; v2 = y[(i-1) * ystride];
      delta_3 = x[(i+2) * xstride] - x[i * xstride]; v3 = y[(i+2) * ystride];
      delta_4 = x[(i+1) * xstride] - x[i * xstride]; v4 = y[(i+1) * ystride];
    }
    if((! silent) &&
       (
        (delta_1 == 0) || (delta_2 == 0)
        || (delta_3 == 0) || (delta_4 == 0)
        )
       )
      throw RuntimeError("Duplicate value of X in the derivative around x = %1").
       arg(x[i*xstride]);
    alpha_1 = delta_2*delta_3*delta_4/
      (delta_1 * (delta_2 - delta_1) * (delta_3 - delta_1) 
       * (delta_4 - delta_1));
    alpha_2 = delta_1*delta_3*delta_4/
      (delta_2 * (delta_1 - delta_2) * (delta_3 - delta_2) 
       * (delta_4 - delta_2));
    alpha_3 = delta_1*delta_2*delta_4/
      (delta_3 * (delta_1 - delta_3) * (delta_2 - delta_3) 
       * (delta_4 - delta_3));
    alpha_4 = delta_1*delta_2*delta_3/
      (delta_4 * (delta_1 - delta_4) * (delta_2 - delta_4) 
       * (delta_3 - delta_4));
    target[i * tstride] =  -(alpha_1 + alpha_2 + alpha_3 + alpha_4) * v0 +
      alpha_1 * v1 + alpha_2 * v2 + 
      alpha_3 * v3 + alpha_4 * v4;
  }

}


DataSet * DataSet::firstDerivative() const
{
  int size = x().size();
  if(size < 5)
    throw RuntimeError("Need at least 5 points");
  const double *x = this->x().data();
  const double *y = this->y().data();

  Vector deriv;
  int i;
  double delta_1, delta_2, delta_3, delta_4;
  double alpha_1, alpha_2, alpha_3, alpha_4;
  double v0,v1,v2,v3,v4;
  /* TODO: what happens when there are less than 5 points ? */

  for(i = 0; i < size; i++) {
    /* First initialize values, though this is very suboptimal */
    v0 = y[i];
    if(i == 0) {
      delta_1 = x[1] - x[0]; v1 = y[1];
      delta_2 = x[2] - x[0]; v2 = y[2];
      delta_3 = x[3] - x[0]; v3 = y[3];
      delta_4 = x[4] - x[0]; v4 = y[4];
    } else if(i == 1) {
      delta_1 = x[0] - x[1]; v1 = y[0];
      delta_2 = x[2] - x[1]; v2 = y[2];
      delta_3 = x[3] - x[1]; v3 = y[3];
      delta_4 = x[4] - x[1]; v4 = y[4];
    } else if(i == size - 2) {
      delta_1 = x[size-1] - x[size-2]; v1 = y[size-1];
      delta_2 = x[size-3] - x[size-2]; v2 = y[size-3];
      delta_3 = x[size-4] - x[size-2]; v3 = y[size-4];
      delta_4 = x[size-5] - x[size-2]; v4 = y[size-5];
    } else if(i == size - 1) {
      delta_1 = x[size-2] - x[size-1]; v1 = y[size-2];
      delta_2 = x[size-3] - x[size-1]; v2 = y[size-3];
      delta_3 = x[size-4] - x[size-1]; v3 = y[size-4];
      delta_4 = x[size-5] - x[size-1]; v4 = y[size-5];
    } else {
      delta_1 = x[i-2] - x[i]; v1 = y[i-2];
      delta_2 = x[i-1] - x[i]; v2 = y[i-1];
      delta_3 = x[i+2] - x[i]; v3 = y[i+2];
      delta_4 = x[i+1] - x[i]; v4 = y[i+1];
    }
    alpha_1 = delta_2*delta_3*delta_4/
      (delta_1 * (delta_2 - delta_1) * (delta_3 - delta_1) 
       * (delta_4 - delta_1));
    alpha_2 = delta_1*delta_3*delta_4/
      (delta_2 * (delta_1 - delta_2) * (delta_3 - delta_2) 
       * (delta_4 - delta_2));
    alpha_3 = delta_1*delta_2*delta_4/
      (delta_3 * (delta_1 - delta_3) * (delta_2 - delta_3) 
       * (delta_4 - delta_3));
    alpha_4 = delta_1*delta_2*delta_3/
      (delta_4 * (delta_1 - delta_4) * (delta_2 - delta_4) 
       * (delta_3 - delta_4));
    deriv << -(alpha_1 + alpha_2 + alpha_3 + alpha_4) * v0 +
      alpha_1 * v1 + alpha_2 * v2 + 
      alpha_3 * v3 + alpha_4 * v4;
  }
  return derivedDataSet(deriv, "_diff");
}

DataSet * DataSet::secondDerivative() const
{
  int size = x().size();
  if(size < 5)
    throw RuntimeError("Need at least 5 points");
  const double *x = this->x().data();
  const double *y = this->y().data();

  Vector deriv;
  int i;
  double delta_1, delta_2, delta_3, delta_4;
  double alpha_1, alpha_2, alpha_3, alpha_4;
  double v0,v1,v2,v3,v4;
  /* TODO: what happens when there are less than 5 points ? */

  for(i = 0; i < size; i++) {
    /* First initialize values, though this is very suboptimal */
    v0 = y[i];
    if(i == 0) {
      delta_1 = x[1] - x[0]; v1 = y[1];
      delta_2 = x[2] - x[0]; v2 = y[2];
      delta_3 = x[3] - x[0]; v3 = y[3];
      delta_4 = x[4] - x[0]; v4 = y[4];
    } else if(i == 1) {
      delta_1 = x[0] - x[1]; v1 = y[0];
      delta_2 = x[2] - x[1]; v2 = y[2];
      delta_3 = x[3] - x[1]; v3 = y[3];
      delta_4 = x[4] - x[1]; v4 = y[4];
    } else if(i == size - 2) {
      delta_1 = x[size-1] - x[size-2]; v1 = y[size-1];
      delta_2 = x[size-3] - x[size-2]; v2 = y[size-3];
      delta_3 = x[size-4] - x[size-2]; v3 = y[size-4];
      delta_4 = x[size-5] - x[size-2]; v4 = y[size-5];
    } else if(i == size - 1) {
      delta_1 = x[size-2] - x[size-1]; v1 = y[size-2];
      delta_2 = x[size-3] - x[size-1]; v2 = y[size-3];
      delta_3 = x[size-4] - x[size-1]; v3 = y[size-4];
      delta_4 = x[size-5] - x[size-1]; v4 = y[size-5];
    } else {
      delta_1 = x[i-2] - x[i]; v1 = y[i-2];
      delta_2 = x[i-1] - x[i]; v2 = y[i-1];
      delta_3 = x[i+2] - x[i]; v3 = y[i+2];
      delta_4 = x[i+1] - x[i]; v4 = y[i+1];
    }
    alpha_1 = -2 * (delta_2*delta_3 + delta_2*delta_4 + delta_3*delta_4)/
      (delta_1 * (delta_2 - delta_1) * (delta_3 - delta_1) 
       * (delta_4 - delta_1));
    alpha_2 = -2 * (delta_1*delta_3 + delta_1*delta_4 + delta_3*delta_4)/
      (delta_2 * (delta_1 - delta_2) * (delta_3 - delta_2) 
       * (delta_4 - delta_2));
    alpha_3 = -2 * (delta_2*delta_1 + delta_2*delta_4 + delta_1*delta_4)/
      (delta_3 * (delta_1 - delta_3) * (delta_2 - delta_3) 
       * (delta_4 - delta_3));
    alpha_4 = -2 * (delta_2*delta_3 + delta_2*delta_1 + delta_3*delta_1)/
      (delta_4 * (delta_1 - delta_4) * (delta_2 - delta_4) 
       * (delta_3 - delta_4));
    deriv << -(alpha_1 + alpha_2 + alpha_3 + alpha_4) * v0 +
			alpha_1 * v1 + alpha_2 * v2 + 
			alpha_3 * v3 + alpha_4 * v4;
  }
  return derivedDataSet(deriv, "_diff2");
}

DataSet * DataSet::arbitraryDerivative(int deriv, int order) const
{
  int size = x().size();
  Vector res = x();

  arbitraryDerivative(deriv, order, size, x().constData(),
                      y().constData(), res.data());
  
  return derivedDataSet(res, QString("_diff%1o%2").arg(deriv).arg(order-1));
}

void DataSet::arbitraryDerivative(int deriv, int order, int size,
                                  const double * x, const double * y,
                                  double * target)
{
  if(size < order)
    throw RuntimeError("Need at least %1 points for a derivate of order %2").
      arg(order+1).arg(order);

  if(deriv > order)
    throw RuntimeError("Order must be at least as large as the derivative order").
      arg(order+1).arg(order);

  order += 1;

  gsl_matrix * mat = gsl_matrix_alloc(order, order);
  gsl_vector * rhs = gsl_vector_alloc(order);
  gsl_vector * dervs = gsl_vector_alloc(order);
  gsl_permutation * perm = gsl_permutation_alloc(order);

  for(int i = 0; i < size; i++) {
    int left = std::min(std::max(i-order/2, 0), size-order);
    for(int j = left; j < left + order; ++j) {
      double dx = x[j] - x[i];
      double v = 1;
      int row = j - left;
      gsl_vector_set(rhs, row, y[j]);
      for(int k = 0; k < order; k++) {
        if(k > 0)
          v /= k;
        gsl_matrix_set(mat, row, k, v);
        v *= dx;
      }
    }

    // OK, now, we have a matrix of dx_i^k/k!, and a vector of the
    // functions. We just need to invert the matrix, and well get the
    // Taylor series

    int sgn;
    gsl_linalg_LU_decomp(mat, perm, &sgn);
    gsl_linalg_LU_solve(mat, perm, rhs, dervs);
    target[i] = gsl_vector_get(dervs, deriv);
  }

  gsl_matrix_free(mat);
  gsl_vector_free(rhs);
  gsl_vector_free(dervs);
  gsl_permutation_free(perm);
}





DataSet * DataSet::concatenateDataSets(QList<const DataSet *> datasets, 
                                       bool set)
{
  QList<int> segs;
  int nbcols = datasets.first()->nbColumns();
  for(int i = 1; i < datasets.size(); i++)
    if(nbcols > datasets[i]->nbColumns())
      nbcols = datasets[i]->nbColumns();

  QList<Vector> vects;
  for(int i = 0; i < nbcols; i++)
    vects << Vector();
  QStringList names;
  QList<QStringList> rowNames;
  int idx = 0;
  for(int i = 0; i < datasets.size(); i++) {
    const DataSet * ds = datasets[i];
    names << ds->cleanedName();
    for(int j = 0; j < nbcols; j++)
      vects[j] << ds->column(j);

    // Now add row names, padding to empty strings when appropriate
    for(int j = 0; j < ds->rowNames.size(); j++) {
      if(rowNames.size() <= j)
        rowNames << QStringList();
      QStringList & lst = rowNames[j];
      while(lst.size() < idx)
        lst << QString();
      lst << ds->rowNames[j];
    }

    if(set && i > 0)
      segs << idx;
    for(int j = 0; j < ds->segments.size(); j++)
      segs << ds->segments[j] + idx;
    idx += ds->nbRows();
  }

  /// @question hmmm, what do we do HERE about meta-data ?
  DataSet * newDs = datasets.first()->derivedDataSet(vects, "");
  newDs->segments = segs;
  newDs->name = Utils::smartConcatenate(names, "+", "(", ")") + ".dat";
  newDs->rowNames = rowNames;
  return newDs;
}

void DataSet::removeRow(int index)
{
  for(int i = 0; i < columns.size(); i++)
    columns[i].remove(index);
  segments.shiftAbove(index);

  for(QStringList & lst : rowNames) {
    if(lst.size() > index)
      lst.takeAt(index);
  }
  
  invalidateCache();            // important.
}

Vector DataSet::segmentPositions() const
{
  Vector ret;
  for(int i = 0; i < segments.size(); i++) {
    int idx = segments[i];
    if(idx >= x().size())
      continue;                 // Avoid hard crashes here...
    double xv = x()[idx];
    if(idx > 0) {
      xv *= 0.5;
      xv += 0.5 * x()[idx-1];
    }
    ret << xv;
  }
  return ret;
}

QString DataSet::standardNameForColumn(int col) 
{
  if(col == 0)
    return "x";
  if(col == 1)
    return "y";
  return QString("y%1").arg(col);
}

QStringList DataSet::standardColumnNames() const
{
  QStringList ret;
  for(int i = 0; i < nbColumns(); i++)
    ret << standardNameForColumn(i);
  return ret;
}

void DataSet::setColumnName(int idx, const QString & name)
{
  if(columnNames.size() == 0)
    columnNames << QStringList();
  QStringList & nm = columnNames.first();
  if(nm.size() > nbColumns())   // brutally truncate
    nm = nm.mid(0, nbColumns());
  while(nm.size() < nbColumns())
    nm << standardNameForColumn(nm.size());
  if(idx < 0 || idx >= nm.size())
    return;
  nm[idx] = name;
}

void DataSet::setRowName(int idx, const QString & name)
{
  if(rowNames.size() == 0)
    rowNames << QStringList();
  QStringList & nm = rowNames.first();
  if(nm.size() > nbRows())   // brutally truncate
    nm = nm.mid(0, nbRows());
  while(nm.size() < nbRows())
    nm << "";
  if(idx < 0 || idx >= nm.size())
    return;
  nm[idx] = name;
}



bool DataSet::hasMetaData(const QString & name) const
{
  return metaData.contains(name);
}

void DataSet::setMetaData(const QString & name, const QVariant & val)
{
  metaData[name] = val;
}

void DataSet::clearMetaData(const QString & name)
{
  metaData.remove(name);
}

QVariant DataSet::getMetaData(const QString & val) const
{
  return metaData[val];
}


void DataSet::addMetaData(const ValueHash & val, bool override)
{
  ValueHash tmp(val);
  if(tmp.contains("__segments__")) {
    segments.clear();
    for(const QVariant & i : tmp["__segments__"].toList())
      segments.insert(i.toInt());
    tmp.remove("__segments__");
  }
  metaData.merge(tmp, override);
}

const Vector & DataSet::perpendicularCoordinates() const
{
  return perpCoords;
}

void DataSet::setPerpendicularCoordinates(const Vector & vect)
{
  if(vect.size() > 0 && vect.size() != nbColumns() - 1)
    throw RuntimeError("Wrong number of perpendicular coordinates: %1 for %2 Y column(s)").
      arg(vect.size()).arg(nbColumns() - 1);
  perpCoords = vect;
}

void DataSet::setPerpendicularCoordinates(double val)
{
  perpCoords = Vector(nbColumns() - 1, val);
}


bool DataSet::checkColNames() const
{
  for(const QStringList & lst : columnNames)
    if(lst.size() != nbColumns())
      return false;
  return true;
}

bool DataSet::checkRowNames() const
{
  for(const QStringList & lst : rowNames)
    if(lst.size() != nbRows())
      return false;
  return true;
}

QStringList DataSet::mainColumnNames(bool * madeup) const
{
  if(madeup)
    *madeup = false;
  if(checkColNames() && columnNames.size() > 0)
    return columnNames[0];
  if(madeup)
    *madeup = true;
  return standardColumnNames();
}

QStringList DataSet::mainRowNames() const
{
  if(checkRowNames() && rowNames.size() > 0)
    return rowNames[0];
  return QStringList();
}

QString DataSet::perpendicularCoordinatesName(bool * madeup) const
{
  if(madeup)
    *madeup = false;
  if(perpCoordNames.size() > 0)
    return perpCoordNames[0];
  if(madeup)
    *madeup = true;
  return "Z";
}



DataSet * DataSet::transpose() const
{
  // First, check sanity of perpendicular coordinate, or create it as
  // index
  Vector pc = perpCoords;
  if(pc.size() != nbColumns() - 1) {
    pc.clear();
    for(int i = 0; i < nbColumns() - 1; i++)
      pc << i;
  }

  QList<Vector> cols;
  cols << pc;
  for(int i = 0; i < nbRows(); i++)
    cols << Vector(pc.size(), 0);

  for(int i = 0; i < nbColumns() - 1; i++) {
    for(int j = 0; j < nbRows(); j++) 
      cols[j+1][i] = columns[i+1][j]; 
  }

  DataSet * ds = derivedDataSet(cols, "_transposed.dat");
  ds->perpCoords = x();
  ds->columnNames = rowNames;
  for(int i = 0; i < rowNames.size(); i++) {
    QString n = perpCoordNames.value(i, i ? QString() : QString("Z"));
    ds->columnNames[i].insert(0, n);
  }
  ds->rowNames = columnNames;
  for(int i = 0; i < ds->rowNames.size(); i++) {
    if(ds->rowNames[i].size() > 0)
      ds->perpCoordNames << ds->rowNames[i].takeFirst();
    else
      break;
  }

  return ds;
}


bool DataSet::flagged() const
{
  return flags.size() > 0;
}

bool DataSet::flagged(const QString & str) const
{
  return flags.contains(str);
}

void DataSet::setFlag(const QString & str)
{
  flags.insert(str);
}

void DataSet::setFlags(const QStringList & lst)
{
  setFlags(QSet<QString>::fromList(lst));
}

void DataSet::setFlags(const QSet<QString> & s)
{
  flags.unite(s);
}

void DataSet::clearFlag(const QString & str)
{
  flags.remove(str);
}

void DataSet::clearFlags()
{
  flags.clear();
}

void DataSet::clearFlags(const QStringList & lst)
{
  for(int i = 0; i < lst.size(); i++)
    clearFlag(lst[i]);
}

QSet<QString> DataSet::allFlags() const
{
  return flags;
}

ValueHash DataSet::getMetaData() const
{
  ValueHash rv = metaData;
  rv["name"] = name;
  if(perpCoords.size() == 1)
    rv["Z"] = perpCoords[0];
  else if(perpCoords.size() > 1) {
    QList<QVariant> l;
    for(double d : perpCoords)
      l << d;
    rv["Zs"] = l;
  }
  return rv;
}


mrb_value DataSet::evaluateWithMeta(const QString & expression, bool useStats) const
{
  DataSetExpression ex(this, useStats, true, true);
  return ex.evaluate(expression);
}

mrb_value DataSet::evaluateWithMeta(const QString & expression, bool useStats,  bool modifyMeta) 
{
  DataSetExpression ex(this, useStats, true, true);
  mrb_value v = ex.evaluate(expression);
  MRuby * mr = MRuby::ruby();
  if(modifyMeta) {
    metaData.setFromRuby(mr->getGlobal("$meta"));
    // We also get the column names:
    mrb_value n = mr->getGlobal("$col_names");
    if(mr->isArray(n) && mr->arrayLength(n) == nbColumns()) {
      for(int i = 0; i < nbColumns(); i++)
        setColumnName(i, mr->toQString(mr->arrayRef(n, i)));
    }
    n = mr->getGlobal("$row_names");
    if(mr->isArray(n) && mr->arrayLength(n) == nbRows()) {
      for(int i = 0; i < nbRows(); i++)
        setRowName(i, mr->toQString(mr->arrayRef(n, i)));
    }
  }
  return v;
}

bool DataSet::matches(const QString & expression) const
{
  mrb_value v = evaluateWithMeta(expression, true);
  return mrb_test(v);
}


// looks dirty, but should be OK:
static uint qHash(const QVector<int> & v)
{
  uint rv = qHash(v.size());
  for(int i = 0; i < v.size(); i++)
    rv ^= qHash(v[i]);
  return rv;
}

static bool cmpVectors(const QVector<int> & a, const QVector<int> & b)
{
  if(a.size() != b.size())
    throw InternalError("Comparing vectors of different sizes: %1 vs %2").
      arg(a.size()).arg(b.size());
  for(int i = 0; i < a.size(); i++) {
    if(a[i] < b[i])
      return true;
    if(a[i] > b[i])
      return false;
  }
  return false;
}

QList<DataSet *> DataSet::autoSplit(const QHash<int, QString> & cols,
                                    double tolerance) const
{
  QList<DataSet *> d;
  QHash<int, Vector> uniqueValues;
  for(auto i = cols.begin(); i != cols.end(); i++) {
    if(i.key() >= columns.size())
      throw RuntimeError("Dataset does not have %1 columns").arg(i.value()+1);
    const Vector & v = columns[i.key()];
    uniqueValues[i.key()] = v.values(tolerance);
  }

  QHash<QVector<int>, DataSet *> rvs;

  QVector<int> cls = uniqueValues.keys().toVector();
  std::sort(cls.begin(), cls.end());

  // A template for columns
  QList<Vector> tmplt;
  for(int i = 0; i < columns.size() - cols.size(); i++)
    tmplt << Vector();

  int sz = x().size();
  int nbc = cls.size(); 
  QVector<int> idx = cls;       // just for initialization

  
  for(int i = 0; i < sz; i++) {
    for(int j = 0; j < nbc; j++) {
      int cur_col = cls[j];
      double val = columns[cur_col][i];
      int uidx = uniqueValues[cur_col].bfind(val);
      if(uidx > 0 && Utils::fuzzyCompare(val, uniqueValues[cur_col][uidx-1],
                                         tolerance))
        uidx --;
      idx[j] = uidx;
    }

    DataSet * ds = rvs.value(idx, NULL);
    if(! ds) {
      ds = derivedDataSet(tmplt, ".tmp");
      ds->segments.clear();

      // set the meta-data
      for(auto k = cols.begin(); k != cols.end(); k++)
        // We use the original data as base
        ds->setMetaData(k.value(), columns[k.key()][i]);
      rvs[idx] = ds;
      ds->rowNames = rowNames;

      for(QStringList & rn : ds->rowNames)
        rn.clear();
    }
    int tgi = 0;
    for(int j = 0; j < columns.size(); j++) {
      if(uniqueValues.contains(j))
        continue;
      ds->columns[tgi] << columns[j][i];
      tgi += 1;
    }
    for(int rni = 0; rni < rowNames.size(); rni++) {
      if(rowNames[rni].size() > i)
        ds->rowNames[rni] << rowNames[rni][i];
    }
  }

  QList<QVector<int> > keys = rvs.keys();
  std::sort(keys.begin(), keys.end(), &cmpVectors);

  for(int i = 0; i < keys.size(); i++) {
    DataSet * ds = rvs[keys[i]];
    ds->name = ds->cleanedName() + QString("_subset_%1.dat").arg(i);
    // Remove the column names of the columns that were removed
    ds->columnNames = columnNames;
    for(int i = cls.size()-1; i >= 0; i--) {
      for(QStringList & lst : ds->columnNames) {
        int idx = cls[i];
        if(idx < lst.size())
          lst.takeAt(idx);
      }
    }

    d << ds;
  }
  return d;
}

void DataSet::expandMeta(const QStringList & meta,
                         const QList<DataSet*> & datasets,
                         bool strict) const
{
  for(const QString & n : meta) {
    if(! hasMetaData(n)) {
      if(strict)
        throw RuntimeError("No such meta: '%1'").arg(n);
      else
        continue;
    }
    QVariant var = getMetaData(n);
    QList<QVariant> lst = var.toList();
    if(lst.size() != datasets.size()) {
      if(strict)
        throw RuntimeError("Not the right number of values for meta '%1': %2 vs %3").
          arg(n).arg(lst.size()).arg(datasets.size());
      else
        continue;
    }
    for(int i = 0; i < datasets.size(); i++)
      datasets[i]->setMetaData(n, lst[i]);
  }
}

void DataSet::contractMeta(const QStringList & meta,
                           const QList<const DataSet*> & datasets,
                           bool strict)
{
  for(const QString & n : meta) {
    QList<QVariant> vals;
    for(const DataSet * ds : datasets) {
      if(! ds->hasMetaData(n)) {
        if(strict)
          throw RuntimeError("Dataset: '%1' has no meta '%2'").
            arg(ds->name).arg(n);
        else
          continue;
      }
      vals << ds->getMetaData(n);
    }
    metaData[n] = vals;
  }
}



double DataSet::metaScanRate(bool * ok) const
{
  if(! metaData.contains("sr")) {
    if(ok)
      *ok = false;
    return 0.1;                 // Why not ?
  }
  if(ok)
    *ok = true;
  return getMetaData("sr").toDouble();
}


// OK, this maybe isn't that great, let's forget that.
/*
Vector DataSet::convolveWith(const DataSet * kernel) const
{
  Vector rv = y();

  int nbr = nbRows();
  int kbr = kernel->nbRows();

  double dmin = kernel->x().first();
  if(dmin > 0)
    throw RuntimeError("Trying to convolve with a kernel whose min x value > 0");
  // double xmin = x().first();
  double dmax = kernel->x().last();
  if(dmax < 0)
    throw RuntimeError("Trying to convolve with a kernel whose max x value < 0");


  // Using directly the pointers to improve a bit the performance
  double * target = rv.data();

  const double * ds_x = x().data();
  const double * ds_y = y().data();

  const double * ke_x = kernel->x().data();
  const double * ke_y = kernel->y().data();

  int j_prev = 0;

  // We deal with each element one by one
  // QTextStream o(stdout);
  // o << "Convolving: " << nbr << "/" << kbr << endl;
  for(int i = 0; i < nbr; i++) {
    double xv = ds_x[i];
    // o << "Point #" << i << ": x = " << xv << endl;
    // first look for the boundaries within this dataset

    int j = j_prev;
    // int j = 0;
    while(j < nbr && ds_x[j] < xv - dmax)
        j++;
    j_prev = j;
    // o << " -> starting at " << j << ": x = " << x()[j] << endl;
    //j should be <= i
    if(j > i)
      throw RuntimeError("Confusion in convolve, are the datasets sorted ?");

    double sum = 0;
    double prev = 0;
    double cur = 0;
    int j0 = j;

    int k_prev = kbr-2;

    while(j < nbr && ds_x[j] < xv - dmin) {
      // o << "\t dealing with " << j << "/" << nbr << endl;
      // o << "\t -> x value is " << x()[j] << endl;
      // First find the position in the kernel
      double dx = xv - ds_x[j];
      int k = k_prev;
      while(ke_x[k] > dx && k > 0)
        k --;
      k_prev = k;
      // o << "\t -> found kernel position #" << k << " for x = "
      //   << kernel->x()[k] << endl;
      // In principle we should have found
      double alpha = (dx - ke_x[k])/
        (ke_x[k+1] - ke_x[k]);
      double ky = alpha * ke_y[k+1] + (1 - alpha) * ke_y[k];
      cur = ds_y[j]*ky;
      if(j > j0)
        sum += 0.5 * (cur + prev) * (ds_x[j] - ds_x[j-1]);
      prev = cur;
      j++;
    }
    // o << "-> got value for" << i << ": " << sum <<  endl;
    target[i] = sum;
  }
  
  return rv;
}
*/


//////////////////////////////////////////////////////////////////////

QDataStream & operator<<(QDataStream & out, const DataSet & ds)
{
  qint32 nbCols = ds.columns.size();
  out << nbCols;
  for(qint32 i = 0; i < nbCols; i++)
    out << ds.columns[i];

  out << ds.name;
  out << ds.segments;
  out << ds.metaData;

  // From version 1 onwards
  // out << ds.flagged;

  // From version 2
  out << ds.perpCoords;

  // From version 4
  out << ds.flags;

  // From version 5
  out << ds.options;

  // From version 6
  out << ds.columnNames << ds.rowNames << ds.perpCoordNames;

  return out;
}


QDataStream & operator>>(QDataStream & in, DataSet & ds)
{
  qint32 nbCols;
  in >> nbCols;
  ds.columns.clear();
  for(qint32 i = 0; i < nbCols; i++) {
    Vector v;
    in >> v;
    ds.columns.append(v);
  }

  in >> ds.name;

  in >> ds.segments;

  in >> ds.metaData;

  bool flagged = false;
  if(DataStack::serializationVersion >= 1 &&
     DataStack::serializationVersion < 4) {
    in >> flagged;
    ds.flags.clear();
    if(flagged)
      ds.flags.insert("default");
  }

  if(DataStack::serializationVersion >= 2)
    in >> ds.perpCoords;

  if(DataStack::serializationVersion >= 4) 
    in >> ds.flags;

  if(DataStack::serializationVersion >= 5) 
    in >> ds.options;

  if(DataStack::serializationVersion >= 6) 
    in >> ds.columnNames >> ds.rowNames >> ds.perpCoordNames;



  return in;
}
