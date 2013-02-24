/*
  vector.cc: implementation of the Vector class
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
#include <vector.hh>

#include <exceptions.hh>

QList<Vector> Vector::readFromStream(QIODevice * source,
                                     const QRegExp & separatorREt,
                                     const QRegExp & commentREt,
                                     QStringList * comments)
{
  QList<Vector> retVal;
  int lineNumber = 0;
  QRegExp separatorRE(separatorREt);
  QRegExp commentRE(commentREt);
  QRegExp blankLineRE("^\\s*$"); /// @todo halt on blank lines

  QLocale locale = QLocale::c(); /// @todo offer the possibility to customize.

  int numberRead = 0;
  while(! source->atEnd()) {
    lineNumber++;
    QString line = source->readLine();
    if(commentRE.indexIn(line) >= 0) {
      if(comments)
        *comments << line;
      continue;
    }
    if(blankLineRE.indexIn(line) == 0) {
      continue;
    }
    /// @todo A manual split would be much much faster (no memory
    /// allocation). I think DVector::fast_fancy_read greatly
    /// outperforms this, but well...
    QStringList elements = line.trimmed().split(separatorRE);
    /// @todo customize trimming.
    while(retVal.size() < elements.size()) {
      retVal << Vector(numberRead, 0.0/0.0);
      retVal.last().reserve(10000); // Most files won't be as large
    }
    int nbNans = 0;
    for(int i = 0; i < retVal.size(); i++) {
      bool ok = false;
      double value = locale.toDouble(elements.value(i, ""), &ok);
      if(! ok)
        value = 0.0/0.0; /// @todo customize
      if(value != value)
        nbNans++;
      retVal[i] << value;
    }
    // We remove lines fully made of NaNs
    if(nbNans == retVal.size()) {
      for(int i = 0; i < retVal.size(); i++)
        retVal[i].resize(retVal[i].size() - 1);
    }
    else
      numberRead++;
  }
  // Trim the values in order to save memory a bit (at the cost of
  // quite a bit of reallocation/copying time)
  for(int i = 0; i < retVal.size(); i++)
    retVal[i].squeeze();

  return retVal;
}

QList<Vector> Vector::readFromStream(QIODevice * source,
                                     const QString & separatorREt,
                                     const QString & commentREt,
                                     QStringList * comments)
{
  return readFromStream(source, QRegExp(separatorREt),
                        QRegExp(commentREt), comments);
}



double Vector::min() const
{
  int sz = size();
  if(! sz)
    return 0.0/0.0;
  const double * d = data();
  double m = d[0];
  for(int i = 1; i < sz; i++)
    if(d[i] < m)
      m = d[i];
  return m;
}

double Vector::magnitude() const
{
  int sz = size();
  if(! sz)
    return 0.0/0.0;
  const double * d = data();
  double m = d[0];
  for(int i = 1; i < sz; i++)
    if(fabs(d[i]) > fabs(m))
      m = d[i];
  return m;
}

double Vector::max() const
{
  int sz = size();
  if(! sz)
    return 0.0/0.0;
  const double * d = data();
  double m = d[0];
  for(int i = 1; i < sz; i++)
    if(d[i] > m)
      m = d[i];
  return m;
}


// Note: these functions may seem very inefficient, but in fact, with
// optimization turned on, g++ generates the same code as the loops
// one would directly write.
static inline Vector & bang_operate(Vector & a, double b, 
                                    double (*op)(double, double))
{
  int sz = a.size();
  double * aval = a.data();
  for(int i = 0; i < sz; i++, aval++)
    *aval = op(*aval, b);
  return a;
}

static inline Vector dup_operate(const Vector &a, double b, 
                                 double (*op)(double, double))
{
  int sz = a.size();
  const double * aval = a.data();
  Vector c;
  c.reserve(sz);
  for(int i = 0; i < sz; i++, aval++)
    c << op(*aval, b);
  return c;
}

static inline Vector & bang_operate(Vector & a, const Vector & b, 
                                    double (*op)(double, double))
{
  int sz = a.size();
  if(sz != b.size())
    throw InternalError("Size mismatch in vector operation");

  double * aval = a.data();
  const double * bval = b.data();
  for(int i = 0; i < sz; i++, aval++, bval++)
    *aval = op(*aval, *bval);
  return a;
}

static inline Vector dup_operate(const Vector &a, const Vector & b, 
                                 double (*op)(double, double))
{
  int sz = a.size();
  if(sz != b.size())
    throw InternalError("Size mismatch in vector operation");

  const double * aval = a.data();
  const double * bval = b.data();
  Vector c;
  c.reserve(sz);
  for(int i = 0; i < sz; i++, aval++, bval++)
    c << op(*aval, *bval);
  return c;
}

static double add(double a, double b)
{
  return a + b;
}

Vector & Vector::operator+=(const Vector & a)
{
  return bang_operate(*this, a, add);
}

Vector & Vector::operator+=(double a)
{
  return bang_operate(*this, a, add);
}

Vector Vector::operator+(const Vector & a) const
{
  return dup_operate(*this, a, add);
}

Vector Vector::operator+(double a) const
{
  return dup_operate(*this, a, add);
}



static double mul(double a, double b)
{
  return a * b;
}

Vector & Vector::operator*=(const Vector & a)
{
  return bang_operate(*this, a, mul);
}

Vector & Vector::operator*=(double a)
{
  return bang_operate(*this, a, mul);
}

Vector Vector::operator*(const Vector & a) const
{
  return dup_operate(*this, a, mul);
}

Vector Vector::operator*(double a) const
{
  return dup_operate(*this, a, mul);
}

static double sub(double a, double b)
{
  return a - b;
}

Vector & Vector::operator-=(const Vector & a)
{
  return bang_operate(*this, a, sub);
}

Vector & Vector::operator-=(double a)
{
  return bang_operate(*this, a, sub);
}

Vector Vector::operator-(const Vector & a) const
{
  return dup_operate(*this, a, sub);
}

Vector Vector::operator-(double a) const
{
  return dup_operate(*this, a, sub);
}

static double div(double a, double b)
{
  return a / b;
}

Vector & Vector::operator/=(const Vector & a)
{
  return bang_operate(*this, a, div);
}

Vector & Vector::operator/=(double a)
{
  return bang_operate(*this, a, div);
}

Vector Vector::operator/(const Vector & a) const
{
  return dup_operate(*this, a, div);
}

Vector Vector::operator/(double a) const
{
  return dup_operate(*this, a, div);
}

gsl_vector_view Vector::vectorView()
{
  return gsl_vector_view_array(data(),size());
}

gsl_vector_const_view Vector::vectorView() const
{
  return gsl_vector_const_view_array(data(),size());
}

Vector Vector::fromGSLVector(const gsl_vector * vect)
{
  Vector ret;
  ret.reserve(vect->size);
  for(int i = 0; i < vect->size; i++)
    ret << gsl_vector_get(vect, i);
  return ret;
}

Vector Vector::deltas() const
{
  Vector ret;
  ret.reserve(size()-1);
  for(int i = 0; i < size() - 1; i++)
    ret << value(i+1) - value(i);
  return ret;
}

void Vector::stats(double * average, double * variance) const
{
  *average = 0;
  *variance = 0;
  int sz = size();
  for(int i = 0; i < sz; i++) {
    double val = value(i);
    *average += val;
    *variance += val*val;
  }
  *average /= sz;
  *variance /= sz;
  *variance -= (*average * *average);
}

Vector Vector::removeSpikes(int nb, double extra, int *nbFound) const
{
  Vector v = deltas();
  Vector nv = (*this);
  double davg, dvar;
  v.stats(&davg, &dvar);

  int found = 0;
  
  // We really use standard deviation and not variance !
  dvar = sqrt(dvar);

  for(int i = nb-1; i < size() - nb - 2; i++) {
    for(int j = 1; j <= nb; j++) {
      double dvlim = j * (fabs(davg) + extra * dvar);
      if((fabs(value(i + j) - value(i)) >= dvlim)) {
        if((fabs(value(i + j + 1) - value(i)) <= dvlim)) {

          // We found a spike of j points: replace by the weighted
          // average of bounds.
          // printf("Found spike at index %d/%d\n", i, j);
          for(int k = 1; k <= j; k++)
            nv[i+k] = (j - k)/(1.0*j) * value(i) + 
              k/(1.0*j) * value(i + j + 1);
          i += j;
          found++;
          break;                  // exit the inner loop !
        }
      }
      else                      // There is no peak here, skip.
                                // (should only happen for j == 1)
        break;                  
    }
  }
  if(nbFound)
    *nbFound = found;
  return nv;
}

double Vector::norm() const
{
  gsl_vector_const_view v = 
    gsl_vector_const_view_array(data(), size());
  return gsl_blas_dnrm2(&v.vector);
}

QList<int> Vector::extrema(int window) const
{
  /// @todo Shall I add threshold mechanisms here ?
  QList<int> ret;
  int sz = size();
  const double * v = data();

  for(int i = 0; i < sz; i++) {
    int first = std::max(i - window, 0);
    int last = std::min(i + window, sz - 1);
    double min = v[first];
    double max = v[first];
    int where_min = first;
    int where_max = first;

    for(int j = first + 1; j <= last; j++) {
      if(v[j] > max) {
        where_max = j;
        max = v[j];
      }
      else if(v[j] < min) {
        where_min = j;
        min = v[j];
      }
    }
    if(where_min == i)
      ret << -(i + 1);
    else if(where_max == i)
      ret << (i + 1);
  }

  return ret;
  
}

int Vector::closestPoint(double v) const
{
  const double * d = data();
  if(size() <= 0)
    throw RuntimeError("Cannot find a point in an empty vector");
  double delta = fabs(v - d[0]);
  int idx = 0;
  for(int i = 1; i < size(); i++) {
    double nd = fabs(v - d[i]);
    if(nd < delta) {
      delta = nd;
      idx = i;
    }
  }
  // We could also return delta, but, well...
  return idx;
}

Vector Vector::uniformlySpaced(double min, double max, int nb)
{
  Vector r(nb, 0);
  if(nb < 2)
    throw RuntimeError("Cannot create a segment-spanning vector of "
                       "less than 2 points !");
  for(int i = 0; i < nb; i++)
    r[i] = min + (max - min) * i/(nb - 1);
  return r;
}

Vector Vector::resample(int nb) const
{
  if(nb < 2)
    nb = size();
  if(nb == 1)
    return *this;               // Well, it's still correct, isn't it ?
  return uniformlySpaced(min(), max(), nb);
}


/// @todo Use a more time-efficient algorithm ? See
/// http://stackoverflow.com/questions/2579912/how-do-i-find-the-median-of-numbers-in-linear-time-using-heaps
double Vector::median() const
{
  if(! size())
    throw RuntimeError("Need at least one element !");
  Vector v = *this;
  qSort(v);
  return v[v.size()/2];
}
