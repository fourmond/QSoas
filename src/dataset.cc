/*
  dataset.cc: implementation of the DataSet class
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

#include <headers.hh>
#include <dataset.hh>

#include <math.h>
#include <utils.hh>

#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>

#include <gsl/gsl_blas.h>

#include <exceptions.hh>

#include <terminal.hh>

void DataSet::dump() const
{
  QTextStream o(stdout);
  for(int i = 0; i < x().size(); i++)
    for(int j = 0; j < columns.size(); j++)
      o << columns[j][i] << (j == columns.size() - 1 ? "\n" : "\t");      
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

QString DataSet::stringDescription() const
{
  return QObject::tr("'%1': %2 columns, %3 rows, %4 bytes").
    arg(name).arg(nbColumns()).arg(nbRows()).arg(byteSize());
}

void DataSet::regenerateCache() const
{
  if(isCacheValid())
    return;

  int size = columns.size();
  cache.minima.resize(size);
  cache.maxima.resize(size);

  for(int i = 0; i < size; i++) {
    cache.minima[i] = columns[i].min();
    cache.maxima[i] = columns[i].max();
  }
  
  cache.valid = true;
}

QRectF DataSet::boundingBox() const
{
  updateCache();
  QRectF r;
  if(columns.size() >= 2)
    r.setCoords(cache.minima[0], cache.minima[1], 
                cache.maxima[0], cache.maxima[1]);
  return r;
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
  if(idx > 0)
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
    *first = new DataSet(newcols);
    (*first)->name = cleanedName() + "_a.dat";
  }
  if(second) {
    if(idx >= nbRows())         // Ensure we don't go over.
      idx = nbRows() - 1;
    QList<Vector> newcols;
    for(int i = 0; i < columns.size(); i++)
      newcols << Vector(columns[i].mid(idx));
    *second = new DataSet(newcols);
    (*second)->name = cleanedName() + "_b.dat";
  }
}


QList<DataSet *> DataSet::chop(const QList<double> & lengths, 
                               bool isLength) const
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
        retvals.append(subset(lastidx, i-1));
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
        retvals.append(subset(lastidx, i-1));
        lastidx = i;
        curIdx++;
        if(curIdx >= lengths.size())
          break;
      }
    }
  }
  if(lastidx < size - 1)
    retvals.append(subset(lastidx, size-1));
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


DataSet * DataSet::applyBinaryOperation(const DataSet * a,
                                        const DataSet * b,
                                        double (*op)(double, double),
                                        const QString & cat, 
                                        bool naive)
{
  // only deal with the common columns
  int nbcols = std::min(a->nbColumns(), b->nbColumns());
  if(nbcols < 2)
    throw RuntimeError("Need at least a Y column for both datasets");

  int size_a = a->nbRows();
  const double * xa = a->columns[0].data();

  int size_b = b->nbRows();
  const double * xb = b->columns[0].data();

  QList<Vector> vects; 
  for(int i = 0; i < nbcols; i++)
    vects << Vector();
  if(naive) {
    for(int i = 0; i < size_a; i++) {
      vects[0] << xa[i];
      int si = i;
      if(i >= size_b)
        si = size_b - 1;        // Pad with last value
      for(int k = 1; k < nbcols; k++)
        vects[k] << op(a->columns[k][i], b->columns[k][si]);
    }
  }
  else {
    int j = 0;
    for(int i = 0; i < size_a; i++) {
      /* We first look for the closest point */
      double diff = fabs(xa[i] - xb[j]);
      while((j < (size_b - 1)) && (fabs(xa[i] - xb[j+1]) <  diff))
        diff = fabs(xa[i] - xb[++j]);
      // ys[i] -= yo[j];
      vects[0] << xa[i];        // a is the master dataset
      for(int k = 1; k < nbcols; k++)
        vects[k] << op(a->columns[k][i], b->columns[k][j]);
    }
  }

  DataSet * ds = new DataSet(vects);
  ds->name = a->cleanedName() + cat + b->cleanedName() + ".dat";
  return ds;
}

static inline double sub(double a, double b)
{
  return a - b;
}

DataSet * DataSet::subtract(const DataSet * ds, bool naive) const
{
  return applyBinaryOperation(this, ds, sub, "-", naive);
}

static inline double div(double a, double b)
{
  return a/b;
}

DataSet * DataSet::divide(const DataSet * ds, bool naive) const
{
  return applyBinaryOperation(this, ds, div, "_div_", naive);
}

static inline double keep_second(double, double b)
{
  return b;
}

