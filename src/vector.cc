/*
  vector.cc: implementation of the Vector class
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014 by CNRS/AMU

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
#include <utils.hh>

#include <gsl/gsl_histogram.h>

#include <linereader.hh>


Vector::Vector(const double * values, int nb)
{
  resize(nb);
  double * d = data();
  for(int i = 0; i < nb; i++)
    d[i] = values[i];
}


QList<QList<Vector> > Vector::readFromStream(QTextStream * source,
                                             std::function<void (const QString &, QVector<int> *)> splitter,
                                             const QRegExp & commentREt,
                                             bool splitOnBlank,
                                             const QString & decimalSep,
                                             const QRegExp & blankREt,
                                             QStringList * comments,
                                             int skip,
                                             QList<int> textColumns,
                                             QList<QList<QStringList> > * savedTexts)
{

  QList<QList<Vector> > retval;
  retval << QList<Vector>();
  if(savedTexts)
    *savedTexts << QList<QStringList>();

  QList<Vector> * curCols = &retval.first();
  QList<QStringList> * curTexts = NULL;
  if(savedTexts)
    curTexts = &savedTexts->last();
  int lineNumber = 0;
  QRegExp commentRE(commentREt);
  QRegExp blankLineRE(blankREt);

  QLocale locale = QLocale::c(); /// @todo offer the possibility to customize.

  // We need the text column list in reverse order
  std::sort(textColumns.begin(), textColumns.end()); 
  std::reverse(textColumns.begin(), textColumns.end());

  LineReader s(source);
  int numberRead = 0;
  QVector<int> indices;
  while(! s.atEnd()) {
    lineNumber++;
    QString line = s.readLine();
    if(skip >= lineNumber)
      continue;
    if(commentRE.indexIn(line) >= 0) {
      if(comments)
        *comments << line;
      continue;
    }
    if(blankLineRE.indexIn(line) == 0) {
      if(splitOnBlank && curCols->size() > 0 && curCols->first().size() > 0) {
        // we split !
        retval << QList<Vector>();
        if(savedTexts) {
          *savedTexts << QList<QStringList>();
          curTexts = &savedTexts->last();
        }
        curCols = &retval.last();
        numberRead = 0;
      }
      continue;
    }
    
    splitter(line, &indices);
    int idx = textColumns.size();
    QStringList taken;

    // the number of numbers
    int numbers = indices.size() / 2;
    // QTextStream o(stdout);
    // o << "line: " << line << " -> " << indices.size() << endl;
    for(int txt : textColumns) {
      // o << "txt: " << txt << ", indices:" << indices.size() << endl;
      if(txt >= (indices.size()/2) || txt < 0)
        continue;
      // o << " -> " << indices[2*txt] << ": " << indices[2*txt+1] << endl;
      taken.insert(0, line.mid(indices[2*txt], indices[2*txt+1]));
      // o << "..." << endl;
      indices[2*txt] = -1;
      --numbers;
    }
    // o << " -> done reading text"  << endl;

    /// @todo customize trimming.
    while(curCols->size() < numbers) {
      *curCols << Vector(numberRead, std::nan(""));
      curCols->last().reserve(10000); // Most files won't be as large
    }
    int nbNans = 0;
    int col = 0;
    for(int i = 0; i < curCols->size(); i++) {
      // o << "Column: " << i << endl;
      bool ok = false;
      while(col < indices.size() && indices[col] < 0)
        col += 2;
      // o << "col: " << col << ", " << line.size() << endl;
      double value;
      if(col < indices.size()) {
        // o << "idx: " << indices[col] << "," << indices[col+1] << endl;
        QStringRef s(&line, indices[col], indices[col+1]);
        if(! decimalSep.isEmpty()) {
          QString s2 = s.toString();
          s2.replace(decimalSep, ".");
          value = locale.toDouble(s2, &ok);
        }
        else
          value = locale.toDouble(s, &ok);
        if(! ok) {
          value = std::nan(""); /// @todo customize
          nbNans++;
        }
      }
      else {
        value = std::nan("");
        nbNans++;
      }
      (*curCols)[i] << value;
      col += 2;
    }
    // We remove lines fully made of NaNs
    if(nbNans == curCols->size()) {
      /// @todo This will not go well with text
      for(int i = 0; i < curCols->size(); i++)
        (*curCols)[i].resize((*curCols)[i].size() - 1);
    }
    else {
      if(curTexts) {
        for(int i = 0; i < taken.size(); i++) {
          if(curTexts->size() <= i)
            *curTexts << QStringList();
          QStringList & l = (*curTexts)[i];
          while(l.size() < numberRead)
            l << "";
          l << taken[i];
        }
      }
      numberRead++;
    }
  }

  // We don't leave an empty dataset at the end, excepted if it's the
  // last one ;-)...
  if(curCols->size() == 0 && retval.size() > 1)
    retval.takeLast();
  

  // // Trim the values in order to save memory a bit (at the cost of
  // // quite a bit of reallocation/copying time)
  // for(int i = 0; i < curCols->size(); i++)
  //   retVal[i].squeeze();

  return retval;
}

QList<QList<Vector> > Vector::readFromStream(QTextStream * source,
                                             const QRegExp & separatorREt,
                                             const QRegExp & commentREt,
                                             bool splitOnBlank,
                                             const QString & decimalSep,
                                             const QRegExp & blankREt,
                                             QStringList * comments,
                                             int skip,
                                             QList<int> textColumns,
                                             QList<QList<QStringList> > * savedTexts,
                                             bool trim)
{
  QRegExp separatorRE(separatorREt);
  return Vector::readFromStream(source,
                                [&separatorRE,trim](const QString & str, QVector<int> * indices)  {
                                  int tgt = 2;
                                  int idx = 0;
                                  int eof = str.size();
                                  if(trim) {
                                    while(str[idx].isSpace() && idx < eof)
                                      ++idx;
                                    while(str[eof-1].isSpace() && idx < eof)
                                      --eof;
                                  }
                                  if(idx == eof) {
                                    indices->resize(0);
                                    return;
                                  }
                                  /// @todo Watch out for trimmed stuff...
                                  // QTextStream o(stdout);
                                  // o << "String: " << str << endl;
                                  /// @todo what out when no data is found ?
                                  while(indices->size() <= tgt+1)
                                    *indices << 0;
                                  (*indices)[0] = idx;
                                  while((idx = separatorRE.indexIn(str, idx)) >= 0) {
                                    if(idx >= eof)
                                      break;
                                    // o << " -> idx: " << idx
                                    //   << ", tgt: " << tgt << endl;
                                    while(indices->size() <= tgt+1)
                                      *indices << 0;
                                    (*indices)[tgt-1] = idx -
                                      (*indices)[tgt-2];
                                    idx += separatorRE.matchedLength();
                                    (*indices)[tgt] = idx;
                                    tgt += 2;
                                  }
                                  (*indices)[tgt-1] = eof -
                                    (*indices)[tgt-2];
                                  indices->resize(tgt);
                                  // for(int i = 0; i < indices->size()/2; i++)
                                  //   o << " -> " << (*indices)[2*i] << ", "
                                  //     << (*indices)[2*i+1] << endl;
                                }, commentREt, splitOnBlank,
                                decimalSep, blankREt, comments, skip,
                                textColumns, savedTexts);
}

QList<QList<Vector> > Vector::readFromStream(QTextStream * source,
                                             const QString & separatorREt,
                                             const QString & commentREt,
                                             bool soB,
                                             const QString & blankRE,
                                             QStringList * comments)
{
  return readFromStream(source, QRegExp(separatorREt),
                        QRegExp(commentREt), soB, QString(),
                        QRegExp(blankRE),
                        comments);
}

/// @todo A lot of code duplication here.
double Vector::min() const
{
  int sz = size();
  const double * d = (sz > 0 ? data() : NULL);
  while(sz > 0 && std::isnan(*d)) {
    sz--;
    d++;
  }
  if(! sz)
    return std::nan("");
  double m = d[0];
  for(int i = 1; i < sz; i++)
    if(d[i] < m)
      m = d[i];
  return m;
}

double Vector::finiteMin() const
{
  int sz = size();
  const double * d = (sz > 0 ? data() : NULL);
  while(sz > 0 && ! std::isfinite(*d)) {
    sz--;
    d++;
  }
  if(! sz)
    return std::nan("");
  double m = d[0];
  for(int i = 1; i < sz; i++) {
    if(! std::isfinite(d[i]))
       continue;
    if(d[i] < m)
      m = d[i];
  }
  return m;
}

bool Vector::allFinite() const
{
  int sz = size();
  const double * v = data();
  for(int i = 0; i < sz; i++)
    if(! std::isfinite(v[i]))
      return false;
  return true;
}

int Vector::whereMin() const
{
  int sz = size();
  if(! sz)
    return -1;
  const double * d = data();
  double m = d[0];
  int lastidx = 0;
  for(int i = 1; i < sz; i++)
    if(d[i] < m) {
      m = d[i];
      lastidx = i;
    }
  return lastidx;
}


double Vector::max() const
{
  int sz = size();
  const double * d = (sz > 0 ? data() : NULL);
  while(sz > 0 && std::isnan(*d)) {
    sz--;
    d++;
  }
  if(! sz)
    return std::nan("");
  double m = d[0];
  for(int i = 1; i < sz; i++)
    if(d[i] > m)
      m = d[i];
  return m;
}

double Vector::finiteMax() const
{
  int sz = size();
  const double * d = (sz > 0 ? data() : NULL);
  while(sz > 0 && !std::isfinite(*d)) {
    sz--;
    d++;
  }
  if(! sz)
    return std::nan("");
  double m = d[0];
  for(int i = 1; i < sz; i++) {
    if(! std::isfinite(d[i]))
       continue;
    if(d[i] > m)
      m = d[i];
  }
  return m;
}

int Vector::whereMax() const
{
  int sz = size();
  if(! sz)
    return -1;
  const double * d = data();
  double m = d[0];
  int lastidx = 0;
  for(int i = 1; i < sz; i++)
    if(d[i] > m) {
      m = d[i];
      lastidx = i;
    }
  return lastidx;
}

double Vector::magnitude() const
{
  int sz = size();
  if(! sz)
    return std::nan("");
  const double * d = data();
  double m = d[0];
  for(int i = 1; i < sz; i++)
    if(fabs(d[i]) > fabs(m))
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

gsl_vector * Vector::toGSLVector()
{
  view = vectorView();
  return &view.vector;
}

const gsl_vector * Vector::toGSLVector() const
{
  return const_cast<Vector*>(this)->toGSLVector();
}



int Vector::findCrossing(int seed, double y, int di, bool * found) const
{
  bool fnd = false;
  if(di > 0)
    di = 1;
  else
    di = -1;
  int sz = size();
  if(seed >= sz)
    throw RuntimeError("Seed outside of the vector");

  const double * vals = data();

  double sgn = (vals[seed] > y ? 1.0 : -1.0);
  int idx = seed;
  while(idx >= 0 && idx < sz) {
    double yv = vals[idx];
    if(sgn * (yv - y) <= 0.0) {
      fnd = true;
      break;
    }
    idx += di;
  }
  if(found)
    *found = fnd;
  return idx;
}


Vector Vector::fromGSLVector(const gsl_vector * vect)
{
  Vector ret;
  ret.reserve(vect->size);
  for(size_t i = 0; i < vect->size; i++)
    ret << gsl_vector_get(vect, i);
  return ret;
}

Vector Vector::deltas(bool centered) const
{
  Vector ret;
  if(centered)
    throw InternalError("Not implemented");
  else {
    ret.reserve(size()-1);
    for(int i = 0; i < size() - 1; i++)
      ret << value(i+1) - value(i);
  }
  return ret;
}

void Vector::deltaStats(double * pdmin, double * pdmax) const
{
  double dmin = fabs(value(1) - value(0));
  double dmax = dmin;
  for(int i = 1; i < size() - 1; i++) {
    double d = fabs(value(i+1) - value(i));
    if(d > dmax)
      dmax = d;
    else if(d < dmin)
      dmin = d;
  }
  if(pdmin)
    *pdmin = dmin;
  if(pdmax)
    *pdmax = dmax;
}

void Vector::stats(double * average, double * variance, double * sum) const
{
  *average = 0;
  *variance = 0;
  int sz = size();
  for(int i = 0; i < sz; i++) {
    double val = value(i);
    *average += val;
    *variance += val*val;
  }
  if(sum)
    *sum = *average;
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

double Vector::deltaSum() const
{
  double s = 0.0;
  for(int i = 1; i < size(); i++)
    s += fabs(value(i) - value(i-1));
  return s;
}


bool Vector::isJaggy(double threshold) const
{
  if(threshold <= 0)
    return false;
  double dx = deltaSum()/(max() - min());
  if(dx > threshold)
    return true;
  return false;
}

void Vector::applyFunction(const std::function<double (double)> & func)
{
  int sz = size();
  double * d = data();
  for(int i = 0; i < sz; ++i, ++d)
    *d = func(*d);
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

bool Vector::hasOnlyNaN() const
{
  for(int i = 0; i < size(); i++) {
    if(! std::isnan(value(i)))
      return false;
  }
  return true;
}

bool Vector::hasNotFinite() const
{
  for(int i = 0; i < size(); i++) {
    if(! std::isfinite(value(i)))
      return true;
  }
  return false;
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

Vector Vector::indexVector(int nb)
{
  Vector r(nb, 0);
  for(int i = 0; i < nb; i++)
    r[i] = i;
  return r;
}


Vector Vector::logarithmicallySpaced(double min, double max, int nb)
{
  Vector r(nb, 0);
  if(nb < 2)
    throw RuntimeError("Cannot create a segment-spanning vector of "
                       "less than 2 points !");
  if(max*min <= 0)
    throw RuntimeError("Cannot create log scale in this range "
                       "%1 to %2").arg(min).arg(max);
  double sgn = max < 0 ? -1 : 1;
  min = ::log(sgn * min);
  max = ::log(sgn * max);
  for(int i = 0; i < nb; i++) 
    r[i] = sgn * exp(min + (max - min) * i/(nb - 1));
  return r;
}

Vector Vector::uniformlySpaced(int nb) const
{
  if(nb < 2)
    nb = size();
  if(nb == 1)
    return *this;               // Well, it's still correct, isn't it ?
  return uniformlySpaced(min(), max(), nb);
}


Vector Vector::downSample(int factor) const
{
  // bounds checking ?
  Vector ret;
  if(factor >= size())
    factor = size()/2;
  
  if(factor <= 1)
    factor = 1;
  
  int cur_nb = 0;
  double cur_val = 0;
  for(int i = 0; i < size(); i++) {
    if(cur_nb == factor) {
      ret << cur_val / factor;
      cur_nb = 0;
      cur_val =0;
    }
    cur_val += value(i);
    cur_nb++;
  }
  if(cur_nb)
    ret << cur_val / cur_nb;

  return ret;
}


/// @todo Use a more time-efficient algorithm ? See
/// http://stackoverflow.com/questions/2579912/how-do-i-find-the-median-of-numbers-in-linear-time-using-heaps
double Vector::median() const
{
  if(! size())
    throw RuntimeError("Need at least one element !");
  Vector v = *this;
  std::sort(v.begin(), v.end());
  return v[v.size()/2];
}

double Vector::integrate(const Vector & x, const Vector & y)
{
  int sz = y.size();
  if(x.size() != sz)
    throw RuntimeError("X and Y vectors must have same size (%1 vs %2)").
      arg(x.size()).arg(sz);
  double sum = 0;
  for(int j = 1; j < y.size(); j++)
    sum += (x[j] - x[j-1]) * 0.5 * (y[j] + y[j-1]);
  return sum;
}

Vector Vector::integrateVector(const Vector & x, const Vector & y, int idx)
{
  int sz = y.size();
  if(x.size() != sz)
    throw RuntimeError("X and Y vectors must have same size (%1 vs %2)").
      arg(x.size()).arg(sz);
  
  if(idx < 0 || idx >= sz)
    idx = 0;
  double sum = 0;
  Vector re = x;
  re[idx] = 0;
  for(int j = idx + 1; j < sz; j++) {
    sum += (x[j] - x[j-1]) * 0.5 * (y[j] + y[j-1]);
    re[j] = sum;
  }
  sum = 0;
  for(int j = idx - 1; j >= 0; j--) {
    sum += (x[j] - x[j+1]) * 0.5 * (y[j] + y[j+1]);
    re[j] = sum;
  }
  return re;
}

void Vector::reverse()
{
  int sz = size();
  for(int i = 0; i < sz/2; i++) {
    double s = value(i);
    (*this)[i] = value(sz - i - 1);
    (*this)[sz - i - 1] = s;
  }
}


QList<Vector> Vector::bin(int boxes, bool lg, const Vector & weights,
                          bool norm, double mi, double ma) const
{
  if(std::isnan(mi))
     mi = min();
  if(std::isnan(ma))
     ma = max();

  if(lg && mi <= 0)
    throw RuntimeError("Cannot bin using a log scale with negative numbers !");

  gsl_histogram * hist = gsl_histogram_alloc(boxes);

  if(lg) {
    mi = log10(mi);
    ma = log10(ma);
  }

  // We extend the ma slightly to higher values in order for the
  // maximum to be counted too (remember range is exclusive for the
  // upper bound !)
  ma += 1e-5 * (ma - mi);
    
  gsl_histogram_set_ranges_uniform(hist, mi, ma);

  double total = 0;
  for(int i = 0; i < size(); i++) {
    double v = value(i);
    if(lg)
      v = log10(v);
    double w;
    if(weights.size() > 0)
      w = weights[i];
    else
      w = 1;
    
    gsl_histogram_accumulate(hist, v, w);
    total += w;
  }
  

  Vector mid;
  Vector bin;

  for(int i = 0; i < boxes; i++) {
    double lr, ur;
    gsl_histogram_get_range(hist, i, &lr, &ur);
    mid << 0.5 * (lr + ur);
    double v = gsl_histogram_get(hist, i);
    if(norm)
      v /= total;
    bin << v;
  }
  

  gsl_histogram_free(hist);

  QList<Vector> ret;
  ret << mid << bin;
  return ret;
}

QStringList Vector::asText(int fieldWidth, char format, int precision,
                           const QChar & fillChar) const
{
  QString fmt("%1");
  QStringList ret;
  for(int i = 0; i < size(); i++)
    ret << fmt.arg(value(i), fieldWidth, format, precision, fillChar);
  return ret;
}

int Vector::bfind(double value) const
{
  int left = 0, right = size() -1;
  if(right < 0)
    return 0;
  const double *d = data();
  if(value < d[left])
    return 0;
  if(std::isnan(value)) {
    if(std::isnan(d[right]))
      return right;
    else
      return right+1;
  }
  if(std::isnan(d[right]))
    right--;
  if(right < 0)
    return 0;
  if(value > d[right])
    return right+1;

  while(right - left > 1) {
    if(value == d[left])
      return left;
    if(value == d[right])
      return right;
    int nd = (left+right)/2;
    if(value <= d[nd])
      right = nd;
    else
      left = nd;
  }
  return right;
}

Vector Vector::values(double tolerance) const
{
  Vector rv;

  int sz = size();
  const double *d = data();
  for(int i = 0; i < sz; i++) {
    if(rv.size() == 0) {
      rv << d[i];
      continue;
    }
    int idx = rv.bfind(d[i]);
    if(idx >= rv.size() || (! Utils::fuzzyCompare(d[i], rv[idx], tolerance)))
      rv.insert(idx, d[i]);
  }
  return rv;
}

bool Vector::withinTolerance(const Vector & x, const Vector & y, double tol)
{
  int sz = x.size();
  if(sz != y.size())
    return false;

  for(int i = 0; i < sz; i++) {
    if(x[i] == 0) {
      if(y[i] == 0)
        continue;
      else
        return false;
    }
    if(fabs((x[i] - y[i])/x[i]) > tol)
      return false;
  }
  return true;
}

void Vector::randomize(double low, double high)
{
  int sz = size();
  for(int i = 0; i < sz; i++) {
    double scale = Utils::random(low, high);
    (*this)[i] *= scale;
  }
}


void Vector::rotate(int delta)
{
  if(delta == 0)
    return;

  int sz = size();
  double * vals = data();

  int d = abs(delta);

  QVarLengthArray<double, 100> buffer(d);
  double * bu = buffer.data();
  // QTextStream o(stdout);
  auto ref = [&vals, &bu, d, sz](int i) -> double & {
               // o << "ref: " << i  << endl;
            if(i >= sz)
              return bu[i-sz];
            if(i < 0)
              return bu[d+i];
            return vals[i];
          };
  if(delta > 0) {
    for(int i = sz - 1; i >= -delta; i--) {
      // i is the source
      double & tgt = ref(i+delta);
      const double & src = ref(i);
      // o << "Copy at : " << i << ", " << tgt
      //   << " becomes " << src << endl;
      tgt = src;
    }
  }
  else {
    for(int i = delta; i < sz; i++) {
      // i is the source
      const double & src = ref(i-delta);
      double & tgt = ref(i);
      // o << "Copy at : " << i << ", " << tgt
      //   << " becomes " << src << endl;
      tgt = src;
    }
  }
}

/// Very dumb class to classify data by order of magnitude
class Order {
  double logSum;
public:
  Vector xvals, yvals;

  void append(double x, double y) {
    xvals << x;
    yvals << y;
    logSum += log10(y);
  };

  Order(double x, double y) : logSum(0) {
    append(x, y);
  };

  /// Returns true if
  bool isMine(double y, double mag = 1.5) const {
    y = log10(y);
    if(fabs(y - logSum/xvals.size()) < mag)
      return true;
    return false;
  };

  
  
};

QList<QList<Vector> > Vector::orderOfMagnitudeClassify(const Vector & xv,
                                                       const Vector & yv,
                                                       double tolerance)
{
  QList<Order> mags;
  for(int i = 0; i < xv.size(); i++) {
    bool found = false;
    double x = xv[i], y = yv[i];
    for(Order & o : mags) {
      if(o.isMine(y)) {
        o.append(x,y);
        found = true;
        break;
      }
    }
    if(! found) {
      mags << Order(x,y);
    }
  }
  QList<QList<Vector> > rv;
  for(const Order & t : mags) {
    QList<Vector> cols;
    cols << t.xvals << t.yvals;
    rv << cols;
  }
  return rv;
}

double Vector::correlation(const Vector & x, const Vector & y)
{
  if(x.size() != y.size())
    throw InternalError("Vectors must have the same size: %1 vs %2").
      arg(x.size()).arg(y.size());

  double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
  int nb = x.size();
  for(int i = 0; i < nb; i++) {
    sx += x[i];
    sy += y[i];
    sxx += x[i]*x[i];
    syy += y[i]*y[i];
    sxy += x[i]*y[i];
  }
  sx /= nb;
  sy /= nb;
  sxx /= nb;
  syy /= nb;
  sxy /= nb;
  return (sxy - sx*sy)/(sqrt((sxx - sx*sx)*(syy - sy*sy)));
}

#include <integrator.hh>


void Vector::convolve(const double * vector,
                      int nb,
                      double * target,
                      double xmin,
                      double xmax,
                      std::function<double (double)> function,
                      bool symmetric,
                      double * buffer)
{
  double dx = (xmax - xmin)/(nb-1);
  int elements = (symmetric ? 2*nb : nb);
  int center = (symmetric ? nb - 1 : -1);

  double xr;

  std::function<double (double)> fnl = [&xr, &function](double x) -> double {
    return function(xr - x);
  };


  std::function<double (double)> xfnl = [&xr, &dx, &function](double x) -> double {
    double xv = xr - x;
    return x*function(xv)/dx;
  };


  ////////////////////////////////////////
  // First step: computation of the coefficients of the kernel

  double * av = buffer;
  double * bv = buffer + elements;

  // The absolute values of the and b coefficients
  double amax = 0, bmax = 0;
  int amax_i = 0, bmax_i = 0;

  QSet<int> needUpdate;

        //   std::unique_ptr<Integrator> in(Integrator::createNamedIntegrator());
        //   double err = 0;
        //   av[i] = in->integrateSegment(fnl, 0, dx, &err);
        //   bv[i] = in->integrateSegment(xfnl, 0, dx, &err);
        // }
  

    // Wow, this works very very well.
  for(int i = 0; i < elements; i++) {
    // We must take special care for the integrals around 0, i.e. for
    // c == 1 (and c == 0 if we allow for a singular point on both sides)
    
      int c = i - center;
      xr = c*dx;
      // We need to handle specially the case i == 0 for the
      // non-symmetric case, to allow for singularities or
      // "pseudo-singularities" (like functions vanishing very quickly
      // to 0, like fast decaying exponentials)"
      //
      // OK, this function actually works fine now for almost all the
      // cases but the ones in which the decrease is sharp but not so
      // sharp as to be negligible on the second element.

      if(! symmetric && c == 1) {
        int subdiv = 0;
        // Final values of a and b
        double a,b, a_prev = 0, b_prev = 0;
        // Values of the leftmost segment
        double left_a, left_b;
        // Values of the rest
        double right_a = 0, right_b = 0;
        // Value of the right side of the current 
        double cur_f = fnl(0);
        // Keep in mind that fnl is defined right to left.
        double left_f = fnl(dx);
        double cur_dx = dx;

        bool missing_left = false;

        if(std::isinf(left_f))
          missing_left = right;


        // Here, we subdivide only the leftmost segment
        // really should be enough
        // QTextStream o(stdout);
        // o << "Computing for: " << formula << endl;
        while(subdiv < 40) {
          if(! missing_left) {
            left_a = 0.5*(left_f + cur_f) * cur_dx;
            left_b = 0.5*(left_f + cur_f * (dx - cur_dx)/dx) * cur_dx;
            
            a = left_a + right_a;
            b = left_b + right_b;
          }
          else {
            // Extrapolate to 0
            a = right_a * (dx)/(dx-cur_dx);
            b = right_b * (dx)/(dx-cur_dx);
          }
          // o << "Iteration #" << subdiv << "@ " << cur_dx << "/" << dx << "\n"
          //   << "a = " << a << "\tb = " << b << "\tcur_f = " << cur_f << "\n"
          //   << "right_a = " << right_a << "\tright_b = " << right_b << endl;
          if(subdiv > 0) {
            if(abs(a - a_prev) < 0.001*abs(a) &&
               abs(b - b_prev) < 0.001*abs(b)) // We're good enough
              break;
          }
          a_prev = a;
          b_prev = b;
          cur_dx *= 0.5;
          double prev_f = cur_f;
          cur_f = fnl(dx - cur_dx);
          
          right_a += 0.5*(cur_f + prev_f) * cur_dx;
          right_b += 0.5*(
                          cur_f * (dx - cur_dx)/dx +
                          prev_f * (dx - 2*cur_dx)/dx
                          ) * cur_dx;
          subdiv += 1;
        }

        av[i] = a;
        bv[i] = b;
        // o << "Final:\n"
        //   << "a = " << a << "\tb = " << b << endl;

        // I think the singularities We need a much better handling of
        // the integration, in particular, we need to go over the whole
        if(subdiv >=3 && !missing_left)
          needUpdate.insert(i);
      }

      else {
        av[i] = 0.5*(fnl(0) + fnl(dx))*dx;

        bv[i] = 0.5*(xfnl(0) + xfnl(dx))*dx;
      }
      if(i == 0) {
        amax = fabs(av[i]);
        bmax = fabs(bv[i]);
      }
      else {
        double ab_a = fabs(av[i]);
        if(amax < ab_a) {
          amax_i = i;
          amax = ab_a;
        }
        double ab_b = fabs(bv[i]);
        if(bmax < ab_b) {
          bmax_i = i;
          bmax = ab_b;
        }
      }
  }

  int lefti = 0;
  while(fabs(av[lefti]) < 1e-16 * amax &&
        fabs(bv[lefti]) < 1e-16 * bmax)
    lefti++;

  int righti = elements-1;
  while(fabs(av[righti]) < 1e-16 * amax &&
        fabs(bv[righti]) < 1e-16 * bmax)
    righti--;

  // OK, now we look for each relevant place and find elements which
  // are not too small by varying by more than 20% between
  double threshold = 3e-4;
  double variation = 0.8;
  for(int i = lefti; i < righti; i++) {
    if(i < amax_i) {
      if(fabs(av[i]) > threshold &&
         fabs(av[i]) < variation * fabs(av[i+1]))
        needUpdate.insert(i);
    }
    else {
      if(fabs(av[i]) > threshold &&
         fabs(av[i]) < variation * fabs(av[i-1]))
        needUpdate.insert(i);
    }
    if(i < bmax_i) {
      if(fabs(bv[i]) > threshold &&
         fabs(bv[i]) < variation * fabs(bv[i+1]))
        needUpdate.insert(i);
    }
    else {
      if(fabs(bv[i]) > threshold &&
         fabs(bv[i]) < variation * fabs(bv[i-1]))
        needUpdate.insert(i);
    }
  }

  std::unique_ptr<Integrator> in(Integrator::createNamedIntegrator());
  for(int i : needUpdate) {
    int c = i - center;
    double err;
    xr = c*dx;
    av[i] = in->integrateSegment(fnl, 0, dx, &err);
    bv[i] = in->integrateSegment(xfnl, 0, dx, &err);
  }

  
  /// @todo Post-processing work to:
  /// @li detect the max of a and b (and their position ?)
  /// @li detect elements that should be better computed, using
  /// like more than a 30% decrease in one step, and more than 1% total ?
  /// @li the elements on the side that are consistently less than 1e-16
  /// of the max, which should be removed from the convolution itself.

  ////////////////////////////////////////
  // Now the convolution proper
  const double * yv = vector;
  for(int i = 0; i < nb; i++) {
    double sum = 0;
    int minj = std::max(0, center + i - righti);
    int maxj = std::min(nb-1, center + i - lefti + 1);
    for(int j = minj; j < maxj; j++) {
      int k = i - j + center;
      /// @todo this can be included in the loop
      // if(k >= lefti && k <= righti)
      // if(k >= 0)
      sum += (av[k] - bv[k]) * yv[j] + bv[k] * yv[j+1];
    }
    target[i] = sum;
  }


}
