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

QList<Vector> Vector::readFromStream(QIODevice * source,
                                     const QString & separatorREt,
                                     const QString & commentREt,
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

  // QTextStream o(stdout);
  // o << "Read " << retVal.size() << " columns and "
  //   << numberRead << " rows and " << lineNumber << " lines" << endl;
  return retVal;
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
  if(sz != b.size()) {
    throw std::runtime_error("Size mismatch in vector operation");
  }
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
  if(sz != b.size()) {
    throw std::runtime_error("Size mismatch in vector operation");
  }
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
