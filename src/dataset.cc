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


/// @todo Include peak detection, with the algorithm used for the
/// film_decay command in the old Soas.

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


QList<DataSet *> DataSet::chop(const QList<double> & lengths) const
{
  QList<DataSet *> retvals;
  int lastidx = 0;
  double tdx = 0;
  int size;
  int curl = 0;
  const double *x = getValues(0, &size);
  for(int i = 1; i < size; i++) {
    double dx = fabs(x[i] - x[i-1]);
    tdx += dx;
    if(tdx >= lengths[curl]) {
      retvals.append(subset(lastidx, i-1));
      lastidx = i;
      tdx = dx; // We restart as if the previous split had finished
      curl++;
      if(curl >= lengths.size())
        break;
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
    if(size_a > size_b)
      throw RuntimeError(QString("Not enough data points in %1 to match those "
                                 "of %2").arg(b->name).arg(a->name));
    for(int i = 0; i < size_a; i++) {
      vects[0] << xa[i];
      for(int k = 1; k < nbcols; k++)
        vects[k] << op(a->columns[k][i], b->columns[k][i]);
    }
  }
  else {
    for(int i = 0; i < nbcols; i++)
      vects << Vector();

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

Vector DataSet::bSplinesSmooth(int order, const Vector & xvalues, 
                               double * residuals, Vector * derivative) const
{
  Vector xv = xvalues;
  qSort(xv);
  if(xv.min() > columns[0].min())
    xv.prepend(columns[0].min());
  if(xv.max() < columns[0].max())
    xv.append(columns[0].max());
  
  
  gsl_bspline_workspace * bw = gsl_bspline_alloc(order, xv.size());
  int nbCoeffs = xv.size() + order - 2;

  gsl_vector_view v = gsl_vector_view_array(xv.data(), xv.size());
  gsl_bspline_knots(&v.vector, bw);

  int nb = nbRows();
  gsl_matrix * X = gsl_matrix_alloc(nb, nbCoeffs);
  const double * xvals = columns[0].data();

  for(int i = 0; i < nb; i++) {
    gsl_vector_view v = gsl_matrix_row(X, i);
    gsl_bspline_eval(xvals[i], &v.vector, bw);
  }

  gsl_multifit_linear_workspace * mfit = 
    gsl_multifit_linear_alloc(nb, nbCoeffs);
  
  gsl_vector * coeffs = gsl_vector_alloc(nbCoeffs);
  gsl_matrix * cov = gsl_matrix_alloc(nbCoeffs, nbCoeffs);
  double chisq;
  gsl_vector_const_view yv = 
    gsl_vector_const_view_array(columns[1].data(), nb);
  
  gsl_multifit_linear(X, &yv.vector,
                      coeffs, cov, &chisq, mfit);
  if(residuals)
    *residuals = chisq;


  // Now computing the target values
  Vector ret(nb, 0);
  gsl_vector_view target = gsl_vector_view_array(ret.data(), nb);
  
  // Computing the final Y values:
  gsl_blas_dgemv(CblasNoTrans, 1, X, coeffs, 0, &target.vector);

  if(derivative) {
    gsl_bspline_deriv_workspace * dw = gsl_bspline_deriv_alloc(order);
    gsl_matrix * mtx = gsl_matrix_alloc(nbCoeffs, 2);
    for(int i = 0; i < nb; i++) {
      gsl_vector_view v = gsl_matrix_row(X, i);
      gsl_bspline_deriv_eval(xvals[i], 1, mtx, bw, dw);
      gsl_vector_view s = gsl_matrix_column(mtx, 1);
      gsl_vector_memcpy(&v.vector, &s.vector);
    }
    gsl_bspline_deriv_free(dw);
    gsl_matrix_free(mtx);

    derivative->resize(nb);
    gsl_vector_view target = gsl_vector_view_array(derivative->data(), nb);
    // Computing the final Y values:
    gsl_blas_dgemv(CblasNoTrans, 1, X, coeffs, 0, &target.vector);
  }



  // Freeing what isn't necessary anymore:
  gsl_bspline_free(bw);
  gsl_vector_free(coeffs);
  gsl_matrix_free(X);
  gsl_matrix_free(cov);
  gsl_multifit_linear_free(mfit);

  return ret;
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