DataSet * DataSet::merge(const DataSet * ds, bool naive) const
{
  DataSet * nd = applyBinaryOperation(this, ds, keep_second, 
                                      "_merged_", naive);
  nd->columns << nd->columns[0];
  nd->columns[0] = columns[1];
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

QPair<double, double> DataSet::reglin(int begin, int end) const
{
  int nb = nbRows();
  if(end < 0 || end > nb)
    end = nb;
  const double * x = columns[0].data();
  const double * y = columns[1].data();
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


DataSet * DataSet::subset(int beg, int end, bool within) const
{
  QList<Vector> cols;
  int nb = nbRows();
  // Sanity checking for indices
  if(beg < 0)
    beg = 0;
  if(beg >= nb)
    beg = nb - 1;
  if(end < beg)
    end = beg;
  if(end >= nb)
    end = nb-1;
    

  if(within) {
    for(int i = 0; i < columns.size(); i++)
      cols << columns[i].mid(beg, 1+end-beg);
  }
  else {
    for(int i = 0; i < columns.size(); i++) {
      Vector v = columns[i].mid(0, beg);
      v << columns[i].mid(end);
      cols << v;
    }
  }
  DataSet * newds = new DataSet(cols);
  newds->name = cleanedName() + QString("_%1from_%2_to_%3.dat").
    arg(within ? "" : "excl_").
    arg(columns[0][beg]).arg(columns[0][end]);
  return newds;
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

  DataSet * newds = new DataSet(cols);
  newds->name = cleanedName() + "_spikes.dat";
  return newds;
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
double smooth_pick(const double *x, const double *y, 
		   int nb, int idx, int range)
{
  int left, right,i,nb_same_sign;
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

double DataSet::yValueAt(double x) const
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
      /// @todo we may want to use linear interpolation here.
      if(fabs(dx) < fabs(lastdx))
        return yvals[i];
      else
        return yvals[i-1];
    }
    lastdx = dx;
  }
  return yvals[nb-1];
}

void DataSet::write(QIODevice * target) const
{
  QTextStream o(target);
  
  /// @todo Write header and meta-information when applicable.

  int nb = nbRows();
  for(int i = 0; i < nb; i++) {
    for(int j = 0; j < columns.size(); j++) {
      if(j)
        o << "\t";
      o << columns[j][i];
    }
    o << "\n";
  }
  o << flush;
}

void DataSet::write(const QString & n) const
{
  QString fileName = n;
  if(fileName.isEmpty())
    fileName = cleanedName() + ".dat";
  
  QFile file(fileName);
  Utils::open(&file, QIODevice::WriteOnly);

  QTextStream o(&file);
  o << "# saved from Soas buffer name " << name << endl;

  write(&file);
}

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

DataSet * DataSet::sort(bool reverse) const
{
  /// @todo This algorithm isn't fast nor space-efficient, but it does
  /// work, and its simple.

  QList< QPair<double, int> > vals;

  const double *xv = x().data();
  int size = x().size();
  vals.reserve(size);
  for(int i = 0; i < size; i++)
    vals << QPair<double, int>(xv[i], i);
  
  if(reverse)
    qSort(vals.begin(), vals.end(), &greaterThan);
  else
    qSort(vals.begin(), vals.end(), &lessThan);


  QList<Vector> nv;
  for(int i = 0; i < columns.size(); i++)
    nv << Vector(size, 0);
  for(int i = 0; i < size; i++) {
    for(int j = 0; j < columns.size(); j++)
      nv[j][i] = columns[j][(vals[i].second)];
  }

  DataSet * ds = new DataSet(nv);
  ds->name = cleanedName() + "_sorted.dat";
  return ds;
}

DataSet * DataSet::derivedDataSet(const Vector &newy, 
                                  const QString & suffix) const
{
  QList<Vector> nv;
  nv << x();
  nv << newy;

  /// @todo Shall we put the other columns back in ?
  DataSet * ds = new DataSet(nv);
  ds->name = cleanedName() + suffix;

  /// @todo Should we update the date too ?
  return ds;
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

DataSet * DataSet::concatenateDataSets(QList<const DataSet *> datasets)
{
  int nbcols = datasets.first()->nbColumns();
  for(int i = 1; i < datasets.size(); i++)
    if(nbcols > datasets[i]->nbColumns())
      nbcols = datasets[i]->nbColumns();

  QList<Vector> vects;
  for(int i = 0; i < nbcols; i++)
    vects << Vector();
  QStringList names;
  for(int i = 0; i < datasets.size(); i++) {
    const DataSet * ds = datasets[i];
    names << ds->cleanedName();
    for(int i = 0; i < nbcols; i++)
      vects[i] << ds->column(i);
  }
  DataSet * newDs = new DataSet(vects);
  newDs->name = names.join("_") + ".dat";
  return newDs;
}

void DataSet::removePoint(int index)
{
  for(int i = 0; i < columns.size(); i++)
    columns[i].remove(index);
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


//////////////////////////////////////////////////////////////////////

QDataStream & operator<<(QDataStream & out, const DataSet & ds)
{
  qint32 nbCols = ds.columns.size();
  out << nbCols;
  for(qint32 i = 0; i < nbCols; i++)
    out << ds.columns[i];

  out << ds.name;
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
  return in;
}
