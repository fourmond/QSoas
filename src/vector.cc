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
      if(txt > (indices.size()/2) || txt < 0)
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
  qSort(v);
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
